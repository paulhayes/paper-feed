#ifndef PROPS_H
#define PROPS_H

#define MAX_PROPERTIES 32

enum PropertyType {
    STRING,
    FLOAT,
    INT
};

typedef struct {
    char propertyName[32];
    void* propertyValue;
    PropertyType propertyType;
} property_t;


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

#endif