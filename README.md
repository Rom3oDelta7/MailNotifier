# MailNotifier
Hardware and software solution to be notified when your mail arrives

![Apple Watch notification](https://github.com/Rom3oDelta7/MailNotifier/blob/master/Photos/apple%20watch.jpg)

This is a simple system that will notify you when your mailbox has been delivered.
You get a notification for the first opening (mail delivered) and the second (mail collected).
These notifications will be sent to your phone (using Blynk) and SMS text.
Optionally, you can also use the Blynk Apple Watch app to get the notifications on your watch.

Since the wiring was already in place for my home
I designed this with a wired connection to my mailbox using a standard magnetic reed switch such as that found in
security systems.
Any normally closed switch can be used to sense when the mailbox is open.
If your installation requires a normally open switch instead, there is a single define in the code that allows you to change the sense.

## Hardware

![board image](https://github.com/Rom3oDelta7/MailNotifier/blob/master/Photos/board%20image.jpg)

The hardware is quite simple. 
A WeMOS D1 R2 Mini (ESP8266) is used as the controller. There are just 2 connections:
1. the reed switch (which should be normally closed (e.g. magnet is engaged)), and
1. a status LED

The system requires a 5V 1A DC power supply.

Earlier versions of this system suffered from EMI interference when lightning stuck nearby
(a common occurrence during the summer monsoons) due in part to the long wire out to the mailbox acting as an antenna.
In Rev B hardware (and Rev 2 software), this issue is handled sensing the switch differently to distinguish an open event from EMI.
Moreover, the lightning-caused high-voltage EMI over time caused the input port to go deaf requiring replacement of the MCU.
Rev B hardware insulates the ESP8266 input port from the external line with a diode and a MOSFET;
these parts have much higher voltage tolerances are easier and cheaper to replace than the ESP8266 MCU module.

Note: if you are running the original hardware you may still run Rev 2 software on it.

PCBs may be ordered from PCBs.io at https://PCBs.io/share/49aNb.

## User Interface

The user interface is shown below:

![ready state](https://github.com/Rom3oDelta7/MailNotifier/blob/master/Photos/ready.PNG)

![open notification](https://github.com/Rom3oDelta7/MailNotifier/blob/master/Photos/notification.PNG)

![after mail delivered](https://github.com/Rom3oDelta7/MailNotifier/blob/master/Photos/opened.PNG)

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
Note that the system will reset itself automatically at midnight each day.
* The "REBOOT" slider is used to reboot the system.
To reboot, slide the switch all the way to the right.
You have 5 seconds to change your mind and slide it back to the left.
The "READY" LED will extinguish, a message about canceling the reboot will appear in the DATE and TIME displays,
and the system will reboot after a 5 second delay.
Note that this mechanism can also be used to ensure the end-to-end system is running: if the reboot message comes up
this indicates the system is active - just quickly slide the switch back to the left.

Note that all of the built-in delays and timing heuristics can be easily personalized by changing the C++ preprocessor defines in the sketch.

## Firmware

The C++ code uses the [Blynk] service. 
You must first download the Blynk app for your smartphone (iOS or Android) and sign up for the service.
You can then download this app from within Blynk using the QR code in the "Blynk" folder.

![Blynk mail notifier app QR code](https://github.com/Rom3oDelta7/MailNotifier/blob/master/Blynk/Blynk%20app%20QR%20code.jpg)

See the [Blynk getting started guide].

The code uses the [Blynk Library], my [NTP (Network Time Protocol) clock library], and Paul Stoffregen's [Time Library].
Be sure to change the "auth", "SSID", and "password" strings
and set the email address string to the [address your carrier uses to send an SMS to your phone] in the file "locals.h". 
In the sketch file, set the parameter to the

```C++
NTP_UTC_Timezone()
```

function to your local timezone. 
See ```WorldTimezones.h``` in the [NTP (Network Time Protocol) clock library] for details and a list of timezones you can set.

The code may be updated using OTA (Over The Air) updates.
You can still upload the code in the traditional USB approach, but after the firmware has been uploaded for the first time
you can upload subsequent updates over WiFi. 
See the [ESP8266 OTA update documentation] for details.

__Important__: access to the current time is fundamental to the operation of the system.
Therefore, you must have Internet access for the NTP library to work. 

## Enclosure

![enclosure](https://github.com/Rom3oDelta7/MailNotifier/blob/master/Photos/enclosure.jpg)

The STL files to 3D print the enclosure are included in the repo:
one file for the base and one for the lid.
The wiring I piggybacked on to get to my mailbox happens to come into a patch panel,
so I have an RJ45 jack opening in my enclosure, which is what you see in the picture. 
In your slicing software you should turn on support for the RJ45 opening.
Just about any filament is fine - I used PLA with a layer height of 0.4mm.

## Implementation

### Assembly

![enclosure](https://github.com/Rom3oDelta7/MailNotifier/blob/master/Photos/assembled.jpg)

* 3D print the enclosure base and lid.
* Assemble the discrete components to the PCB.
* Solder the female headers that come with the WeMOS to the PCB.
* If using the RJ45 connector, punch down the switch connection wires into the RJ-45 keystone connector, leaving enough wire to comfortably reach the PCB.
Then insert the RJ-45 into the case.
* Solder the connection for the LED and use some 2mm shrink wrap tubing to protect the leads from shorting
(or electrical tape if you don't have this).
* Mount the LED.
The hole is sized for a 3mm LED. If the fit is too loose, then secure the LED to the case with some UV-curable resin (preferred) or hot melt glue.
If you don't use a green LED, you should recalculate the value of the current limiting resistor to match the Vf of your LED.
If you don't know what this means, just use a green LED.
* Solder the power wires to the DC power jack and fasten to the case.
* Solder the wires to the PCB: power, switch, and LED, being sure to observe the correct polarity (marked on the PCB).
* Insert the WeMOS into the headers.
__Note:__ You can mount the PCB to the enclosure now, but you will probably find it easier to program it first (see below).
* Fasten the board to the mounting post in the base of the enclosure.


### Programming

* Ensure the aforementioned local/personal changes have been implemented in the sketch (.ino) file and "locals.h".
* For the first upload, connect a microUSB plug to the WeMOS.
* In the Arduino IDE, select the "LOLIN(WeMos)D1 R2 & Mini" board.
* Board settings:

|Parameter|Setting|
|---|---|
|CPU Frequency|160 MHz|
|Flash Size|4M(1M SPIFFS)|
|Upload SPeed|921600|
|Port|_your USB serial port_|

* The WeMOS has an automatic system for enabling the flash sequence so no buttons need to be pressed.
* After the code is uploaded, the green LED will illuminate when all of the setup and initialization steps are complete.
* The USB cable may now be removed.
Subsequent firmware uploads can be done using the wireless OTA mechanism.
* At this point, you can launch the Blynk app and run the notifier app dashboard.

# Copyright

Copyright 2018 Rob Redford.
This work is licensed under the Creative Commons Attribution-ShareAlike 4.0 International License.
To view a copy of this license, visit [CC-BY-SA].


[Blynk]: http://www.blynk.cc/
[Blynk Library]: http://www.blynk.cc/getting-started/
[Blynk getting started guide]: http://www.blynk.cc/getting-started/
[NTP (Network Time Protocol) clock library]: https://github.com/Rom3oDelta7/NTP-RTC
[Time Library]: http://www.pjrc.com/teensy/td_libs_Time.html
[ESP8266 OTA update documentation]: https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html
[address your carrier uses to send an SMS to your phone]: http://www.digitaltrends.com/mobile/how-to-send-e-mail-to-sms-text/
[CC-BY-SA]: https://creativecommons.org/licenses/by-sa/4.0/


