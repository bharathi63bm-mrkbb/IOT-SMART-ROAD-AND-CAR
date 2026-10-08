#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

const char DEVICE_LOGIN_NAME[] = "81c30d14-aaa9-4ea6-9933-ab38ce06a86b";

const char SSID[] = "APPA";
const char PASS[] = "MRKB@123";
const char DEVICE_KEY[] = "IxDpQOS@bRb!q0#xCE8FrAqwy";

String currentGreenSystem;
bool ambulanceDetected;
bool pedestrainLeftToRight;
bool pedestrainRightToLeft;

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

#define N_GREEN 4
#define N_ORANGE 5
#define N_RED 6

#define E_GREEN 7
#define E_ORANGE 15
#define E_RED 16

#define S_GREEN 17
#define S_ORANGE 18
#define S_RED 21

#define W_GREEN 1
#define W_ORANGE 2
#define W_RED 3

#define MIC1 38
#define MIC2 39

#define SIDE1_GREEN 8
#define SIDE1_RED 9

#define SIDE2_GREEN 10
#define SIDE2_RED 11

#define BUTTON1 12
#define BUTTON2 13

#define BUZZER 14

#define GREEN_TIME 10000
#define ORANGE_TIME 2000
#define EMERGENCY_HOLD 5000

int trafficState = 0;

unsigned long trafficStart = 0;
unsigned long lastSound = 0;

