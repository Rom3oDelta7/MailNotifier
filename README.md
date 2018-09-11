# MailNotifier
Hardware and software solution to be notified when your mail arrives (DRAFT)

This is a simple system that will notify you when your mailbox is opened.
Since the wiring was already in place for my home
I designed this with a wired connection to my mailbox using a standard magnetic reed switch such as that found in
security systems.
(You could modify this with an appropriate weatherized power supply and enclosure as long as WiFi access is available at the mailbox.)
There are 2 types of notification for redundancy: a notification from the Blynk dashboard and an SMS text message
sent via email using your carrier's system for sending a text message using email.

## Hardware

The hardware is quite simple. 
A WeMOS D1 Mini (ESP8266) is used as the controller. There are just 2 connections:
1. the reed switch (which should be normally closed (e.g. magnet is engaged)), and
1. a status LED

The system requires a 5V DC power supply, with a 2.1mm center-positive jack/plug.

Earlier versions of this system suffered from EMI interference when lightning stuck nearby,
a common occurrence during the summer monsoons here.
In Rev B hardware (and Rev 2 software), this issue is handled by using delays when determining the status of the mailbox reed switch as well as
insulating the ESP8266 with a diode and a MOSFET.
Note: if you are running the original hardware design you may still run Rev 2 software on it.

Manufactured PCBs may be ordered at <ZZZ INSERT URL ZZZ>.

## User Interface

The user interface is shown below:

<ZZZ image ZZZ>

* The "READY" LED (green) indicates that the mailbox is currently closed.
Note that the ready light will not be on outside operating hours (see below).
In the closed state, the circuit is closed and thus grounded.
This is important since the software is looking for a rising edge to signal that the mailbox has been opened.
* The "OPENED" LED (red) indicates that the mailbox has been opened.
* The "CLLCTD" LED (blue) indicates that the mail has been collected.
This is a heuristic: it is activated if the mailbox is opened _again_ after 10 minutes.
* "DATE" and "TIME" indicate the date and time the mailbox was first opened.
If the mailbox is opened multiple times, the date & time do _not_ change;
you must reset the system (see below) to clear the date & time and reset the indicators.
* The "STOP/START" field is used to set operating hours.
The default is 1100 to 2000 hours.
* "IGNORED" indicates how many times the mailbox was opened outside operating hours.
* The "RESET" button resets the LEDs and the date & time displays.
Note that if the system will reset itself automatically at midnight.
* The "REBOOT" slider is used to reboot the system.
To reboot, slide the switch all the way to the right.
You have 5 seconds to change your mind and slide it back to the left.
The "READY" LED will extinguish, a message about canceling the reboot will appear in the DATE and TIME displays,
and the system will reboot after a 5 second delay.
Note that this mechanism can also be used to ensure the end-to-end system is running: if the reboot message comes up
this indicates the system is active - just quickly slide the switch back to the left.

Note that all of the built-in delays can be easily personalized by changing the C++ preprocessor defines in the sketch.

## Firmware

The C++ code uses the [Blynk] service. 
You must first download the Blynk app for your smartphone (iOS or Android) and sign up for the service.
You can then download this app from within Blynk using the QR code in the "Blynk" folder.
See the [Blynk getting started guide].

The code uses the [Blynk Library], my [NTP (Network Time Protocol) clock library], and Paul Stoffregen's [Time Library].
Be sure to change the "auth", "SSID", and "password" for your installation,
and the email address string to the [address your carrier uses to send an SMS to your phone] in the file "locals.h". 
In the sketch file, set the parameter to the

```C++
NTP_UTC_Timezone()
```

function to your local timezone. 
See the [NTP (Network Time Protocol) clock library] for details.

The code may be updated using OTA (Over The Air) updates.
You can still upload the code in the traditional USB approach, but after the firmware has been uploaded for the first time
you can upload subsequent updates over WiFi. 
See the [ESP8266 OTA update documentation] for more details.

__Important__: access to the current time is essential to the operation of the system.
Therefore, you must have Internet access for the NTP library to work. 

## Enclosure

I have uploaded a 3D printed enclosure.
The wiring I piggybacked on to get to my mailbox happens to come into a patch panel,
so I have an RJ45 jack opening in my enclosure, which is what you see in the picture. 
WHen printing, you should turn on support everywhere.
Just about any filament is fine - I used PLA with a layer height of 0.4mm.

## Implementation

### Packaging

* Solder the WeMOS to the PermaProto board, ensuring that the top mounting hole is not covered up.
* Size and cut the wires.
* Solder the resistor. In the picture, the resistor connection to the WeMOS is wired on the bottom of the board (green wire).
* Solder the connection for the LED and use some 2mm shink wrap tubing to protect the leads from shorting
(or electrical tape if you don't have this).
* Mount the LED. The hole is sized for a 3mm LED. If the fit is too loose, then secure the LED to the case with some hot melt glue.
(Hot melt glue is an insulator, so don't worry about the LED connections.)
If you don't use a green LED, you should recalculate the value of the curent limiting resistor to match the Vf of your LED.
If you don't know what this means, just use a green LED.
* Mount the DC jack and solder the conenction wires.
* You can mount the proto board to the enclosure now, but you will probably find it easier to program it first (see below).

### Programming

* Ensure the aforementioned local/personal changes have been implemented in the sketch (.ino) file and "locals.h".
* For the first upload, connect a microUSB plug to the WeMos.
* In the Arduino IDE, select the "WeMOS D1 R2 & mini" board.
* Board settings:

|Parameter|Setting|
|---|---|
|CPU Frequency|160 MHz|
|Flash Size|4M(1M SPIFFS)|
|Upload SPeed|921600|
|Port|_your USB serial port_|

* The WeMOS has an automatic system for enabling the flash sequence so no buttons need to be pressed.
* After the code is uploaded, the LED will illuminate when all of the setup and initialization steps are complete.
* The USB cable may now be removed.
Subsequent firmware uploads can be done using the wireless OTA mechanism.
* At this point, you can launch the Blynk app and run the notifier app dashboard.



[Blynk]: http://www.blynk.cc/
[Blynk Library]: http://www.blynk.cc/getting-started/
[Blynk getting started guide]: http://www.blynk.cc/getting-started/
[NTP (Network Time Protocol) clock library]: https://github.com/Rom3oDelta7/NTP-RTC
[Time Library]: http://www.pjrc.com/teensy/td_libs_Time.html
[ESP8266 OTA update documentation]: http://esp8266.github.io/Arduino/versions/2.3.0/doc/ota_updates/readme.html
[address your carrier uses to send an SMS to your phone]: http://www.digitaltrends.com/mobile/how-to-send-e-mail-to-sms-text/


