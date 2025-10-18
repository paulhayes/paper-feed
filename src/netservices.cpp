#include <ArduinoOTA.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "netservices.h"
#include "main.h"

const String localIPURL = "http://192.168.1.1/";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
WebServer server(80);

// Message storage
const int MAX_MESSAGES = 50;
const unsigned long MESSAGE_MAX_AGE = 30UL * 24UL * 60UL * 60UL; // 30 days in seconds
const unsigned long RATE_LIMIT_SECONDS = 60; // 1 minute between posts

struct Message {
    unsigned long timestamp;
    String text;
};

Message messages[MAX_MESSAGES];
int messageCount = 0;
unsigned long lastPostTime = 0;

extern uint8_t ledState;
extern bool updateLed;
extern bool exists(String path);

char wifi_ssid[] = WIFI_SSID;
char wifi_password[] = WIFI_PASS;

void networkSetup(){
  setupWiFi();
  setupOTA();
  setupDNS();
  initMessageSystem();
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
    server.on("/api/messages", HTTP_GET, handleGetMessages);
    server.on("/api/message", HTTP_POST, handlePostMessage);
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
    } else if (filename.endsWith(".json")) {
        return "application/json";
    }
    return "text/plain";
}

// Initialize message system
void initMessageSystem() {
    Serial.println("Initializing message system");

    // Create messages directory if it doesn't exist
    if (!SPIFFS.exists("/messages")) {
        Serial.println("Creating /messages directory");
        SPIFFS.mkdir("/messages");
    }

    // Load existing messages from disk
    loadMessagesFromDisk();

    // Clean up old messages
    cleanupOldMessages();

    Serial.print("Loaded ");
    Serial.print(messageCount);
    Serial.println(" messages");
}

// Sanitize message to prevent XSS
String sanitizeMessage(const String& message) {
    String sanitized = message;

    // Replace special HTML characters
    sanitized.replace("&", "&amp;");
    sanitized.replace("<", "&lt;");
    sanitized.replace(">", "&gt;");
    sanitized.replace("\"", "&quot;");
    sanitized.replace("'", "&#39;");

    // Remove any control characters
    String result = "";
    for (unsigned int i = 0; i < sanitized.length(); i++) {
        char c = sanitized.charAt(i);
        if (c >= 32 || c == '\n' || c == '\r' || c == '\t') {
            result += c;
        }
    }

    return result;
}

// Load messages from disk into memory
void loadMessagesFromDisk() {
    messageCount = 0;

    File root = SPIFFS.open("/messages");
    if (!root || !root.isDirectory()) {
        Serial.println("Failed to open /messages directory");
        return;
    }

    // Collect all message files
    File file = root.openNextFile();
    while (file && messageCount < MAX_MESSAGES) {
        if (!file.isDirectory()) {
            String filename = String(file.name());

            // Parse timestamp from filename
            int lastSlash = filename.lastIndexOf('/');
            String basename = filename.substring(lastSlash + 1);
            unsigned long timestamp = basename.toInt();

            if (timestamp > 0) {
                // Read message content
                String content = "";
                while (file.available()) {
                    content += (char)file.read();
                }

                messages[messageCount].timestamp = timestamp;
                messages[messageCount].text = content;
                messageCount++;
            }
        }
        file = root.openNextFile();
    }

    // Sort messages by timestamp (newest first)
    for (int i = 0; i < messageCount - 1; i++) {
        for (int j = i + 1; j < messageCount; j++) {
            if (messages[i].timestamp < messages[j].timestamp) {
                Message temp = messages[i];
                messages[i] = messages[j];
                messages[j] = temp;
            }
        }
    }
}

// Clean up old messages
void cleanupOldMessages() {
    unsigned long now = millis() / 1000; // Current time in seconds (approximation)

    // Remove messages older than 30 days
    int writeIdx = 0;
    for (int i = 0; i < messageCount; i++) {
        unsigned long age = now - messages[i].timestamp;
        if (age <= MESSAGE_MAX_AGE) {
            if (writeIdx != i) {
                messages[writeIdx] = messages[i];
            }
            writeIdx++;
        } else {
            // Delete file from disk
            String filename = "/messages/" + String(messages[i].timestamp);
            if (SPIFFS.exists(filename)) {
                SPIFFS.remove(filename);
                Serial.print("Deleted old message: ");
                Serial.println(filename);
            }
        }
    }
    messageCount = writeIdx;

    // If we have more than 50 messages, remove the oldest ones
    while (messageCount > MAX_MESSAGES) {
        messageCount--;
        String filename = "/messages/" + String(messages[messageCount].timestamp);
        if (SPIFFS.exists(filename)) {
            SPIFFS.remove(filename);
            Serial.print("Deleted excess message: ");
            Serial.println(filename);
        }
    }
}

