#include <ArduinoOTA.h>
#include <SPIFFS.h>
#include "netservices.h"
#include "main.h"

const String localIPURL = "http://192.168.1.1/";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
WebServer server(80);

extern uint8_t ledState;
extern bool updateLed;
extern bool exists(String path);

char wifi_ssid[] = WIFI_SSID;
char wifi_password[] = WIFI_PASS;

void networkSetup(){
  setupWiFi();
  setupOTA();
  setupDNS();
  setupWebServer();
}

void setupWiFi() {
    WiFi.setTxPower(WIFI_POWER_7dBm);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(wifi_ssid, wifi_password);
}

void setupOTA() {
    ArduinoOTA.setPasswordHash(OTA_HASH);
    ArduinoOTA
        .onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH) {
                type = "sketch";
            } else {
                type = "filesystem";
            }
            Serial.println("Start updating " + type);
        })
        .onEnd([]() {
            Serial.println("\nEnd");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) {
                Serial.println("Auth Failed");
            } else if (error == OTA_BEGIN_ERROR) {
                Serial.println("Begin Failed");
            } else if (error == OTA_CONNECT_ERROR) {
                Serial.println("Connect Failed");
            } else if (error == OTA_RECEIVE_ERROR) {
                Serial.println("Receive Failed");
            } else if (error == OTA_END_ERROR) {
                Serial.println("End Failed");
            }
        });

    Serial.println("Starting OTA service");
    ArduinoOTA.begin();
}

void setupDNS() {
    Serial.println("Starting DNS service");
    dnsServer.setTTL(3600);
    dnsServer.start(DNS_PORT, "*", apIP);
}

void setupWebServer() {
    const char * headerkeys[] = {"Host", "User-Agent", "Cookie"};
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
    server.collectHeaders(headerkeys, headerkeyssize);
    server.begin();
    server.on("/", handleRoot);
    server.on("/led_on", ledOn);
    server.on("/led_off", ledOff);
    server.onNotFound(handleUnknown);
}

void webTask(void *p) {
    while(true) {
        dnsServer.processNextRequest();
        server.handleClient();
    }
}

void redirect(const String& url) {
    server.sendHeader("Location", url);
    server.send(302, "text/plain", "");
}

void handleUnknown() {
    String path = server.uri();
    if(!returnFile(path)) {
        handleRoot();
    }
}

bool returnFile(String path) {
    Serial.print("File requested ");
    Serial.print(server.header("Host"));
    Serial.print(" ");
    Serial.println(path);

    if(exists(path)) {
        Serial.print("returning file ");
        Serial.println(path);
        File file = SPIFFS.open(path, "r");
        String type = getContentType(path);
        server.streamFile(file, type);
        file.close();
        return true;
    } else {
        Serial.print("file not found ");
        Serial.println(path);
        return false;
    }
}

void handleRoot() {
    Serial.print("root requested ");
    Serial.println(server.uri());
    if(!returnFile("/index.html")) {
        handle404();
    }
}

void handle404() {
    server.send(404, "text/plain", "File Not Found");
    Serial.println("404");
    Serial.println(server.uri());
}

void ledOn() {
    ledState = HIGH;
    updateLed = true;
    server.send(200, "text/plain", "OK");
}

void ledOff() {
    ledState = LOW;
    updateLed = true;
    server.send(200, "text/plain", "OK");
}

String getContentType(String filename) {
    if (server.hasArg("download")) {
        return "application/octet-stream";
    } else if (filename.endsWith(".htm")) {
        return "text/html";
    } else if (filename.endsWith(".html")) {
        return "text/html";
    } else if (filename.endsWith(".css")) {
        return "text/css";
    } else if (filename.endsWith(".js")) {
        return "application/javascript";
    } else if (filename.endsWith(".png")) {
        return "image/png";
    } else if (filename.endsWith(".gif")) {
        return "image/gif";
    } else if (filename.endsWith(".jpg")) {
        return "image/jpeg";
    } else if (filename.endsWith(".ico")) {
        return "image/x-icon";
    } else if (filename.endsWith(".xml")) {
        return "text/xml";
    } else if (filename.endsWith(".pdf")) {
        return "application/x-pdf";
    } else if (filename.endsWith(".zip")) {
        return "application/x-zip";
    } else if (filename.endsWith(".gz")) {
        return "application/x-gzip";
    } else if (filename.endsWith(".mp3")) {
        return "audio/mpeg";
    }
    return "text/plain";
}
