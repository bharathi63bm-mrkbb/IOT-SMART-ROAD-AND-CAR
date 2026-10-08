#include <WiFi.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================= WIFI =================

const char WIFI_SSID[] = "APPA";
const char WIFI_PASSWORD[] = "12345678";

// ================= ARDUINO CLOUD =================

const char DEVICE_LOGIN_NAME[] = "41b9eded-4dd3-4a6b-ae5e-a0ada38b7a91";
const char DEVICE_KEY[] = "OAzI?65ohB5x8WWcd9jqtFUFX";

WiFiConnectionHandler ArduinoIoTPreferredConnection(WIFI_SSID, WIFI_PASSWORD);

// ================= CLOUD VARIABLES =================

String dayNight;
int freeSlots;
bool slot1Status;
bool slot2Status;

// ================= PARKING =================

#define PARKING_IR1 4
#define PARKING_IR2 5

#define OLED_SDA 8
#define OLED_SCL 9

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= STREET LIGHT =================

#define TRIG1 10
#define ECHO1 11

#define TRIG2 12
#define ECHO2 13

#define NIGHT_IR 14

#define ROAD1_LED1 15
#define ROAD1_LED2 16

#define ROAD2_LED1 17
#define ROAD2_LED2 18

#define ROAD1_LED1_DISTANCE 12
#define ROAD1_LED2_DISTANCE 20

#define ROAD2_LED1_DISTANCE 10
#define ROAD2_LED2_DISTANCE 18

// ================= PARKING SENSOR =================

int readParkingSensor(int pin) {

  int lowCount = 0;
  int highCount = 0;

  for (int i = 0; i < 10; i++) {

    int value = digitalRead(pin);

    if (value == LOW)
      lowCount++;
    else
      highCount++;

    vTaskDelay(2 / portTICK_PERIOD_MS);
  }

  if (lowCount > highCount)
    return LOW;

  return HIGH;
}

// ================= ULTRASONIC =================

float getDistance(int trig, int echo) {

  digitalWrite(trig, LOW);
  delayMicroseconds(2);

  digitalWrite(trig, HIGH);
  delayMicroseconds(10);

  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, 30000);

  if (duration == 0)
    return 999;

  return duration * 0.0343 / 2;
}

// ================= ROAD 1 =================

void controlRoad1(float distance) {

  if (distance <= ROAD1_LED1_DISTANCE) {

    digitalWrite(ROAD1_LED1, HIGH);
    digitalWrite(ROAD1_LED2, LOW);

  }
  else if (distance <= ROAD1_LED2_DISTANCE) {

    digitalWrite(ROAD1_LED1, LOW);
    digitalWrite(ROAD1_LED2, HIGH);

  }
  else {

    digitalWrite(ROAD1_LED1, LOW);
    digitalWrite(ROAD1_LED2, LOW);
  }
}

// ================= ROAD 2 =================

void controlRoad2(float distance) {

  if (distance <= ROAD2_LED1_DISTANCE) {

    digitalWrite(ROAD2_LED1, HIGH);
    digitalWrite(ROAD2_LED2, LOW);

  }
  else if (distance <= ROAD2_LED2_DISTANCE) {

    digitalWrite(ROAD2_LED1, LOW);
    digitalWrite(ROAD2_LED2, HIGH);

  }
  else {

    digitalWrite(ROAD2_LED1, LOW);
    digitalWrite(ROAD2_LED2, LOW);
  }
}

// ================= ALL LIGHTS OFF =================

void allLightsOff() {

  digitalWrite(ROAD1_LED1, LOW);
  digitalWrite(ROAD1_LED2, LOW);

  digitalWrite(ROAD2_LED1, LOW);
  digitalWrite(ROAD2_LED2, LOW);
}

// ================= CLOUD PROPERTIES =================

void initProperties() {

  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);

  ArduinoCloud.addProperty(dayNight, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(freeSlots, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(slot1Status, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(slot2Status, READ, ON_CHANGE, NULL);
}

// ================= PARKING TASK =================

void parkingTask(void *parameter) {

  bool occupied1 = false;
  bool occupied2 = false;

  int lastSensor1 = HIGH;
  int lastSensor2 = HIGH;

  int stableCount1 = 0;
  int stableCount2 = 0;

  while (true) {

    int sensor1 = readParkingSensor(PARKING_IR1);
    int sensor2 = readParkingSensor(PARKING_IR2);

    if (sensor1 == lastSensor1)
      stableCount1++;
    else {
      lastSensor1 = sensor1;
      stableCount1 = 0;
    }

    if (sensor2 == lastSensor2)
      stableCount2++;
    else {
      lastSensor2 = sensor2;
      stableCount2 = 0;
    }

    if (stableCount1 >= 3) {

      occupied1 = (sensor1 == LOW);
      stableCount1 = 0;
    }

    if (stableCount2 >= 3) {

      occupied2 = (sensor2 == LOW);
      stableCount2 = 0;
    }

    int freeSlotCount = 0;

    if (!occupied1)
      freeSlotCount++;

    if (!occupied2)
      freeSlotCount++;

    slot1Status = occupied1;
    slot2Status = occupied2;
    freeSlots = freeSlotCount;

    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("PARKING");

    display.setTextSize(1);

    display.setCursor(0, 22);
    display.print("SLOT 1: ");

    if (occupied1)
      display.println("FULL");
    else
      display.println("FREE");

    display.setCursor(0, 34);
    display.print("SLOT 2: ");

    if (occupied2)
      display.println("FULL");
    else
      display.println("FREE");

    display.setCursor(0, 48);
    display.print("FREE SLOTS: ");
    display.println(freeSlotCount);

    display.display();

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// ================= STREET LIGHT TASK =================

void streetLightTask(void *parameter) {

  while (true) {

    int nightStatus = digitalRead(NIGHT_IR);

    if (nightStatus == LOW) {

      dayNight = "DAY";

      allLightsOff();
    }
    else {

      dayNight = "NIGHT";

      float distance1 = getDistance(TRIG1, ECHO1);

      delay(50);

      float distance2 = getDistance(TRIG2, ECHO2);

      controlRoad1(distance1);
      controlRoad2(distance2);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// ================= SETUP =================

void setup() {

  Serial.begin(115200);

  delay(1500);

  // Parking

  pinMode(PARKING_IR1, INPUT);
  pinMode(PARKING_IR2, INPUT);

  // OLED

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

    Serial.println("OLED FAILED");

    while (true);
  }

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);

  display.println("PARKING");

  display.setTextSize(1);
  display.setCursor(0, 25);

  display.println("SYSTEM STARTING...");

  display.display();

  delay(1000);

  // Street lights

  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);

  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);

  pinMode(NIGHT_IR, INPUT);

  pinMode(ROAD1_LED1, OUTPUT);
  pinMode(ROAD1_LED2, OUTPUT);

  pinMode(ROAD2_LED1, OUTPUT);
  pinMode(ROAD2_LED2, OUTPUT);

  allLightsOff();

  // Arduino Cloud

  initProperties();

  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  setDebugMessageLevel(2);

  ArduinoCloud.printDebugInfo();

  // Tasks

  xTaskCreatePinnedToCore(
    parkingTask,
    "Parking Task",
    4096,
    NULL,
    1,
    NULL,
    0
  );

  xTaskCreatePinnedToCore(
    streetLightTask,
    "Street Light Task",
    4096,
    NULL,
    1,
    NULL,
    1
  );

  Serial.println("DUAL CORE SMART SYSTEM STARTED");
}

// ================= LOOP =================

void loop() {

  ArduinoCloud.update();

  delay(10);
}