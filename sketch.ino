
#include <Keyboard.h>
#include <Joystick.h>
#include "RotaryEncoder.h"
#include <Z906.h>

const int DEBOUNCE_DELAY_MS = 300;
const int PIN_CLK = A2;
const int PIN_DT = A3;
const int PIN_BTN = A4;
const int PIN_LED = A5;
const int PIN_BUTTON_0 = 2;
const int PIN_BUTTON_1 = 3;
const int PIN_BUTTON_2 = 4;
const int PIN_BUTTON_3 = 5;
const int PIN_BUTTON_4 = 6;
const int PIN_BUTTON_5 = 7;
const uint8_t STATUS_STBY = 0x14;

unsigned long buttonLastClickedTimeMs = 0;
unsigned long lastStatusCheckTimeMs = 0;
int lastButtonState[6] = { 0, 0, 0, 0, 0, 0 };
Joystick_ Joystick;
RotaryEncoder encoder(PIN_DT, PIN_CLK);
Z906 LOGI(Serial1);

void setup() {
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_CLK, INPUT_PULLUP);
  pinMode(PIN_DT, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON_0, INPUT_PULLUP);
  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);
  pinMode(PIN_BUTTON_3, INPUT_PULLUP);
  pinMode(PIN_BUTTON_4, INPUT_PULLUP);
  pinMode(PIN_BUTTON_5, INPUT_PULLUP);

  Joystick.begin();
  Serial.begin(9600);
  Serial.println("Starting...");

  while (LOGI.request(VERSION) == 0) {
    Serial.println("Waiting Z906 Power-Up");
    digitalWrite(PIN_LED, HIGH);
    delay(500);
    digitalWrite(PIN_LED, LOW);
    delay(500);
  }

  LOGI.input(SELECT_INPUT_3);
  LOGI.cmd(MUTE_OFF);

  Serial.println("");
  Serial.println("Z906 Version: " + (String)LOGI.request(VERSION));
  Serial.println("Main level: " + (String)LOGI.request(MAIN_LEVEL));
  Serial.println("Rear level: " + (String)LOGI.request(REAR_LEVEL));
  Serial.println("Center level: " + (String)LOGI.request(CENTER_LEVEL));
  Serial.println("Sub level: " + (String)LOGI.request(SUB_LEVEL));
  Serial.println("Running: " + (String)!LOGI.request(STATUS_STBY));
  Serial.println("");
}

void loop() {
  encoder.tick();

  int deltaPos = encoder.getPosition();

  if (deltaPos != 0) {
    if (deltaPos > 0) {
      LOGI.cmd(LEVEL_MAIN_UP);
    } else {
      LOGI.cmd(LEVEL_MAIN_DOWN);
    }
    encoder.setPosition(0);
    Serial.println("Main level: " + (String)LOGI.request(MAIN_LEVEL));
  }

  unsigned long now = millis();

  if ((now - lastStatusCheckTimeMs) > DEBOUNCE_DELAY_MS) {
    int running = LOGI.request(VERSION) != 0 && !LOGI.request(STATUS_STBY);

    if (digitalRead(PIN_LED) != running) {
      Serial.println("Power status changed: " + (String)running);
      digitalWrite(PIN_LED, running);
    }
    lastStatusCheckTimeMs = now;
  }

  if (!digitalRead(PIN_BTN) && (now - buttonLastClickedTimeMs) > DEBOUNCE_DELAY_MS) {
    if (LOGI.request(STATUS_STBY)) {
      LOGI.on();
      LOGI.input(SELECT_INPUT_3);
      LOGI.cmd(MUTE_OFF);
      Serial.println("Power on");
    } else {
      LOGI.off();
      Serial.println("Power off");
    }
    buttonLastClickedTimeMs = now;
  }

  for (int buttonIndex = 0; buttonIndex < 6; buttonIndex++) {
    int currentButtonState = !digitalRead(PIN_BUTTON_0 + buttonIndex);

    if (currentButtonState != lastButtonState[buttonIndex]) {
      Joystick.setButton(buttonIndex, currentButtonState);
      lastButtonState[buttonIndex] = currentButtonState;
    }
  }
}
