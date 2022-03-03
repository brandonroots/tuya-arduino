# Tuya-Arduino
Inspired by the work of folks on [Tuya-Convert](https://github.com/ct-Open-Source/tuya-convert), this guide takes a hardware approach to flashing Tuya IoT modules with custom firmware. 


# Tuya

A Chinese company named Tuya offers a free-to-brand turnkey smart home solution to anyone. Using their offer is simple, since everything can be done by clicking through the [Tuya web page](https://en.tuya.com/), from choosing your pre-designed products or pre-programmed wifi-modules (mostly ESP8266) to build your own app. In the end, this has resulted in over 11,000 devices 'made' by over 10,000 vendors using Tuya's firmware and cloud services.

While [Tuya-Convert](https://github.com/ct-Open-Source/tuya-convert) takes an entirely software oriented approach to reflashing firmware on Tuya modules the company has made this increasingly more difficult through firmware updates. In response this guide provides steps involved in hardware flashing a Tuya module through the Arduino IDE after some simple hardware modifications, which should work regardless of Tuya firmware version.


# Why

I wanted access to granular data about my energy use and an option to export that data from a smart plug. 

Amazon is flooded with affordable smart plugs using Tuya modules and they are incredibly cheap, however at the time of this writing the rebranded Tuya apps and [Smart Life](https://ifttt.com/smartlife) IFTTT service do not provide an option to export power consumption data. End users are locked into using Tuya services and at the mercy of what appear to be fly by night companies to contiue providing updates to the apps for their rebranded Tuya devices to stay operational. 

## 🚨WARNING🚨
Please be sure that you understand what you're doing before using this guide and software. Flashing an alternative firmware and modifying hardware can lead to unexpected behavior and/or render the device unusable so that it might be permanently damaged.


# The Hardware

## Adding a serial port

<img alt="TopGreener Smart Plug" src="https://brandonroots.com/wp-content/uploads/2021/03/TopGreener_SmartSocket_00001-1536x2048.jpg" width="500">

A peek inside one of my Smart Plugs revealed a [TYWE2S module](https://developer.tuya.com/en/docs/iot/device-development/module/wifi-module/we-series-module/wifie2smodule?id=K9605u79tgxug) operating as the main microcontroller for the device. This is itself an ESP8266/ESP8265 WiFi module which is easily programmed using the Arduino IDE. Further research pointed me to a GitHub effort called Tuya-Convert as a means to upload custom firmware OTA to these generic modules, however it appears from recent forum posts that a vendor firmware update has broken this technique. So instead I went to the hardware hacking approach.

[Forum posts here from user PeteKnight](https://community.blynk.cc/t/alternative-to-sonoff-s20-eu-type-f-smart-socket/23318) outline the steps to reprogramming the TYWE2S module and provide a useful starting point with Arduino code for OTA updates.

<img alt="TYWE2S diagram" src="https://brandonroots.com/wp-content/uploads/2021/03/TYWE2S.png" width="500">

Referencing the diagram pinout above for the TYWE2S my first step was soldering a header to the 3.3V, GND, RX, and TX pins.

<img alt="Header attached to TYWE2S pins" src="https://brandonroots.com/wp-content/uploads/2021/03/TopGreener_SmartSocket_00009-2048x1625.jpg" width="500">

Next I soldered a small push button switch directly on the TYWE2S module between GND and the contact 100, which when pressed during boot will put the ESP module into programming mode.

<img alt="Button attached to TYWE2S for flashing" src="https://brandonroots.com/wp-content/uploads/2021/03/TopGreener_SmartSocket_00012-1536x2048.jpg" width="500">

Usually efforts like this take some troubleshooting but reprogramming the board through a USB to TTL connector with the Arduino IDE went smoothly (just be sure not to have the Smart Plug connected to mains power!).

<img alt="TYWE2S header connected to USB to TTL connector to flash with Arduino IDE" src="https://brandonroots.com/wp-content/uploads/2021/03/TopGreener_SmartSocket_00014-2048x1536.jpg" width="500">

The Arduino code uses a few libraries you will need to download that are available in the Arduino Libraries Manager including [ESP8266Wifi](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi), [Blynk](https://github.com/blynkkk/blynk-library) (from [vshymanskyy](https://github.com/vshymanskyy)) or [Adafruit_IO_Arduino](https://github.com/adafruit/Adafruit_IO_Arduino), and [HLW8012](https://github.com/xoseperez/hlw8012) (from [xoseperez](https://github.com/xoseperez/hlw8012/commits?author=xoseperez)).

<img alt="View of HLW8012 module" src="https://brandonroots.com/wp-content/uploads/2021/03/TopGreener_SmartSocket_00021-2048x1536.jpg" width="500">

An unmarked IC in the Smart Plug is responsible for measuring power use and communicating that to the ESP module. My best guess is that this is an [HLW8012](https://tinkerman.cat/post/hlw8012-ic-new-sonoff-pow/) based on other user comments and the pinout. Modifying the custom firmware to include code from [HLW8012](https://github.com/xoseperez/hlw8012) (from [xoseperez](https://github.com/xoseperez/hlw8012/commits?author=xoseperez)) seemed to confirm this, though the same code mentioned 230V mains and returned readings with half of the expected wattage. As a quick fix I adjusted the value for VOLTAGE_RESISTOR_UPSTREAM to match results from an unmodified TopGreener Smart Outlet.


# Services

To control the device and store data I initially made use of Blynk.io. It is certainly possible to setup a custom web server and by no means is it a requirement to use this service but it helped to make this process a heck of a lot easier. I have also included Arduino code and instructions for connecting to Adafruit IO.


## Blynk

After downloading the Blynk smartphone app and providing an email address to create an account I was able to create a “New Project” which provides a unique Authentication Token over email to place in the Arduino code.

<img alt="Blynk IO App" src="https://brandonroots.com/wp-content/uploads/2021/03/Blynk_Socket1_00008-1-946x2048.png" width="500">
<img alt="Blynk IO New Project Window" src="https://brandonroots.com/wp-content/uploads/2021/03/Blynk_Socket1_00009-2-946x2048.png" width="500">

From here it is a matter of adding in various widgets from the “Widget Box” including categories such as controllers, displays, notifications, device management, etc.

<img alt="Blynk IO App Smart Socket Readings" src="https://brandonroots.com/wp-content/uploads/2021/03/Blynk_Socket1_00010-946x2048.png" width="500">
<img alt="Blynk IO App Widget Box" src="https://brandonroots.com/wp-content/uploads/2021/03/Blynk_Socket1_00011-946x2048.png" width="500">

I set mine up to include a “Button” to control power (ON/OFF) linked to Virtual Pin 1 in the Arduino Code and two displays for Virtual Pin 5 where the Arduino Code is sharing real time wattage: both a “Labeled Display” to show current wattage and a “SuperChart” to record the real time data which is conveniently available to download as a CSV file.

<img alt="Blynk IO HTTP RESTful API" src="https://brandonroots.com/wp-content/uploads/2021/03/Blynk-Restful-API-2048x1189.png" width="500">


## Adafruit IO
🚧Under Construction🚧

# Results

<img alt="Blynk IO Screenshot" src="https://brandonroots.com/wp-content/uploads/2021/03/Blynk_Socket1_00007-2048x946.png" width="500">
