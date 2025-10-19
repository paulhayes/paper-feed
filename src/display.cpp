#include "display.h"
#include "main.h"
#include "netservices.h"

// Canvas for drawing
M5Canvas canvas(&M5.Display);

const int QR_SIZE = 370;
const char* QR_IMAGE_PATH = "/connect-qr.png";

void setupDisplay() {
    // Initialize display
    M5.Display.begin();
    M5.Display.setRotation(0);
    M5.Display.fillScreen(TFT_WHITE);

    // Get display dimensions
    int displayWidth = M5.Display.width();
    int displayHeight = M5.Display.height();

    Serial.print("Display dimensions: ");
    Serial.print(displayWidth);
    Serial.print(" x ");
    Serial.println(displayHeight);

    // Create canvas for the entire display
    canvas.createSprite(displayWidth, displayHeight);

    // Display QR code with latest message if available
    String latestMessage = getLatestMessage();
    Serial.print("Latest message:");
    Serial.println(latestMessage);
    displayQRCodeAndMessage(latestMessage);
}

void displayQRCodeAndMessage(const String& message) {
    // Get display dimensions
    int displayWidth = M5.Display.width();
    int displayHeight = M5.Display.height();

    // Clear canvas
    canvas.fillSprite(TFT_WHITE);

    // Draw QR code
    if (exists(QR_IMAGE_PATH)) {
        int qrcodeSize=370;
        int x = (canvas.width()-qrcodeSize)/2;
        canvas.drawPngFile(LittleFS, QR_IMAGE_PATH, x, 0);
        char qrcodeStr[64];
        sprintf(qrcodeStr,"WIFI:T:WPA;S:%s;P:%s;;",WIFI_SSID,WIFI_PASS);
        canvas.qrcode(qrcodeStr,x,0,qrcodeSize);        
    } else {
        // Draw placeholder if QR code not found
        canvas.setTextColor(TFT_BLACK);
        canvas.setTextSize(2);
        canvas.drawString("QR Code", 10, 10);
        canvas.drawString("Not Found", 10, 30);
    }

    // Calculate message area (below QR code)
    int messageY = QR_SIZE + 10;
    int messageAreaHeight = displayHeight - QR_SIZE - 20;
    
    // Draw message area background
    //canvas.fillRect(0, messageY, displayWidth, messageAreaHeight, TFT_LIGHTGRAY);
    canvas.setTextWrap(true);

    // Set up text for message display
    canvas.setTextColor(TFT_BLACK);
    canvas.setTextDatum(TL_DATUM);
    canvas.setFont(&fonts::DejaVu56);

    // Draw "Latest Message:" label
    canvas.drawString("Latest Message:", 10, messageY + 5);
    messageY+=canvas.fontHeight()+5;
    
    if (message.length() > 0) {
        canvas.setFont(&fonts::DejaVu56);
        canvas.setTextWrap(true);
        canvas.setCursor(10, messageY + 5 + canvas.fontHeight());
        //canvas.drawString(message,10, messageY);
        canvas.write(message.c_str());
        canvas.setTextWrap(true);
    } else {
        canvas.setFont(&fonts::Font4);
        canvas.drawString("No messages yet",10,messageY);
    }

    

    // Push canvas to display
    canvas.pushSprite(0, 0);
}

void updateMessageDisplay(const String& message) {
    Serial.print("Updating display with message: ");
    Serial.println(message);
    displayQRCodeAndMessage(message);
}
