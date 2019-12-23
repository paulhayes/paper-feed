#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

//*********SSID and Pass for AP**************/
const char *ssidAP = "give SSID";
//const char *passAP = "pass key";

//*********Static IP Config**************/
IPAddress ap_local_IP(192,168,1,77);
IPAddress ap_gateway(192,168,1,254);
IPAddress ap_subnet(255,255,255,0);

WebServer server(80);

extern const char test_html[] asm("_binary_web_test_html_start");
extern const char test_html_end[] asm("_binary_web_test_html_end");

//**************DEEP SLEEP CONFIG******************//
#define uS_TO_S_FACTOR 1000000  
#define TIME_TO_SLEEP  5

//**********check for connection*************//
bool isConnected = true;
bool isAPConnected = true;

//**********softAPconfig Timer*************//
unsigned long APTimer = 0;
unsigned long APInterval = 120000;

void setup(){
  Serial.begin(115200);
  while(!Serial);
  WiFi.persistent(false);
  WiFi.disconnect(true);

  dhcpAPConfig(ssidAP);
  //handleClientAP();

  reconnectWiFi();
}

void loop(){

}



void handleClientAP(){
   //*********Static IP Config**************//
   WiFi.mode(WIFI_AP);
   Serial.println(WiFi.softAP(ssidAP) ? "soft-AP setup": "Failed to connect");
   delay(100);
   Serial.println(WiFi.softAPConfig( IPAddress(192,168,1,77),IPAddress(192,168,1,254), IPAddress(255,255,255,0))? "Configuring Soft AP" : "Error in Configuration");      
   Serial.println(WiFi.softAPIP());
   server.begin();
   //server.on("/", handleRoot); 
   //server.on("/dhcp", handleDHCP);
   //server.on("/static", handleStatic);
   server.onNotFound(handleNotFound);  
   
   APTimer = millis();
    
   while(isConnected && millis()-APTimer<= APInterval) {
        server.handleClient();}  
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();    
  }

  void  staticAPConfig(String IPStatic, String gateway, String subnet, String ssid, String pass){
      //*********hold IP octet**************//
      uint8_t ip0,ip1,ip2,ip3;
      //*********IP Char Array**************//
      Serial.print(ssid);
      Serial.print(pass);
      byte ip[4];
      parseBytes(IPStatic.c_str(),'.', ip, 4, 10);
      ip0 = (uint8_t)ip[0];
      ip1 = (uint8_t)ip[1];
      ip2 = (uint8_t)ip[2];
      ip3 = (uint8_t)ip[3];
      IPAddress ap_local(ip0,ip1,ip2,ip3);
      parseBytes(gateway.c_str(),'.', ip, 4, 10);
      ip0 = (uint8_t)ip[0];
      ip1 = (uint8_t)ip[1];
      ip2 = (uint8_t)ip[2];
      ip3 = (uint8_t)ip[3];
      IPAddress ap_gate(ip0,ip1,ip2,ip3);
      parseBytes(subnet.c_str(),'.', ip, 4, 10);
      ip0 = (uint8_t)ip[0];
      ip1 = (uint8_t)ip[1];
      ip2 = (uint8_t)ip[2];
      ip3 = (uint8_t)ip[3];
      IPAddress ap_net(ip0,ip1,ip2,ip3);  
      WiFi.disconnect(true);
      WiFi.mode(WIFI_AP);   
      Serial.println(WiFi.softAP(ssid.c_str(),pass.c_str()) ? "Setting up SoftAP" : "error setting up");
      delay(100);       
      Serial.println(WiFi.softAPConfig(ap_local, ap_gate, ap_net) ? "Configuring softAP" : "kya yaar not connected");    
      Serial.println(WiFi.softAPIP());
      server.begin();
      server.on("/", handleStaticForm); 
      server.onNotFound(handleNotFound);
   
      APTimer = millis();
      while(isAPConnected && millis()-APTimer<= APInterval) {
         server.handleClient();  }            
    }

//***************************WiFi Credintial Form**************************//

void dhcpAPConfig( const char ssid[]){
      WiFi.mode(WIFI_OFF);
      WiFi.softAPdisconnect(true);
      delay(1000);
      WiFi.mode(WIFI_AP);
      Serial.println(WiFi.softAP(ssid) ? "Setting up SoftAP" : "error setting up");
      delay(200);
      Serial.println(WiFi.softAPIP());
      
      server.begin();
      server.on("/", handleStaticForm); 
      server.onNotFound(handleNotFound);
      APTimer = millis();
      while(isAPConnected && millis()-APTimer<= APInterval) {
        server.handleClient();  
      }
       
}
  
void reconnectWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
        String string_Ssid="";
        String string_Password="";
        string_Ssid= ssidAP; 
        //string_Password= passAP;        
        Serial.println("ssid: "+ string_Ssid);
       // Serial.println("Password: "+string_Password);
               
  delay(400);
  WiFi.begin(string_Ssid.c_str());
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED)
  {   
      delay(500);
      Serial.print(".");
      if(counter == 20){
          String response = "<script>alert(\"Password not connected\")</script";
          server.send(200,"text/html",response);
          ESP.restart();
        }
        counter++;
  }

  Serial.print("Connected to:\t");
  Serial.println(WiFi.localIP());
}


int getRSSIasQuality(int RSSI) {
  int quality = 0;
  if (RSSI <= -100) {
    quality = 0;
  } else if (RSSI >= -50) {
    quality = 100;
  } else {
    quality = 2 * (RSSI + 100);
  }
  return quality;
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  server.send(404, "text/plain", message);
}

void handleStaticForm(){
  server.send(200,test_html);
}

//****************************EEPROM Read****************************//
/*String read_string(int l, int p){
  String temp;
  for (int n = p; n < l+p; ++n)
    {
   // read the saved password from EEPROM
     if(char(EEPROM.read(n))!=';'){
     
       temp += String(char(EEPROM.read(n)));
     }else n=l+p;
    }
  return temp;
}*/

//***************Parse bytes from string******************//

void parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base) {
    for (int i = 0; i < maxBytes; i++) {
        bytes[i] = strtoul(str, NULL, base);  // Convert byte
        str = strchr(str, sep);               // Find next separator
        if (str == NULL || *str == '\0') {
            break;                            // No more separators, exit
        }
        str++;                                // Point to next character after separator
    }
}