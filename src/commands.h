#include "main.h"
#include <SimpleSerialShell.h>
#include <Preferences.h>
#include "properties.h"

#ifndef COMMANDS_H
#define COMMANDS_H

int badArgCount( char * cmdName )
{
    shell.print(cmdName);
    shell.println(F(": bad arg count"));
    return -1;
}

int cmd_reset(int argc, char **argv) {
    Serial.println("Resetting MCU...");
    delay(100);
    ESP.restart();
    return EXIT_SUCCESS;
}

int cmd_led(int argc, char **argv) {
    if(argc < 2) {
        return badArgCount(argv[0]);
    }

    if(strcmp(argv[1], "on") == 0) {
        ledOn();
        Serial.println("LED on");
    } else if(strcmp(argv[1], "off") == 0) {
        ledOff();
        Serial.println("LED off");
    } else {
        Serial.println("Usage: led <on|off>");
        return -1;
    }
    return EXIT_SUCCESS;
}

int cmd_set(int argc, char **argv) {
    if(argc < 3) {
        return badArgCount(argv[0]);
    }

    property_t* prop = findProperty(argv[1]);
    if(!prop) {
        Serial.print("Property not found: ");
        Serial.println(argv[1]);
        return -2;
    }

    prefs.begin("settings", false);
    switch(prop->propertyType) {
        case STRING:
            *(String*)prop->propertyValue = String(argv[2]);
            prefs.putString(prop->propertyName, argv[2]);
            break;
        case FLOAT:
            *(float*)prop->propertyValue = atof(argv[2]);
            prefs.putFloat(prop->propertyName, atof(argv[2]));
            break;
        case INT:
            *(int*)prop->propertyValue = atoi(argv[2]);
            prefs.putInt(prop->propertyName, atoi(argv[2]));
            break;
    }
    prefs.end();

    Serial.print(prop->propertyName);
    Serial.print(" = ");
    Serial.println(argv[2]);
    return EXIT_SUCCESS;
}

int cmd_get(int argc, char **argv) {
    if(argc < 2) {
        return badArgCount(argv[0]);
    }

    property_t* prop = findProperty(argv[1]);
    if(!prop) {
        Serial.print("Property not found: ");
        Serial.println(argv[1]);
        return -2;
    }

    Serial.print(prop->propertyName);
    Serial.print(" = ");
    switch(prop->propertyType) {
        case STRING:
            Serial.println(*(String*)prop->propertyValue);
            break;
        case FLOAT:
            Serial.println(*(float*)prop->propertyValue);
            break;
        case INT:
            Serial.println(*(int*)prop->propertyValue);
            break;
    }
    return EXIT_SUCCESS;
}

int cmd_exists(int argc, char **argv) {
    if(argc < 2) {
        return badArgCount(argv[0]);
    }

    if(exists(String(argv[1]))) {
        Serial.print(argv[1]);
        Serial.println(" exists");
    } else {
        Serial.print(argv[1]);
        Serial.println(" not found");
    }
    return EXIT_SUCCESS;
}

int cmd_cat(int argc, char **argv){
    if(argc < 2) {
        return badArgCount(argv[0]);
    }
    const char* path = argv[1];
    if(exists(path)){
        File file = LittleFS.open(path,"r");
        shell.println( file.readString());
        file.close();
    }
}

int cmd_dir(int argc, char **argv) {
    const char* path = "/";
    if(argc >= 2) {
        path = argv[1];
    }

    File root = LittleFS.open(path);
    if(!root) {
        Serial.println("Failed to open directory");
        return -3;
    }

    if(!root.isDirectory()) {
        Serial.println("Not a directory");
        root.close();
        return -4;
    }

    File file = root.openNextFile();
    while(file) {
        if(file.isDirectory()) {
            Serial.print("[DIR]  ");
        } else {
            Serial.print("       ");
            Serial.print(file.size());
            Serial.print("\t");
        }
        Serial.println(file.name());
        file = root.openNextFile();
    }
    root.close();
    return EXIT_SUCCESS;
}

