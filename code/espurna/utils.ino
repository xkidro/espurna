/*

UTILS MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

String getIdentifier() {
    char identifier[20];
    sprintf(identifier, "ESPURNA_%06X", ESP.getChipId());
    return String(identifier);
}

float Celsius2Fahrenheit(float celsius) {
    return 32.0 + celsius * 9 / 5;
}
