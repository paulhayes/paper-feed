#include <Arduino.h>
#include "esp_bt.h"
#include "esp32-hal-cpu.h"
#include <soc/rtc.h>
#include <SPIFFS.h>
#include "main.h"
#include "netservices.h"
#include "commands.h"

uint8_t ledState = LOW;
bool updateLed = true;

void setup() {
  //setCpuFrequencyMhz(80);
  pinMode(4, OUTPUT);
  Serial.begin(115200);

  // Initialize M5Unified
  auto cfg = M5.config();
  
  M5.begin();

  SPIFFS.begin();

  networkSetup();

  Serial.println("Waiting for clients");

  setupCommands();

  xTaskCreatePinnedToCore(webTask, "WEB_UPDATE", 4096, NULL, 1, NULL, 1);
}

void loop() {
  shell.executeIfInput();

  delay(5);

  if(updateLed) {
    digitalWrite(4, ledState);
    updateLed = false;
  }
}

bool exists(String path) {
  if(!SPIFFS.exists(path)) {
    return false;
  }

  File file = SPIFFS.open(path, "r");
  bool fileExists = false;
  if(!file.isDirectory()) {
    fileExists = true;
  }
  file.close();
  return fileExists;
}

unsigned long getRtcTime() {
  auto dt = M5.Rtc.getDateTime();
  // Convert to Unix timestamp
  struct tm timeinfo;
  timeinfo.tm_year = dt.date.year - 1900;
  timeinfo.tm_mon = dt.date.month - 1;
  timeinfo.tm_mday = dt.date.date;
  timeinfo.tm_hour = dt.time.hours;
  timeinfo.tm_min = dt.time.minutes;
  timeinfo.tm_sec = dt.time.seconds;
  timeinfo.tm_isdst = 0;
  return mktime(&timeinfo);
}