int cmd_props(int argc, char **argv) {
    if(propertyCount == 0) {
        Serial.println("No properties registered");
        return EXIT_SUCCESS;
    }

    Serial.print("Registered properties (");
    Serial.print(propertyCount);
    Serial.println("):");

    for(int i = 0; i < propertyCount; i++) {
        Serial.print("  ");
        Serial.print(properties[i].propertyName);
        Serial.print(" [");
        switch(properties[i].propertyType) {
            case STRING:
                Serial.print("STRING");
                break;
            case FLOAT:
                Serial.print("FLOAT");
                break;
            case INT:
                Serial.print("INT");
                break;
        }
        Serial.print("] = ");
        switch(properties[i].propertyType) {
            case STRING:
                Serial.println(*(String*)properties[i].propertyValue);
                break;
            case FLOAT:
                Serial.println(*(float*)properties[i].propertyValue);
                break;
            case INT:
                Serial.println(*(int*)properties[i].propertyValue);
                break;
        }
    }
    return EXIT_SUCCESS;
}

int cmd_date(int argc, char **argv) {
    if(argc == 1) {
        // Get current date/time
        auto dt = M5.Rtc.getDateTime();
        char buffer[32];
        sprintf(buffer, "%04d-%02d-%02dT%02d:%02d:%02dZ",
                dt.date.year, dt.date.month, dt.date.date,
                dt.time.hours, dt.time.minutes, dt.time.seconds);
        Serial.println(buffer);
        return EXIT_SUCCESS;
    } else if(argc == 2) {
        // Set date/time from ISO 8601 UTC string
        // Expected format: YYYY-MM-DDTHH:MM:SSZ
        String dateStr = String(argv[1]);

        if(dateStr.length() < 19) {
            Serial.println("Invalid date format. Expected: YYYY-MM-DDTHH:MM:SSZ");
            return -1;
        }

        // Parse the date string
        int year = dateStr.substring(0, 4).toInt();
        int month = dateStr.substring(5, 7).toInt();
        int day = dateStr.substring(8, 10).toInt();
        int hour = dateStr.substring(11, 13).toInt();
        int minute = dateStr.substring(14, 16).toInt();
        int second = dateStr.substring(17, 19).toInt();

        // Validate ranges
        if(year < 2000 || year > 2099 ||
           month < 1 || month > 12 ||
           day < 1 || day > 31 ||
           hour < 0 || hour > 23 ||
           minute < 0 || minute > 59 ||
           second < 0 || second > 59) {
            Serial.println("Date/time values out of range");
            return -2;
        }

        // Create DateTime structure
        m5::rtc_datetime_t rtcDateTime;
        rtcDateTime.date.year = year;
        rtcDateTime.date.month = month;
        rtcDateTime.date.date = day;
        rtcDateTime.time.hours = hour;
        rtcDateTime.time.minutes = minute;
        rtcDateTime.time.seconds = second;

        // Set the RTC
        M5.Rtc.setDateTime(rtcDateTime);

        Serial.print("RTC set to: ");
        Serial.println(argv[1]);
        return EXIT_SUCCESS;
    } else {
        return badArgCount(argv[0]);
    }
}

void setupCommands() {
    shell.attach(Serial);
    shell.addCommand(F("reset"), cmd_reset);
    shell.addCommand(F("led"), cmd_led);
    shell.addCommand(F("set"), cmd_set);
    shell.addCommand(F("get"), cmd_get);
    shell.addCommand(F("exists"), cmd_exists);
    shell.addCommand(F("dir"), cmd_dir);
    shell.addCommand(F("cat"),cmd_cat);
    shell.addCommand(F("props"), cmd_props);
    shell.addCommand(F("date"), cmd_date);
}



#endif
