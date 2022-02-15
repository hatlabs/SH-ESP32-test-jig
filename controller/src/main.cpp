#include <Arduino.h>

#define SDA_PIN 16
#define SCL_PIN 17
#define ONEWIRE_DQ_PIN 4
#define OPTO_OUT_PIN 33
#define ESP32_CAN_RX_PIN GPIO_NUM_34
#define ESP32_CAN_TX_PIN GPIO_NUM_32

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <N2kMessages.h>
#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object
#include <Wire.h>

#include "ReactESP.h"
#include "SensESP.h"
#include "sensesp/sensors/digital_input.h"
#include "sensesp/system/lambda_consumer.h"
#include "sensesp/system/startable.h"

using namespace sensesp;

#define SDA_TEST_PIN 14
#define SCL_TEST_PIN 12
#define SDA_PIN 16
#define SCL_PIN 17
#define DQ_PIN 4
#define OPTO_IN_PIN 35
#define OPTO_OUT_PIN 33

const int sda_test_expected_freq = 100;
const int scl_test_expected_freq = 141;
const int dq_expected_freq = 173;
const int opto_in_expected_freq = 217;

int sda_test_freq = 0;
int scl_test_freq = 0;
int dq_freq = 0;
int opto_in_freq = 0;

float cabin_temperature = 0;

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

TwoWire* i2c;
Adafruit_SSD1306* display;

void handle_nmea2000_messages(const tN2kMsg& message) {
  unsigned char SID;
  unsigned char instance;
  tN2kTempSource source;
  double actual;
  double setpoint_temperature;

  if (message.PGN == 130316) {
    if (ParseN2kTemperatureExt(message, SID, instance, source, actual,
                               setpoint_temperature)) {
      if (source == N2kts_MainCabinTemperature) {
        cabin_temperature = actual;
      }
    }
  }
}

bool assert_int_almost_equal(int actual, int expected, int tol, String name) {
  if ((expected - tol < actual) && (actual < expected + tol)) {
    Serial.printf("%s OK: %d ~= %d\n", name.c_str(), actual, expected);
    return true;
  } else {
    Serial.printf("%s invalid: %d != %d\n", name.c_str(), actual, expected);
    return false;
  }
}

ReactESP app;