// Handle GET /api/messages
void handleGetMessages() {
    server.sendHeader("Access-Control-Allow-Origin", "*");

    DynamicJsonDocument doc(8192);
    JsonArray msgArray = doc.createNestedArray("messages");

    // Return only the last 50 messages (they're already sorted newest first)
    int limit = messageCount < MAX_MESSAGES ? messageCount : MAX_MESSAGES;
    for (int i = 0; i < limit; i++) {
        JsonObject msg = msgArray.createNestedObject();
        msg["timestamp"] = messages[i].timestamp;
        msg["text"] = messages[i].text;
    }

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

// Handle POST /api/message
void handlePostMessage() {
    server.sendHeader("Access-Control-Allow-Origin", "*");

    // Check rate limit
    unsigned long now = millis() / 1000;
    if (now - lastPostTime < RATE_LIMIT_SECONDS) {
        DynamicJsonDocument doc(256);
        doc["success"] = false;
        doc["error"] = "Rate limit exceeded. Please wait before posting again.";
        String response;
        serializeJson(doc, response);
        server.send(429, "application/json", response);
        return;
    }

    // Parse JSON body
    if (!server.hasArg("plain")) {
        DynamicJsonDocument doc(256);
        doc["success"] = false;
        doc["error"] = "No message provided";
        String response;
        serializeJson(doc, response);
        server.send(400, "application/json", response);
        return;
    }

    String body = server.arg("plain");
    DynamicJsonDocument requestDoc(512);
    DeserializationError error = deserializeJson(requestDoc, body);

    if (error) {
        DynamicJsonDocument doc(256);
        doc["success"] = false;
        doc["error"] = "Invalid JSON";
        String response;
        serializeJson(doc, response);
        server.send(400, "application/json", response);
        return;
    }

    if (!requestDoc.containsKey("message")) {
        DynamicJsonDocument doc(256);
        doc["success"] = false;
        doc["error"] = "No message field in JSON";
        String response;
        serializeJson(doc, response);
        server.send(400, "application/json", response);
        return;
    }

    String message = requestDoc["message"].as<String>();
    message.trim();

    // Validate message length
    if (message.length() == 0 || message.length() > 140) {
        DynamicJsonDocument doc(256);
        doc["success"] = false;
        doc["error"] = "Message must be between 1 and 140 characters";
        String response;
        serializeJson(doc, response);
        server.send(400, "application/json", response);
        return;
    }

    // Sanitize the message
    String sanitized = sanitizeMessage(message);

    // Add message to array (shift if necessary)
    if (messageCount >= MAX_MESSAGES) {
        // Remove oldest message (last in array since sorted newest first)
        String oldFilename = "/messages/" + String(messages[MAX_MESSAGES - 1].timestamp);
        if (SPIFFS.exists(oldFilename)) {
            SPIFFS.remove(oldFilename);
        }
        messageCount = MAX_MESSAGES - 1;
    }

    // Shift messages down to make room at the beginning
    for (int i = messageCount; i > 0; i--) {
        messages[i] = messages[i - 1];
    }

    // Add new message at the beginning
    messages[0].timestamp = now;
    messages[0].text = sanitized;
    messageCount++;

    // Save to disk
    String filename = "/messages/" + String(now);
    File file = SPIFFS.open(filename, "w");
    if (file) {
        file.print(sanitized);
        file.close();
        Serial.print("Saved message: ");
        Serial.println(filename);
    } else {
        Serial.print("Failed to save message: ");
        Serial.println(filename);
    }

    // Update last post time
    lastPostTime = now;

    // Clean up old messages
    cleanupOldMessages();

    // Send success response
    DynamicJsonDocument doc(256);
    doc["success"] = true;
    doc["timestamp"] = now;
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}
