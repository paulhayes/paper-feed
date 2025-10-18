#include "main.h"
#include <SimpleSerialShell.h>
#include <Preferences.h>
#include "properties.h"

#ifndef COMMANDS_H
#define COMMANDS_H

#define MAX_PROPERTIES 32

property_t properties[MAX_PROPERTIES];
int propertyCount = 0;
Preferences prefs;

void registerProperty(const char* name, void* value, PropertyType type) {
    if(propertyCount >= MAX_PROPERTIES) return;

    property_t &prop = properties[propertyCount];
    strncpy(prop.propertyName, name, 31);
    prop.propertyName[31] = '\0';
    prop.propertyValue = value;
    prop.propertyType = type;

    prefs.begin("settings", true);
    if(prefs.isKey(name)) {
        switch(type) {
            case STRING:
                *(String*)value = prefs.getString(name);
                break;
            case FLOAT:
                *(float*)value = prefs.getFloat(name);
                break;
            case INT:
                *(int*)value = prefs.getInt(name);
                break;
        }
    }
    prefs.end();

    propertyCount++;
}

property_t* findProperty(const char* name) {
    for(int i = 0; i < propertyCount; i++) {
        if(strcmp(properties[i].propertyName, name) == 0) {
            return &properties[i];
        }
    }
    return nullptr;
}

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

int cmd_dir(int argc, char **argv) {
    const char* path = "/";
    if(argc >= 2) {
        path = argv[1];
    }

    File root = SPIFFS.open(path);
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

void setupCommands() {
    shell.attach(Serial);
    shell.addCommand(F("reset"), cmd_reset);
    shell.addCommand(F("led"), cmd_led);
    shell.addCommand(F("set"), cmd_set);
    shell.addCommand(F("get"), cmd_get);
    shell.addCommand(F("exists"), cmd_exists);
    shell.addCommand(F("dir"), cmd_dir);
}



#endif
