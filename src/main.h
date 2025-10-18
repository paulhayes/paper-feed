#include <Arduino.h>

#ifndef MAIN_H
#define MAIN_H

#ifndef WIFI_SSID
#define SSID "NOT SET"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "NOT SET"
#endif

bool exists(String path);

#endif