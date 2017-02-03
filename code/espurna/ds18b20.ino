/*

DS18B20 MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_DS18B20

#include <OneWire.h>
#include <DallasTemperature.h>

OneWire * ds18b20Wire;
DallasTemperature * ds18b20;

// -----------------------------------------------------------------------------
// Cache
// -----------------------------------------------------------------------------

double _dsTemperature = 0;

// -----------------------------------------------------------------------------
// Provider
// -----------------------------------------------------------------------------

std::vector<DallasTemperature *> _ds18b20s;

unsigned int createDS18B20(unsigned int pin) {

    OneWire * wire = new OneWire(pin);
    DallasTemperature * ds18b20 = new DallasTemperature(wire);
    ds18b20->begin();

    #ifdef DEBUG_PORT
        DEBUG_MSG("[SENSORS] GPIO %d, found %d DS18B20 devices\n", pin, ds18b20->getDeviceCount());
        DEBUG_MSG("[SENSORS] GPIO %d, DS18B20 parasite power %s\n", pin, ds18b20->isParasitePowerMode() ? "ON" : "OFF");
        DeviceAddress address;
        for (int i=0; i<ds18b20->getDeviceCount(); i++) {
            ds18b20->getAddress(address, i);
            DEBUG_MSG("[SENSORS] GPIO %d, DS18B20 #%d address: ", pin, i);
            for (uint8_t i = 0; i < 8; i++) {
                DEBUG_PORT.printf("%02X", address[i]);
            }
            DEBUG_MSG("\n");
        }
    #endif

    _ds18b20s.push_back(ds18b20);
    return _ds18b20s.size() - 1;

}

bool getDSAddress(uint8_t * address, unsigned int index) {
    unsigned int aggregated = 0;
    unsigned connectionId = 0;
    while (connectionId <= _ds18b20s.size()) {
        unsigned int count = _ds18b20s[connectionId]->getDeviceCount();
        if (aggregated + count > index) {
            _ds18b20s[connectionId]->getAddress(address, index - aggregated);
            return true;
        }
        ++connectionId;
    }
    return false;
}

double getDSTemperature(unsigned int connectionId, unsigned int index) {
    if (0 <= connectionId && connectionId < _ds18b20s.size()) {
        _ds18b20s[connectionId]->requestTemperatures();
        return _ds18b20s[connectionId]->getTempCByIndex(index);
    }
    return DEVICE_DISCONNECTED_C;
}

double getDSTemperatureByAddress(uint8_t * address) {
    for (int i=0; i<_ds18b20s.size(); i++) {
        _ds18b20s[i]->requestTemperatures();
        double t = _ds18b20s[i]->getTempC(address);
        if (t != DEVICE_DISCONNECTED_C) return t;
    }
    return DEVICE_DISCONNECTED_C;
}


double getDSTemperature(unsigned int index) {
    unsigned int aggregated = 0;
    unsigned connectionId = 0;
    while (connectionId <= _ds18b20s.size()) {
        unsigned int count = _ds18b20s[connectionId]->getDeviceCount();
        if (aggregated + count > index) {
            return getDSTemperature(connectionId, index - aggregated);
        }
        ++connectionId;
    }
    return DEVICE_DISCONNECTED_C;
}

double getDSTemperature() {
    return getDSTemperature(0, 0);
}

unsigned int getDSCount() {
    int count = 0;
    for (int i=0; i<_ds18b20s.size(); i++) {
        count += _ds18b20s[i]->getDeviceCount();
    }
    return count;
}

// -----------------------------------------------------------------------------
// Setup & Loop
// -----------------------------------------------------------------------------

void dsSetup() {

    createDS18B20(DS_PIN);

    apiRegister("/api/temperature", "temperature", [](char * buffer, size_t len) {
        dtostrf(_dsTemperature, len-1, 1, buffer);
    });

}

void dsLoop() {

    if (!mqttConnected()) return;

    // Check if we should read new data
    static unsigned long last_update = 0;
    if ((millis() - last_update > DS_UPDATE_INTERVAL) || (last_update == 0)) {
        last_update = millis();

        // Read sensor data
        double t = getDSTemperature(0);

        // Check if readings are valid
        if (t == DEVICE_DISCONNECTED_C) {

            DEBUG_MSG("[DS18B20] Error reading sensor\n");

        } else {

            _dsTemperature = t;

            char temperature[6];
            dtostrf(t, 5, 1, temperature);
            DEBUG_MSG("[DS18B20] Temperature: %s\n", temperature);

            // Send MQTT messages
            mqttSend(getSetting("dsTmpTopic", DS_TEMPERATURE_TOPIC).c_str(), temperature);

            // Send to Domoticz
            #if ENABLE_DOMOTICZ
                domoticzSend("dczTmpIdx", 0, temperature);
            #endif

            // Update websocket clients
            char buffer[100];
            sprintf_P(buffer, PSTR("{\"dsVisible\": 1, \"dsTmp\": %s}"), temperature);
            wsSend(buffer);

        }

    }

}

#endif
