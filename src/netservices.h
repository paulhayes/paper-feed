#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

#ifndef NETSERVICES_H
#define NETSERVICES_H

extern DNSServer dnsServer;
extern WebServer server;
extern IPAddress apIP;

void networkSetup();
void setupWiFi();
void setupOTA();
void setupDNS();
void setupWebServer();
void webTask(void *p);

void handleRoot();
void handle404();
void handleUnknown();
void ledOn();
void ledOff();
void handleGetMessages();
void handlePostMessage();
bool returnFile(String path);
void redirect(const String& url);
String getContentType(String filename);

// Message management functions
void initMessageSystem();
void loadMessagesFromDisk();
void cleanupOldMessages();
String sanitizeMessage(const String& message);
String getLatestMessage();
void clearAllMessages();

#endif
