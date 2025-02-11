#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include "esp_bt.h"
#include "esp32-hal-cpu.h"
#include <nvs_flash.h>
#include <soc/rtc.h>
#include <SPIFFS.h>
#include "main.h"

const String localIPURL = "http://192.168.1.1/";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
//WiFiServer server(80);
WebServer server(80);

uint8_t ledState = LOW;
bool updateLed = true;

void setup() {
  setCpuFrequencyMhz(80);
  pinMode(4,OUTPUT);
  Serial.begin(115200);
  //while(!Serial);
  
  //WiFi.persistent(false);
  //WiFi.disconnect(true);
  //esp_bt_controller_disable();
  nvs_flash_init();
  WiFi.setTxPower(WIFI_POWER_7dBm);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(WIFI_SSID,WIFI_PASS);
  SPIFFS.begin();

  
  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.setTTL(3600);
  dnsServer.start(DNS_PORT, "*", apIP);
  const char * headerkeys[] = {"Host", "User-Agent", "Cookie"} ;
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize);
  server.begin();
  server.on("/",handleRoot);
  server.on("/led_on",ledOn);
  server.on("/led_off",ledOff);
  server.onNotFound(handleUnknown);
  /*
  server.on("/hotspot-detect.html",handleRoot);
  server.on("/generate_204",handleRoot);
  server.on("/css/simple-grid.css", [](){
    server.send(200, "text/css", css);
  });
  */
  Serial.println("Waiting for clients"); 
}

void loop() {
  
  dnsServer.processNextRequest();
  server.handleClient();
  delay(5);		

  if(updateLed){
    //digitalWrite(,ledState);
    updateLed = false;
  } 
}

void redirect(const String& url){
  server.sendHeader("Location",url);
  server.send(302,"text/plain","");
}

void handleUnknown(){
  String path = server.uri();
  if(!returnFile(path)){
    handleRoot();
  }
}

bool returnFile(String path){
  Serial.print("File requested ");
  Serial.print( server.header("Host") );
  Serial.print(" ");
  Serial.println(path);

  if( exists(path) ){
    Serial.print("returning file ");
    Serial.println(path);
    File file = SPIFFS.open(path, "r");
    String type = getContentType(path);
    server.streamFile(file,type);
    file.close();
    return true;
  } else {
    Serial.print("file not found ");
    Serial.println(path);
    return false;
  }
}

void handleRoot(){
  //server.send(200,"text/html", captive_portal_html);
  Serial.print("root requested ");  
  Serial.println(server.uri());
  if(!returnFile("/index.html")){
    handle404();
  }
}

void handle404(){
  server.send(404,"text/plain","File Not Found");
  Serial.println("404");
  Serial.println(server.uri());
}

void ledOn(){
  ledState = HIGH;
  updateLed = true;
  server.send(200,"text/plain","OK");
}

void ledOff(){
  ledState = LOW;
  updateLed = true;
  server.send(200,"text/plain","OK");
}

bool exists(String path){
  bool yes = false;
  File file = SPIFFS.open(path, "r");
  
  if(!file.isDirectory()){
    yes = true;
  }
  file.close();
  return yes;
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