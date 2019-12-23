#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include "esp_bt.h"
#include "esp32-hal-cpu.h"
#include <nvs_flash.h>
#include <soc/rtc.h>

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
//WiFiServer server(80);
WebServer server(80);

extern const char captive_portal_html[] asm("_binary_web_test_html_start");
extern const char captive_portal_html_end[] asm("_binary_web_test_html_end");

extern const char css[] asm("_binary_web_css_simple_grid_css_start");
extern const char css_end[] asm("_binary_web_css_simple_grid_css_end");


uint8_t ledState = LOW;
bool updateLed = true;

void setup() {
  pinMode(4,OUTPUT);
  Serial.begin(115200);
  while(!Serial);
  setCpuFrequencyMhz(80);
  WiFi.persistent(false);
  WiFi.disconnect(true);
  esp_bt_controller_disable();
  nvs_flash_init();
  WiFi.mode(WIFI_AP);
  WiFi.softAP("DNSServer CaptivePortal");
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.setTxPower(WIFI_POWER_7dBm);
  
  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);
  server.on("/",handleRoot);
  server.on("/hotspot-detect.html",handleRoot);
  server.on("/generate_204",handleRoot);
  server.on("/led_on",ledOn);
  server.on("/led_off",ledOff);
  server.on("/css/simple-grid.css", [](){
    server.send(200, "text/css", css);
  });
  server.onNotFound(handleRoot);
  server.begin();
  
}

void loop() {
  
  dnsServer.processNextRequest();
  server.handleClient();

  if(updateLed){
    digitalWrite(4,ledState);
    updateLed = false;
  } 
}


void handleRoot(){
  server.send(200,"text/html", captive_portal_html);
  Serial.println("root requested");  
  Serial.println(server.uri());
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