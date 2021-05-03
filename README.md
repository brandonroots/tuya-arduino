# Tuya-Arduino
Inspired by the work of folks on [Tuya-Convert](https://github.com/ct-Open-Source/tuya-convert), this guide takes a more direct approach to hardware hacking and flashing Tuya IoT modules. 

# Tuya

A Chinese company named Tuya offers a free-to-brand turnkey smart home solution to anyone. Using their offer is dead-simple, since everything can be done by clicking through the [Tuya web page](https://en.tuya.com/), from choosing your pre-designed products or pre-programmed wifi-modules (mostly ESP8266) to build your own app. In the end, this has resulted in over 11,000 devices 'made' by over 10,000 vendors using Tuya's firmware and cloud services.

While [Tuya-Convert](https://github.com/ct-Open-Source/tuya-convert) takes an entirely software oriented approach to reflashing firmware on Tuya modules the company has made this increasingly more difficult through firmware updates. In response this guide provides steps involved in hardware flashing a Tuya module through the Arduino IDE, which should work regardless of Tuya firmware version.

## 🚨WARNING🚨
Please be sure that you understand what you're doing before using this software. Flashing an alternative firmware can lead to unexpected behavior and/or render the device unusable, so that it might be permanently damaged (highly unlikely) or require soldering a serial connection to the processor in order to reflash it (likely).
