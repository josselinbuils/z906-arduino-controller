#include <Keyboard.h>
#include <Joystick.h>
#include "RotaryEncoder.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#include <Z906.h>

#define I2C_ADDRESS 0x3C

const int DEBOUNCE_DELAY_MS = 300;
const int PIN_CLK = A2;
const int PIN_DT = A3;
const int PIN_BTN = A4;
const int PIN_BUTTON_0 = 4;
const int PIN_BUTTON_1 = 5;
const int PIN_BUTTON_2 = 6;
const int PIN_BUTTON_3 = 7;
const int PIN_BUTTON_4 = 8;
const int PIN_BUTTON_5 = 9;
const int PIN_BUTTON_6 = 10;
const int PIN_BUTTON_7 = 11;
const uint8_t STATUS_STBY = 0x14;

int lastButtonState[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int lastDisplayedMainLevel = 0;
int mainLevel = 0;
int running = 0;
unsigned long buttonLastClickedTimeMs = 0;
unsigned long lastEncoderCheckTimeMs = 0;
unsigned long lastScreenRefreshTimeMs = 0;
unsigned long lastStatusCheckTimeMs = 0;
Joystick_ Joystick;
RotaryEncoder encoder(PIN_DT, PIN_CLK);
SSD1306AsciiAvrI2c oled;
Z906 LOGI(Serial1);

void setup() {
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_CLK, INPUT_PULLUP);
  pinMode(PIN_DT, INPUT_PULLUP);
  pinMode(PIN_BUTTON_0, INPUT_PULLUP);
  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);
  pinMode(PIN_BUTTON_3, INPUT_PULLUP);
  pinMode(PIN_BUTTON_4, INPUT_PULLUP);
  pinMode(PIN_BUTTON_5, INPUT_PULLUP);
  pinMode(PIN_BUTTON_6, INPUT_PULLUP);
  pinMode(PIN_BUTTON_7, INPUT_PULLUP);

  Joystick.begin();
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setFont(Adafruit5x7);
  Serial.begin(9600);
  Serial.println("Starting...");

  while (LOGI.request(VERSION) == 0) {
    Serial.println("Waiting Z906 Power-Up");
    delay(1000);
  }

  LOGI.input(SELECT_INPUT_3);
  LOGI.cmd(MUTE_OFF);

  mainLevel = LOGI.request(MAIN_LEVEL);
  running = !LOGI.request(STATUS_STBY);

  refreshScreenPowerStatus();
  refreshScreenMainLevel();

  Serial.println("");
  Serial.println("Z906 Version: " + (String)LOGI.request(VERSION));
  Serial.println("Main level: " + (String)mainLevel);
  Serial.println("Rear level: " + (String)LOGI.request(REAR_LEVEL));
  Serial.println("Center level: " + (String)LOGI.request(CENTER_LEVEL));
  Serial.println("Sub level: " + (String)LOGI.request(SUB_LEVEL));
  Serial.println("Running: " + (String)running);
  Serial.println("");
}

void loop() {
  unsigned long now = millis();

  if ((now - lastStatusCheckTimeMs) > DEBOUNCE_DELAY_MS) {
    lastStatusCheckTimeMs = now;

    int r = LOGI.request(VERSION) != 0 && !LOGI.request(STATUS_STBY);

    if (running != r) {
      running = r;
      refreshScreenPowerStatus();
      Serial.println("Power status changed: " + (String)running);
    }
  }

  if ((now - buttonLastClickedTimeMs) > DEBOUNCE_DELAY_MS && !digitalRead(PIN_BTN)) {
    buttonLastClickedTimeMs = now;

    if (LOGI.request(STATUS_STBY)) {
      LOGI.on();
      LOGI.input(SELECT_INPUT_3);
      LOGI.cmd(MUTE_OFF);
      Serial.println("Power on");
    } else {
      LOGI.off();
      Serial.println("Power off");
    }
  }

  for (int buttonIndex = 0; buttonIndex < 8; buttonIndex++) {
    int currentButtonState = !digitalRead(PIN_BUTTON_0 + buttonIndex);

    if (currentButtonState != lastButtonState[buttonIndex]) {
      Serial.println((String)buttonIndex + " " + (String)lastButtonState[buttonIndex] + " " + (String)currentButtonState);
      Joystick.setButton(buttonIndex, currentButtonState);
      lastButtonState[buttonIndex] = currentButtonState;
    }
  }

  if (running == 0) {
    return;
  }

  encoder.tick();

  if ((now - lastEncoderCheckTimeMs) > DEBOUNCE_DELAY_MS) {
    int deltaPos = encoder.getPosition();

    if (deltaPos != 0) {
      encoder.setPosition(0);

      if (deltaPos > 0) {
        LOGI.cmd(LEVEL_MAIN_UP);
      } else {
        LOGI.cmd(LEVEL_MAIN_DOWN);
      }
    }
  }

  if ((now - lastScreenRefreshTimeMs) > DEBOUNCE_DELAY_MS) {
    lastScreenRefreshTimeMs = now;
    mainLevel = LOGI.request(MAIN_LEVEL);

    if (lastDisplayedMainLevel != mainLevel) {
      lastDisplayedMainLevel = mainLevel;
      refreshScreenMainLevel();
      Serial.println("Main level: " + (String)mainLevel);
    }
  }
}

void refreshScreenPowerStatus() {
  oled.set1X();

  if (running == 1) {
    oled.setCursor(110, 1);
    oled.print("ON");
  } else {
    oled.clearField(110, 1, 2);
  }

  refreshScreenMainLevel();
}

void refreshScreenMainLevel() {
  oled.set2X();

  if (running == 1) {
    int level_percent = int((float)mainLevel * 100.0 / 255.0);

    oled.setCursor(20, 2);
    oled.print((String)level_percent + "%");

    if (level_percent < 100) {
      oled.print(" ");
    }
  } else {
    oled.clearField(20, 2, 4);
  }
}
