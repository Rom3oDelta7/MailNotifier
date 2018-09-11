/*
	TABS=3
	Mailbox notifier
	Simple Blynk interface for a  magnetic reed switch application
	Supports OTA uploads

	Copyright 2017/2018 Rob Redford
	This work is licensed under the Creative Commons Attribution-ShareAlike 4.0 International License (CC-BY-SA).
	To view a copy of this license, visit https://creativecommons.org/licenses/by-sa/4.0/

*/

//#define DEBUG 3                                    // define as MAX desired debug level, or comment out to disable debug statements

#ifdef DEBUG
#define BLYNK_PRINT Serial    							// Comment this out to disable prints and save space

#define DEBUG_MSG(L, H, M)	       if ((L) <= DEBUG) {Serial.print("DEBUG> "); Serial.print(H); Serial.print(": "); Serial.println(M);}
#else
#define DEBUG_MSG(...)            ;
#endif

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <NTPRTC.h>											// https://github.com/Rom3oDelta7/NTP-RTC
#include <WorldTimezones.h>								// https://github.com/Rom3oDelta7/NTP-RTC

// ============================= Blynk ==================================================

#include "locals.h"

#define DATE_DISPLAY		V1									// Labeled Value, L
#define TIME_DISPLAY		V2									// Labeled Value, L
#define LED_READY			V3									// LED
#define LED_OPEN			V4									// LED
#define RESET_BUTTON		V5									// Button, PUSH
#define START_STOP	   V6									// Time input
#define LED_COLLECTED	V7									// LED
#define REBOOT_SLIDER	V8									// Slider, small
#define IGNORED_EVENTS  V9                         // Value, Small

WidgetLED readyLED(LED_READY);
WidgetLED openedLED(LED_OPEN);
WidgetLED collectedLED(LED_COLLECTED);

// ============================= Hardware & Logic ==========================================

#define NOISE_WINDOW       250                     // dwell time in msec for switch bounce & EMI noise
#define COLLECTION_DELAY	600							// delay in sec after opening before collection window opens
#define CHECK_INTERVAL     500                     // how frequently to check the mailbox (msec)
#define REBOOT_DELAY			5000							// delay in msec for reboot slider

#define DEFAULT_START  		(11 * 3600)	            // 11AM
#define DEFAULT_STOP			(20 * 3600)             // 8PM

int      startTimeInSecs = DEFAULT_START;	         // start & stop times from dashboard settings
int      stopTimeInSecs = DEFAULT_STOP;     

int      ignoredEventCount = 0;                    // count of opening events that were ignored

enum CollectionState { C_INACTIVE, C_PENDING, C_ACTIVE };
CollectionState waitingForCollection;				   // flag for 2nd opening, which we assume is to collect the mail

bool     cycleComplete = false;                    // set after mail has been collected to disable further notifications for the day

#define SWITCH_PIN		4									// NC switch connected to ground (WeMos D2)
#define RUNNING_LED		5									// LED pilot light (WeMos D1); 10 ohm resistor, Green LED

#define SENSE_CLOSED    LOW                        // pin state when mailbox is closed
  

/*
 reset the mailbox switch to normal state
*/
void resetMbox ( void ) {
	openedLED.off();
	collectedLED.off();
	ignoredEventCount = 0;
	Blynk.virtualWrite(TIME_DISPLAY, " ");
	Blynk.virtualWrite(DATE_DISPLAY, " ");
	Blynk.virtualWrite(IGNORED_EVENTS, ignoredEventCount);
	waitingForCollection = C_INACTIVE;
   cycleComplete = false;
}

/*
 Blynk dashboard reset button
*/
BLYNK_WRITE ( RESET_BUTTON ) {
	resetMbox();
}