void initProperties() {

  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);

  ArduinoCloud.addProperty(currentGreenSystem, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(ambulanceDetected, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(pedestrainLeftToRight, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(pedestrainRightToLeft, READ, ON_CHANGE, NULL);
}

void trafficAllOff() {

  digitalWrite(N_GREEN, LOW);
  digitalWrite(N_ORANGE, LOW);
  digitalWrite(N_RED, LOW);

  digitalWrite(E_GREEN, LOW);
  digitalWrite(E_ORANGE, LOW);
  digitalWrite(E_RED, LOW);

  digitalWrite(S_GREEN, LOW);
  digitalWrite(S_ORANGE, LOW);
  digitalWrite(S_RED, LOW);

  digitalWrite(W_GREEN, LOW);
  digitalWrite(W_ORANGE, LOW);
  digitalWrite(W_RED, LOW);
}

void northSouthGreen() {

  trafficAllOff();

  digitalWrite(N_GREEN, HIGH);
  digitalWrite(S_GREEN, HIGH);

  digitalWrite(E_RED, HIGH);
  digitalWrite(W_RED, HIGH);

  currentGreenSystem = "SYSTEM 1 - NORTH + SOUTH";
}

void northSouthOrange() {

  trafficAllOff();

  digitalWrite(N_ORANGE, HIGH);
  digitalWrite(S_ORANGE, HIGH);

  digitalWrite(E_RED, HIGH);
  digitalWrite(W_RED, HIGH);

  currentGreenSystem = "SYSTEM 1 - NORTH + SOUTH ORANGE";
}

void eastWestGreen() {

  trafficAllOff();

  digitalWrite(E_GREEN, HIGH);
  digitalWrite(W_GREEN, HIGH);

  digitalWrite(N_RED, HIGH);
  digitalWrite(S_RED, HIGH);

  currentGreenSystem = "SYSTEM 2 - EAST + WEST";
}

void eastWestOrange() {

  trafficAllOff();

  digitalWrite(E_ORANGE, HIGH);
  digitalWrite(W_ORANGE, HIGH);

  digitalWrite(N_RED, HIGH);
  digitalWrite(S_RED, HIGH);

  currentGreenSystem = "SYSTEM 2 - EAST + WEST ORANGE";
}

void emergencyMode() {

  trafficAllOff();

  digitalWrite(E_GREEN, HIGH);
  digitalWrite(W_GREEN, HIGH);

  digitalWrite(N_RED, HIGH);
  digitalWrite(S_RED, HIGH);

  currentGreenSystem = "SYSTEM 2 - EAST + WEST - AMBULANCE";
}

void trafficTask(void *parameter) {

  trafficStart = millis();

  while (true) {

    bool sound1 = digitalRead(MIC1) == HIGH;
    bool sound2 = digitalRead(MIC2) == HIGH;

    bool soundDetected = sound1 || sound2;

    if (soundDetected) {

      ambulanceDetected = true;

      lastSound = millis();

      emergencyMode();

      Serial.println("AMBULANCE SOUND DETECTED");

      vTaskDelay(50 / portTICK_PERIOD_MS);

      continue;
    }

    if (lastSound != 0 &&
        millis() - lastSound < EMERGENCY_HOLD) {

      ambulanceDetected = true;

      emergencyMode();

      vTaskDelay(50 / portTICK_PERIOD_MS);

      continue;
    }

    ambulanceDetected = false;

    unsigned long elapsed = millis() - trafficStart;

    if (trafficState == 0) {

      northSouthGreen();

      if (elapsed >= GREEN_TIME) {

        trafficState = 1;
        trafficStart = millis();
      }
    }

    else if (trafficState == 1) {

      northSouthOrange();

      if (elapsed >= ORANGE_TIME) {

        trafficState = 2;
        trafficStart = millis();
      }
    }

    else if (trafficState == 2) {

      eastWestGreen();

      if (elapsed >= GREEN_TIME) {

        trafficState = 3;
        trafficStart = millis();
      }
    }

    else if (trafficState == 3) {

      eastWestOrange();

      if (elapsed >= ORANGE_TIME) {

        trafficState = 0;
        trafficStart = millis();
      }
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void normalPedestrian() {

  digitalWrite(SIDE1_GREEN, HIGH);
  digitalWrite(SIDE2_GREEN, HIGH);

  digitalWrite(SIDE1_RED, LOW);
  digitalWrite(SIDE2_RED, LOW);

  digitalWrite(BUZZER, LOW);

  pedestrainLeftToRight = false;
  pedestrainRightToLeft = false;
}

void pedestrianCrossing(int direction) {

  if (direction == 1) {

    pedestrainLeftToRight = true;
    pedestrainRightToLeft = false;

    Serial.println("PEDESTRIAN LEFT TO RIGHT");
  }

  else {

    pedestrainLeftToRight = false;
    pedestrainRightToLeft = true;

    Serial.println("PEDESTRIAN RIGHT TO LEFT");
  }

  for (int i = 0; i < 5; i++) {

    digitalWrite(SIDE1_GREEN, HIGH);
    digitalWrite(SIDE2_GREEN, HIGH);

    digitalWrite(BUZZER, HIGH);

    vTaskDelay(500 / portTICK_PERIOD_MS);

    digitalWrite(SIDE1_GREEN, LOW);
    digitalWrite(SIDE2_GREEN, LOW);

    digitalWrite(BUZZER, LOW);

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }

  digitalWrite(SIDE1_RED, HIGH);
  digitalWrite(SIDE2_RED, HIGH);

  vTaskDelay(7000 / portTICK_PERIOD_MS);

  for (int i = 0; i < 3; i++) {

    digitalWrite(SIDE1_RED, HIGH);
    digitalWrite(SIDE2_RED, HIGH);

    digitalWrite(BUZZER, HIGH);

    vTaskDelay(500 / portTICK_PERIOD_MS);

    digitalWrite(SIDE1_RED, LOW);
    digitalWrite(SIDE2_RED, LOW);

    digitalWrite(BUZZER, LOW);

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }

  normalPedestrian();

  Serial.println("PEDESTRIAN NORMAL");
}

void pedestrianTask(void *parameter) {

  normalPedestrian();

  while (true) {

    bool button1 = digitalRead(BUTTON1) == HIGH;
    bool button2 = digitalRead(BUTTON2) == HIGH;

    if (button1) {

      pedestrianCrossing(1);

      while (digitalRead(BUTTON1) == HIGH) {
        vTaskDelay(20 / portTICK_PERIOD_MS);
      }
    }

    else if (button2) {

      pedestrianCrossing(2);

      while (digitalRead(BUTTON2) == HIGH) {
        vTaskDelay(20 / portTICK_PERIOD_MS);
      }
    }

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

void setup() {

  Serial.begin(115200);

  pinMode(N_GREEN, OUTPUT);
  pinMode(N_ORANGE, OUTPUT);
  pinMode(N_RED, OUTPUT);

  pinMode(E_GREEN, OUTPUT);
  pinMode(E_ORANGE, OUTPUT);
  pinMode(E_RED, OUTPUT);

  pinMode(S_GREEN, OUTPUT);
  pinMode(S_ORANGE, OUTPUT);
  pinMode(S_RED, OUTPUT);

  pinMode(W_GREEN, OUTPUT);
  pinMode(W_ORANGE, OUTPUT);
  pinMode(W_RED, OUTPUT);

  pinMode(MIC1, INPUT);
  pinMode(MIC2, INPUT);

  pinMode(SIDE1_GREEN, OUTPUT);
  pinMode(SIDE1_RED, OUTPUT);

  pinMode(SIDE2_GREEN, OUTPUT);
  pinMode(SIDE2_RED, OUTPUT);

  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);

  pinMode(BUZZER, OUTPUT);

  trafficAllOff();

  ambulanceDetected = false;
  pedestrainLeftToRight = false;
  pedestrainRightToLeft = false;
  currentGreenSystem = "STARTING";

  initProperties();

  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  xTaskCreatePinnedToCore(
    trafficTask,
    "TrafficTask",
    4096,
    NULL,
    1,
    NULL,
    0
  );

  xTaskCreatePinnedToCore(
    pedestrianTask,
    "PedestrianTask",
    4096,
    NULL,
    1,
    NULL,
    1
  );
}

void loop() {

  ArduinoCloud.update();

  vTaskDelay(10 / portTICK_PERIOD_MS);
}