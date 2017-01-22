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
    OneWire * wire = new OneWire(DS_PIN);
    DallasTemperature * ds18b20 = new DallasTemperature(wire);
    ds18b20->begin();
    _ds18b20s.push_back(ds18b20);
    return _ds18b20s.size() - 1;
}

double getDSTemperature(unsigned int index) {
    if (0 <= index && index < _ds18b20s.size()) {
        _ds18b20s[index]->requestTemperatures();
        return _ds18b20s[index]->getTempCByIndex(0);
    }
    return 0;
}

unsigned int getDHTCount() {
    return _ds18b20s.size();
}

double getDSTemperature() { return getDSTemperature(0); }

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
        if (isnan(t)) {

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
                domoticzSend("dczTmpIdx", temperature);
            #endif

            // Update websocket clients
            char buffer[100];
            sprintf_P(buffer, PSTR("{\"dsVisible\": 1, \"dsTmp\": %s}"), temperature);
            wsSend(buffer);

        }

    }

}

#endif