/*
 set the start and stop time for monitoring the mailbox
 use the Blynk convention of seconds since midnight
 if user does not set this the sketch assumes 24 hour operation
*/
BLYNK_WRITE ( START_STOP ) {
	TimeInputParam startStopTime(param);

	DEBUG_MSG(0, "Called", "START_STOP <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
	if ( startStopTime.hasStartTime() ) {
		startTimeInSecs = param[0].asInt();
		if ( startTimeInSecs == -1 ) {
			// no user data entered
			startTimeInSecs = 0;
		}
		DEBUG_MSG(1, "\t\tStart", startTimeInSecs);
	}
	if ( startStopTime.hasStopTime() ) {
		stopTimeInSecs = param[1].asInt();
		if ( stopTimeInSecs == -1 ) {
			stopTimeInSecs = (23*3600) + (59*60) + 59;            // one sec before midnight
		}
		DEBUG_MSG(1, "\t\tStop", stopTimeInSecs);
	}
}


/*
 Reboot the ESP when the slider is all the way to the right (with some margin) but allow the user
 to slide it back to cancel within the specified delay (see above)
 
 Note this can also be used as a test to verify that the ESP is online and responding to commands
*/
BLYNK_WRITE ( REBOOT_SLIDER ) {
	int 		value = param.asInt();
	static 	bool rebootInitiated = false;
	static 	uint32_t rebootRequested = 0;
	
	if ( !rebootInitiated ) {
		if ( value > 90 ) {
			// slider is set to send 0:100
			Blynk.virtualWrite(DATE_DISPLAY, "Slide to 0");
			Blynk.virtualWrite(TIME_DISPLAY, "to cancel");
			rebootInitiated = true;
			rebootRequested = millis();
			readyLED.off();													// turn off to indicate a pending reboot
			Blynk.syncVirtual(REBOOT_SLIDER);                     // send new widget values to dashboard	by forcing call to BLYNK_WRITE()
		}
	} else {
		// process reboot request
		if ( value <= 10 ) {
			// cancel reboot
			rebootInitiated = false;
			Blynk.virtualWrite(DATE_DISPLAY, " ");
			Blynk.virtualWrite(TIME_DISPLAY, " ");
			if ( digitalRead(SWITCH_PIN) == SENSE_CLOSED ) {
				readyLED.on();
			}
		} else if ( (millis() - rebootRequested) < REBOOT_DELAY ) {
			// don't want to call delay(), so just request the value again from the server
			Blynk.syncVirtual(REBOOT_SLIDER);
		} else {
			// still set after delay, so reboot now
			ESP.restart();
			delay(1000);
		}
	}
}

/*
 SETUP
*/
void setup ( void ) {
	Serial.begin(115200);									// always instantiate Serial for OTA messages
   delay(20);
	DEBUG_MSG(0, "Mbox", "*** STARTUP***");

	pinMode(SWITCH_PIN, INPUT_PULLUP);					// normally closed switch, grounded
	pinMode(RUNNING_LED, OUTPUT);
	digitalWrite(RUNNING_LED, LOW);
	
	Blynk.begin(auth, ssid, pass);
	while ( !Blynk.connect() ) {							// wait for connection
		Blynk.run();
	}
	DEBUG_MSG(0, "Mbox", "Blynk CONNECTED");

	if ( digitalRead(SWITCH_PIN) == SENSE_CLOSED ) {
		readyLED.on();
	} else {
		readyLED.off();
	}

	openedLED.off();
	collectedLED.off();
   cycleComplete = false;
   waitingForCollection = C_INACTIVE;
   ignoredEventCount = 0;
	Blynk.virtualWrite(TIME_DISPLAY, " ");
	Blynk.virtualWrite(DATE_DISPLAY, " ");
	Blynk.virtualWrite(IGNORED_EVENTS, ignoredEventCount);
	// the following do not currently work	:-(
	//Blynk.setProperty(START_STOP, "color", "#ED9D00");     // change color to yellow
	//Blynk.virtualWrite(START_STOP, DEFAULT_START, DEFAULT_STOP);
	Blynk.syncVirtual(START_STOP);                           // get current start/stop times

	// initialize NTP source for RTC data
	NTP_UTC_Timezone(UTC_MST);
	NTP_Init();
	
	/*
	 OTA setup
	*/
	// Port defaults to 8266
	ArduinoOTA.setPort(8266);

	// Hostname defaults to esp8266-[ChipID]
	//ArduinoOTA.setHostname("myesp8266");

	// No authentication by default
	//ArduinoOTA.setPassword((const char *)"@kermit87");

	ArduinoOTA.onStart([]() {
		Serial.println("Start");
	});
	ArduinoOTA.onEnd([]() {
		Serial.println("\nEnd. Restarting ...");
		ESP.restart();
		delay(1000);
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		switch ( error ) {
		case OTA_AUTH_ERROR:
			Serial.println("Auth Failed");
			break;
			
		case OTA_BEGIN_ERROR:
			Serial.println("Begin Failed");
			break;
			
		case OTA_CONNECT_ERROR:
			Serial.println("Connect Failed");
			break;
			
		case OTA_RECEIVE_ERROR:
			Serial.println("Receive Failed");
			break;
			
		case OTA_END_ERROR:
			Serial.println("End Failed");
			break;
			
		default:
			Serial.println("Unknown error");
			break;
		}
	});
	ArduinoOTA.begin();
	Serial.println("OTA Ready:");
	Serial.print("\tIP address: ");
	Serial.println(WiFi.localIP());
	Serial.print("\tChip ID: ");
	Serial.println(ESP.getChipId(), HEX);
	
	digitalWrite(RUNNING_LED, HIGH);
}

/*
 update Blynk dashboard with the time the mailbox was opened
*/
void displayOpenedTime ( void ) {
	char currentTime[9];
	time_t	timeNow = now();
	
	// use a constant time value to avoid corner cases where, for example, the hour changes between function calls
	sprintf(currentTime, "%02d:%02d:%02d", hour(timeNow), minute(timeNow), second(timeNow));
	String currentDate = String(year(timeNow)) + "." + month(timeNow) + "." + day(timeNow);
	DEBUG_MSG(2, "current time", currentTime);
	DEBUG_MSG(2, "current date", currentDate);

	Blynk.virtualWrite(TIME_DISPLAY, currentTime);
	Blynk.virtualWrite(DATE_DISPLAY, currentDate);
}

/*
 Check switch pin to determine if the mailbox is open.
 Normally the pin will be low; when open, the (internal) pullup will raise the line to high.
 But if this is an EMI event instead, then after some initial noise, the line will still be low.
 Therefore, if the pin is high, wait for the debounce/EMI noise to settle, then read the line again -
 this should be the actual state.
 
 Note: the EMI issue only comes into play if we happen to be reading the pin while the EMI is occurring.
 Since we're reading the pin quite frequently, this may still happen. Adjust the delay if the time interval is insufficient,
 but don't wait so long that the mailbox may have been closed after delivery.
 */
bool mailboxIsOpen ( void ) {
   if ( digitalRead(SWITCH_PIN) == SENSE_CLOSED ) {
      return false;
   } else {
      // if low again after delay, then initial read was a false positive
      delay(NOISE_WINDOW);
      return digitalRead(SWITCH_PIN);
   }
}

/*
 LOOP
*/
void loop ( void ) {
	static int    today = day();
	static time_t timeOpened = 0;
#ifdef DEBUG
	static bool   displayed = false;
#endif
	
	if ( today != day() ) {
		// automatically reset state every day
		today = day();
		resetMbox();
	}
	
	// check for operating hours. Default (params not set by user) is 24hr operation
	int currentTimeinSecs = ((3600 * hour()) + (60 * minute()) + second());
#if DEBUG > 3
	if ( (second() % 5) == 0 ) {
		if ( !displayed ) {
			displayed = true;
			DEBUG_MSG(3, "current seconds since midnight", currentTimeinSecs);
			DEBUG_MSG(3, "start", startTimeInSecs);
			DEBUG_MSG(3, "stop", stopTimeInSecs);
		}
	} else {
		displayed = false;
	}
#endif
   
   delay(CHECK_INTERVAL);                   // no need to constantly check - also helps with EMI
   if ( !cycleComplete ) {
      bool mailboxOpen = mailboxIsOpen();
      DEBUG_MSG(2, "Loop:Status:", mailboxOpen);
      if ( (currentTimeinSecs >= startTimeInSecs) && (currentTimeinSecs < stopTimeInSecs) ) {
         // inside the operating window
         if ( mailboxOpen ) {
            // mailbox initial open
            DEBUG_MSG(2, "OPEN: Collection state:", waitingForCollection);
            readyLED.off();

            switch ( waitingForCollection ) {
               case C_INACTIVE:
                  // first time mailbox has been opened
                  DEBUG_MSG(2, "Loop:\t", "mailbox opened");
                  Blynk.email(SMS_EMAIL, "Mbox Notifier", "Mailbox opened");
                  Blynk.notify("Mailbox opened");
                  displayOpenedTime();
                  timeOpened = now();
                  openedLED.on();
                  waitingForCollection = C_PENDING;
                  break;
                  
               case C_ACTIVE:
                  if ( (now() - timeOpened) >= COLLECTION_DELAY ) {
                     /*
                      second opening - assume this is to collect the mail
                      ignore it if not enough time has passed yet 
                     */
                     collectedLED.on();
                     Blynk.email(SMS_EMAIL, "Mbox Notifier", "Mail collected");
                     Blynk.notify("Mail collected");
                     DEBUG_MSG(2, "Loop:\t", "mail collected");
                     waitingForCollection = C_INACTIVE;
                     cycleComplete = true;
                     
                     readyLED.off();
                  }
                  break;
                  
               default:
                  break;
            }
         } else {
            if ( waitingForCollection == C_PENDING ) {
               // ensure mailbox has been closed first before checking for collection
               waitingForCollection = C_ACTIVE;
               DEBUG_MSG(3, "Loop:", "mailbox closed before collection");
            }
            readyLED.on();
         }
      } else {
         // ignore events outside working time (count events until cycle is complete, then stop counting)
         readyLED.off();                    // turn off when outside working window
         if ( mailboxOpen ) {
            ++ignoredEventCount;
            Blynk.virtualWrite(IGNORED_EVENTS, ignoredEventCount);
         }
      }
   }
   
	// housekeeping
	Blynk.run();
	yield();
	ArduinoOTA.handle();
}