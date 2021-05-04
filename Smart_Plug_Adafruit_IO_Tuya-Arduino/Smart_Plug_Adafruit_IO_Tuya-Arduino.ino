// SMART SOCKET for Adafruit IO
//
// by Brandon Roots
//
// Written for TOPGREENER Smart Wi-Fi Powerful Plug with Energy Monitoring, Smart Outlet, 15A
// ------------
// Based on Blynk + OTA Code for "W-DE004 WIFI SMART SOCKET" by Pete Kniight
// ------------

// Arduino IDE Upload Settings
// Board:            "Generic ESP8266 Module"
// Flash Mode:       "DOUT"
// Flash Size:       "1M (No SPIFFS)"
// Debug Port:       "Disabled"
// Debug Level:      "None"
// IwIP Variant:     "v1.4 Prebuilt"
// Reset Method:     "ck"                                                                                                                                
// Crystal Frequency "26 MHz"
// Flash Frequency:  "40 MHz"
// CPU Frequency:    "80MHz"
// Upload Speed:     "115200"

// Adafruit IO Project Setup:

// Notes:
// The wifi socket uses GPIO3 for the physical button. This is the hardware TX pin on the ESP8266, so SERIAL OUTPUT FOR DEBUGGING WILL NOT WORK !
// Please do not add Serial.begin() to this sketch ??

// TYWE2S Module Pinout
// Pin 1 --> RST --> I/O --> External Reset
// Pin 2 --> AD --> AI --> ADC Terminal (10-bits SAR ADC)
// Pin 3 --> 13 --> I/O --> GPIO_13
// Pin 4 --> 04 --> I/O --> GPIO_04
// Pin 5 --> 05 --> I/O --> GPIO_05
// Pin 6 --> 3V3 --> P --> Supply Voltage (3.3V)
// Pin 7 --> GND --> P --> Ground
// Pin 8 --> RX --> I/O --> UART0_RXD
// Pin 9 --> TX --> I/O --> UART0_TXD
// Pin 10 --> 12 --> I/O --> GPIO_12

#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include "AdafruitIO_WiFi.h"
#include <BlynkSimpleEsp8266.h>
#include <Ticker.h>                  // Used to flash the LED in non-blocking mode
#include <HLW8012.h>


// WiFi Setup
char OTAhost[] =       "TYWE2S Smart Socket 1"; // Name set here will appear in Arduino IDE
#define WIFI_SSID      "**YOUR WIFI SSID**"
#define WIFI_PASS      "**YOUR WIFI PASSWORD**"

// Adafruit IO
#define IO_USERNAME    "**YOUR ADAFRUIT IO USERNAME**"
#define IO_KEY         "**YOUR ADAFRUIT IO KEY**"

// Connect to Wi-Fi and Adafruit IO handel 
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
 
// Create a feed object that allows us to send data to
AdafruitIO_Feed *wattsFeed = io.feed("watts"); // I named my Adafruit IO feed "watts". Set your feed name here


// Smart Plug
#define RELAY1    14                 // Wi-Fi Switch relay and Blue LED is connected to GPIO 14 (LOW(0)=0ff, HIGH(1)=On)
#define buttonPin  3                 // Wi-Fi Switch pushbutton is connected to GPIO 3 (LOW(0)=Pushed, HIGH(1)=Released)
#define GreenLED  13                 // Wi-Fi Switch Green LED is connected to GPIO 13 (LOW(0)=0n, HIGH(1)=Off)

// HLW8012 Power Sensor
#define CF_PIN     4                 //GPIO4=CF(hlw8012)
#define CF1_PIN    5                 //GPIO5=CF1(hlw8012) 
#define SEL_PIN   12                 //GPIO12=SEL(hlw8012)


int RELAY1_State;                    // Used to track the current Relay state 
int reading = HIGH;                  // Used in the switch debounce routine
int buttonState=HIGH;                // The current button state is released (HIGH)
int lastButtonState = HIGH;          // The last button state is also relesed (HIGH) - Used in debounce routine
unsigned long lastDebounceTime = 0;  // The time in Millis that the output pin was toggled - Used in debounce routine
unsigned long debounceDelay = 20;    // The debounce delay; increase if you get multiple on/offs when the physical button is pressed

Ticker greenticker;                   // Create and instance of Ticker called greenticker
BlynkTimer timer;                    // Create and instance of BlynkTimer called timer


// Check values every 2 seconds
#define UPDATE_TIME                     2000

// Set SEL_PIN to HIGH to sample current
// This is the case for Itead's Sonoff POW, where a
// the SEL_PIN drives a transistor that pulls down
// the SEL pin in the HLW8012 when closed
#define CURRENT_MODE                    HIGH

// These are the nominal values for the resistors in the circuit
// * The CURRENT_RESISTOR is the 1milliOhm copper-manganese resistor in series with the main line
// * The VOLTAGE_RESISTOR_UPSTREAM are the 5 470kOhm resistors in the voltage divider that feeds the V2P pin in the HLW8012
// * The VOLTAGE_RESISTOR_DOWNSTREAM is the 1kOhm resistor in the voltage divider that feeds the V2P pin in the HLW8012
#define CURRENT_RESISTOR                0.001 
#define VOLTAGE_RESISTOR_UPSTREAM       ( 2 * 470000 ) // ( 5 * 470000 ) Real: 2280k
#define VOLTAGE_RESISTOR_DOWNSTREAM     ( 1000 ) // Real 1.009k

HLW8012 hlw8012;

