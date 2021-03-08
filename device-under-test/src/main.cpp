#include <Arduino.h>

#define SDA_PIN 16
#define SCL_PIN 17
#define ONEWIRE_DQ_PIN 4
#define OPTO_OUT_PIN 33
#define ESP32_CAN_RX_PIN GPIO_NUM_34
#define ESP32_CAN_TX_PIN GPIO_NUM_32

#include <N2kMessages.h>
#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object

#include "ReactESP.h"

#define SDA_PIN 16
#define SCL_PIN 17
#define DQ_PIN 4
#define OPTO_IN_PIN 35
#define OPTO_OUT_PIN 33

volatile bool opto_in_change;

ReactESP app([]() {
  // setup serial output
  Serial.begin(115200);
  delay(100);

  // toggle the SDA pin at the rate of 100 Hz
  pinMode(SDA_PIN, OUTPUT);
  app.onRepeatMicros(1e6/100, []() {
    static bool state = false;
    digitalWrite(SDA_PIN, state);
    state = !state;
  });
  
  // toggle the SCL pin at the rate of 141 Hz
  pinMode(SCL_PIN, OUTPUT);
  app.onRepeatMicros(1e6/141, []() {
    static bool state = false;
    digitalWrite(SCL_PIN, state);
    state = !state;
  });
  
  // toggle the DQ pin at the rate of 141 Hz
  pinMode(DQ_PIN, OUTPUT);
  app.onRepeatMicros(1e6/173, []() {
    static bool state = false;
    digitalWrite(DQ_PIN, state);
    state = !state;
  });
  
  // toggle the LED pin at the rate of 4 Hz
  pinMode(LED_BUILTIN, OUTPUT);
  app.onRepeatMicros(1e6/4, []() {
    static bool state = false;
    digitalWrite(LED_BUILTIN, state);
    state = !state;
  });  

  // make OPTO_OUT_PIN the boolean inverse of OPTO_IN_PIN
  pinMode(OPTO_IN_PIN, INPUT);
  app.onInterrupt(OPTO_IN_PIN, CHANGE, []() {
    opto_in_change = true;
  });

  pinMode(OPTO_OUT_PIN, OUTPUT);
  app.onTick([](){
    if (opto_in_change) {
      opto_in_change = false;
      digitalWrite(OPTO_OUT_PIN, !digitalRead(OPTO_IN_PIN));
    }
  });

  // Reserve enough buffer for sending all messages.
  NMEA2000.SetN2kCANSendFrameBufSize(250);
  // Set Product information
  NMEA2000.SetProductInformation(
      "20210307",              // Manufacturer's Model serial code
      102,                     // Manufacturer's product code
      "SH-ESP32 Test Jig DUT",       // Manufacturer's Model ID
      "0.1.0.1 (2021-03-07)",  // Manufacturer's Software version code
      "0.0.3.1 (2021-03-07)"   // Manufacturer's Model version
  );
  // Set device information
  NMEA2000.SetDeviceInformation(
      1,    // Unique number. Use e.g. Serial number.
      132,  // Device function=Analog to NMEA 2000 Gateway. See codes on
            // http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
      25,   // Device class=Inter/Intranetwork Device. See codes on
           // http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
      2046  // Just choosen free from code list on
            // http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
  );

  NMEA2000.SetMode(tNMEA2000::N2km_NodeOnly, 22);
  NMEA2000.EnableForward(false);  // Disable all msg forwarding to USB (=Serial)
  NMEA2000.Open();

  // No need to parse the messages at every single loop iteration; 1 ms will do
  app.onRepeat(1, []() { NMEA2000.ParseMessages(); });

  app.onRepeat(100, []() {
    tN2kMsg N2kMsg;
    SetN2kTemperatureExt(N2kMsg, 1, 1, N2kts_MainCabinTemperature, 273.15 + 24.5);
    NMEA2000.SendMsg(N2kMsg);
  });
});
