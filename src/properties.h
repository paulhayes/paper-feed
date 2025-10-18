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