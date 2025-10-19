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
        canvas.drawPngFile(LittleFS, QR_IMAGE_PATH, 0, 0);
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
    canvas.fillRect(0, messageY, displayWidth, messageAreaHeight, TFT_LIGHTGRAY);

    // Set up text for message display
    canvas.setTextColor(TFT_BLACK);
    canvas.setTextDatum(TL_DATUM);
    canvas.setFont(&fonts::Font4);

    // Draw "Latest Message:" label
    canvas.drawString("Latest Message:", 10, messageY + 5);

    // Draw the message with word wrapping
    if (message.length() > 0) {
        int textX = 10;
        int textY = messageY + 35;
        int maxWidth = displayWidth - 20;

        canvas.setFont(&fonts::Font6);

        // Simple word wrapping
        String remainingText = message;
        int lineHeight = 32;

        while (remainingText.length() > 0 && textY < displayHeight - 10) {
            int endIndex = remainingText.length();

            // Find the longest substring that fits
            while (endIndex > 0 && canvas.textWidth(remainingText.substring(0, endIndex)) > maxWidth) {
                endIndex--;
            }

            // If we couldn't fit even one character, break
            if (endIndex == 0) {
                endIndex = 1;
            }

            // Try to break at a space if possible
            if (endIndex < remainingText.length()) {
                int spaceIndex = remainingText.lastIndexOf(' ', endIndex);
                if (spaceIndex > 0 && spaceIndex > endIndex - 20) {
                    endIndex = spaceIndex + 1;
                }
            }

            // Draw the line
            String line = remainingText.substring(0, endIndex);
            line.trim();
            canvas.drawString(line, textX, textY);

            // Move to next line
            remainingText = remainingText.substring(endIndex);
            remainingText.trim();
            textY += lineHeight;
        }
    } else {
        canvas.setFont(&fonts::Font4);
        canvas.drawString("No messages yet", 10, messageY + 35);
    }

    // Push canvas to display
    canvas.pushSprite(0, 0);
}

void updateMessageDisplay(const String& message) {
    Serial.print("Updating display with message: ");
    Serial.println(message);
    displayQRCodeAndMessage(message);
}