void setup() {
  // setup serial output
  Serial.begin(115200);
  delay(100);

  // toggle the OPTO_OUT pin at the rate of 217 Hz
  pinMode(OPTO_OUT_PIN, OUTPUT);
  app.onRepeatMicros(1e6 / 217, []() {
    static bool state = false;
    digitalWrite(OPTO_OUT_PIN, state);
    state = !state;
  });

  // monitor the SDA pin change frequency
  auto* sda_counter =
      new DigitalInputCounter(SDA_TEST_PIN, INPUT, CHANGE, 1000);
  auto sda_reporter =
      new LambdaConsumer<int>([](int input) { sda_test_freq = input; });
  sda_counter->connect_to(sda_reporter);

  // monitor the SCL pin change frequency
  auto* scl_counter =
      new DigitalInputCounter(SCL_TEST_PIN, INPUT, CHANGE, 1000);
  auto scl_reporter =
      new LambdaConsumer<int>([](int input) { scl_test_freq = input; });
  scl_counter->connect_to(scl_reporter);

  // monitor the DQ pin change frequency
  auto* dq_counter = new DigitalInputCounter(DQ_PIN, INPUT, CHANGE, 1000);
  auto dq_reporter =
      new LambdaConsumer<int>([](int input) { dq_freq = input; });
  dq_counter->connect_to(dq_reporter);

  // monitor the OPTO_IN pin change frequency
  auto* opto_in_counter =
      new DigitalInputCounter(OPTO_IN_PIN, INPUT, CHANGE, 1000);
  auto opto_in_reporter =
      new LambdaConsumer<int>([](int input) { opto_in_freq = input; });
  opto_in_counter->connect_to(opto_in_reporter);

  // toggle the LED pin at rate of 2 Hz
  pinMode(LED_BUILTIN, OUTPUT);
  app.onRepeatMicros(1e6 / 2, []() {
    static bool state = false;
    digitalWrite(LED_BUILTIN, state);
    state = !state;
  });

  // input the NMEA 2000 messages

  // Reserve enough buffer for sending all messages. This does not work on small
  // memory devices like Uno or Mega
  NMEA2000.SetN2kCANSendFrameBufSize(250);
  // Set Product information
  NMEA2000.SetProductInformation(
      "20210307",                      // Manufacturer's Model serial code
      103,                             // Manufacturer's product code
      "SH-ESP32 Test Jig Controller",  // Manufacturer's Model ID
      "0.1.0.1 (2021-03-07)",          // Manufacturer's Software version code
      "0.0.3.1 (2021-03-07)"           // Manufacturer's Model version
  );
  // Set device information
  NMEA2000.SetDeviceInformation(
      1,    // Unique number. Use e.g. Serial number.
      130,  // Device function=Analog to NMEA 2000 Gateway. See codes on
            // http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
      10,   // Device class=Inter/Intranetwork Device. See codes on
           // http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
      2046  // Just choosen free from code list on
            // http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
  );

  NMEA2000.SetMode(tNMEA2000::N2km_NodeOnly, 22);
  NMEA2000.EnableForward(false);  // Disable all msg forwarding to USB (=Serial)
  NMEA2000.SetMsgHandler(handle_nmea2000_messages);
  NMEA2000.Open();

  // No need to parse the messages at every single loop iteration; 1 ms will do
  app.onRepeat(1, []() { NMEA2000.ParseMessages(); });

  // initialize the display
  i2c = new TwoWire(0);
  i2c->begin(SDA_PIN, SCL_PIN);
  display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, i2c, -1);
  if (!display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  delay(100);
  display->setRotation(2);
  display->clearDisplay();
  display->display();

  // update results

  app.onRepeat(1000, []() {
    bool all_ok = true;
    display->clearDisplay();
    display->setTextSize(1);
    display->setCursor(0, 0);
    display->setTextColor(SSD1306_WHITE);
    Serial.printf("Uptime: %lu\n", micros() / 1000000);
    display->printf("Uptime: %lu\n", micros() / 1000000);
    if (!assert_int_almost_equal(sda_test_freq, sda_test_expected_freq, 10,
                                 "SDA")) {
      all_ok = false;
      display->printf("SDA: %d != %d\n", sda_test_freq, sda_test_expected_freq);
    }
    if (!assert_int_almost_equal(scl_test_freq, scl_test_expected_freq, 10,
                                 "SCL")) {
      all_ok = false;
      display->printf("SCL: %d != %d\n", scl_test_freq, scl_test_expected_freq);
    }
    if (!assert_int_almost_equal(dq_freq, dq_expected_freq, 10, "DQ")) {
      all_ok = false;
      display->printf("DQ: %d != %d\n", dq_freq, dq_expected_freq);
    }
    if (!assert_int_almost_equal(opto_in_freq, opto_in_expected_freq, 10,
                                 "OPTO_IN")) {
      all_ok = false;
      display->printf("OPTO_IN: %d != %d\n", opto_in_freq,
                      opto_in_expected_freq);
    }
    if (abs(cabin_temperature - (273.15 + 24.5)) > 0.1) {
      all_ok = false;
      display->printf("CAN bus: %f\n", cabin_temperature);
    }

    if (all_ok) {
      display->println("");
      display->setTextSize(3);
      display->println("OK!");
    }
    display->display();

    // finally, clear the old values
    sda_test_freq = 0;
    scl_test_freq = 0;
    dq_freq = 0;
    opto_in_freq = 0;
    cabin_temperature = 0;
  });

  // enable all object that need enabling
  Startable::start_all();
}

void loop() {
  app.tick();
}