void unblockingDelay(unsigned long mseconds) {
    unsigned long timeout = millis();
    while ((millis() - timeout) < mseconds) delay(1);
}

 
void setup() 
{
  pinMode(RELAY1, OUTPUT);
  pinMode(GreenLED, OUTPUT); 
  pinMode(buttonPin, INPUT);  

  digitalWrite(RELAY1, HIGH);                // Turn the Relay on  
  digitalWrite(GreenLED, HIGH);              // Turn the Green LED off

  greenticker.attach(0.1, greentick);         // start greenticker with 0.1 second flashes to indicate we're trying to connect to WiFi/Blynk
  WiFi.mode(WIFI_STA);
  
  // Connect to Adafruit IO
  io.connect();
 
  // wait for a connection
  while(io.status() < AIO_CONNECTED) 
  {
    delay(500);
  }

  
  greenticker.detach();                      // Stop the greenticker now we're connected
  digitalWrite(GreenLED, HIGH);              // Turn the Green LED Off if it was previoulsy On
  
  ArduinoOTA.onError([](ota_error_t error) { ESP.restart(); });
  ArduinoOTA.setHostname(OTAhost);
  ArduinoOTA.begin();

  timer.setInterval(10L, CheckButtonState); // Timer to check the physical button every 10ms

    
    // Close the relay to switch on the load
    pinMode(RELAY1, OUTPUT);
    digitalWrite(RELAY1, HIGH);

    // Initialize HLW8012
    // void begin(unsigned char cf_pin, unsigned char cf1_pin, unsigned char sel_pin, unsigned char currentWhen = HIGH, bool use_interrupts = false, unsigned long pulse_timeout = PULSE_TIMEOUT);
    // * cf_pin, cf1_pin and sel_pin are GPIOs to the HLW8012 IC
    // * currentWhen is the value in sel_pin to select current sampling
    // * set use_interrupts to false, we will have to call handle() in the main loop to do the sampling
    // * set pulse_timeout to 500ms for a fast response but losing precision (that's ~24W precision :( )
    hlw8012.begin(CF_PIN, CF1_PIN, SEL_PIN, CURRENT_MODE, false, 500000);

    // These values are used to calculate current, voltage and power factors as per datasheet formula
    // These are the nominal values for the Sonoff POW resistors:
    // * The CURRENT_RESISTOR is the 1milliOhm copper-manganese resistor in series with the main line
    // * The VOLTAGE_RESISTOR_UPSTREAM are the 5 470kOhm resistors in the voltage divider that feeds the V2P pin in the HLW8012
    // * The VOLTAGE_RESISTOR_DOWNSTREAM is the 1kOhm resistor in the voltage divider that feeds the V2P pin in the HLW8012
    hlw8012.setResistors(CURRENT_RESISTOR, VOLTAGE_RESISTOR_UPSTREAM, VOLTAGE_RESISTOR_DOWNSTREAM);

}
 
void loop() 
{
  // Always keep this at the top of your main loop
  // While not confirmed, this implies that the Adafruit IO library is not event-driven
  // This means you should refrain from using infinite loops
  io.run();
  ArduinoOTA.handle();
  timer.run();

 
  // HLW8012 Power Monitoring

  static unsigned long last = millis();

    // This UPDATE_TIME should be at least twice the minimum time for the current or voltage
    // signals to stabilize. Experimentally that's about 1 second.
    if ((millis() - last) > UPDATE_TIME) {

        last = millis();

        /*
        Serial.print("[HLW] Active Power (W)    : "); Serial.println(hlw8012.getActivePower());
        Serial.print("[HLW] Voltage (V)         : "); Serial.println(hlw8012.getVoltage());
        Serial.print("[HLW] Current (A)         : "); Serial.println(hlw8012.getCurrent());
        Serial.print("[HLW] Apparent Power (VA) : "); Serial.println(hlw8012.getApparentPower());
        Serial.print("[HLW] Power Factor (%)    : "); Serial.println((int) (100 * hlw8012.getPowerFactor()));
        Serial.println();
        */

        // Send watts reading to our Watts feed
        wattsFeed->save(hlw8012.getActivePower()); // Active Power (W)
        

        // When not using interrupts we have to manually switch to current or voltage monitor
        // This means that every time we get into the conditional we only update one of them
        // while the other will return the cached value.
        hlw8012.toggleMode();

    }

  // DONT SEND A BILLION DATA POINTS! This slows down the program so that it sends readings once every 5 seconds
  delay(5000);
}


void greentick() // Non-blocking ticker for Green LED
{
  //toggle state
  int state = digitalRead(GreenLED);         // get the current state of the GreenLED pin
  digitalWrite(GreenLED, !state);            // set pin to the opposite state
}


void CheckButtonState()
{
  reading = digitalRead(buttonPin);         // read the state of the physical switch
  if (reading != lastButtonState)
  {
    lastDebounceTime = millis();            // Start the debounce timer
  }

  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading != buttonState)             // We get here if the physical button has been in the same state for longer than the debounce delay
    {
      buttonState = reading;

      if (buttonState == LOW)               // only toggle the power if the new button state is LOW (button pressed)
      {
        RELAY1_State = !RELAY1_State;
        if (RELAY1_State==HIGH)             // The physical button press was an instruction to turn the power On
        {
          digitalWrite(RELAY1, HIGH);       // Turn the relay On
          digitalWrite(GreenLED, LOW);       // Turn the LED On
          //Blynk.virtualWrite(V1, 1);        // Turn the App button widget On                    
        }
        else                                // The physical button press was an instruction to turn the power Off
        {
          digitalWrite(RELAY1, RELAY1_State);  // Turn the relay Off
          digitalWrite(GreenLED, HIGH);         // Turn the LED Off
          //Blynk.virtualWrite(V1, 0);           // Turn the App button widget Off 
        }
      }
    }
  }
  lastButtonState = reading;                // Update lastButtonState for use next time around the loop 
}
