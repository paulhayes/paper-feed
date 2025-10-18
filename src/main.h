#include <Arduino.h>

#ifndef MAIN_H
#define MAIN_H

#ifndef WIFI_SSID
#define SSID "NOT SET"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "NOT SET"
#endif


void handle404();
void handleRoot();
String getContentType(String filename);
bool exists(String path);
void ledOn();
void ledOff();
bool returnFile(String path);
void handleUnknown();
void redirect(const String& url);
void webTask(void *p);

#endif