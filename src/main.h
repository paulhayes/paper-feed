#include <Arduino.h>

void handle404();
void handleRoot();
String getContentType(String filename);
bool exists(String path);
void ledOn();
void ledOff();
bool returnFile(String path);
void handleUnknown();
void redirect(const String& url);