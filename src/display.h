#include <Arduino.h>
#include <LittleFS.h>
#include <M5Unified.h>

#ifndef DISPLAY_H
#define DISPLAY_H

void setupDisplay();
void displayQRCodeAndMessage(const String& message);
void updateMessageDisplay(const String& message);

#endif
