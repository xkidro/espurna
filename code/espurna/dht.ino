/*

DHT MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_DHT

#include <DHT.h>
#include <Adafruit_Sensor.h>

// -----------------------------------------------------------------------------
// Cache
// -----------------------------------------------------------------------------

double _dhtTemperature = 0;
unsigned int _dhtHumidity = 0;

// -----------------------------------------------------------------------------
// Provider
// -----------------------------------------------------------------------------

std::vector<DHT *> _dhts;

unsigned int createDHT(unsigned int pin, unsigned int type) {
    DHT * dht = new DHT(pin, type, DHT_TIMING);
    dht->begin();
    _dhts.push_back(dht);
    return _dhts.size() - 1;
}

double getDHTTemperature(unsigned int index) {
    if (0 <= index && index < _dhts.size()) {
        return _dhts[index]->readTemperature();
    }
    return 0;
}

unsigned int getDHTHumidity(unsigned int index) {
    if (0 <= index && index < _dhts.size()) {
        return _dhts[index]->readHumidity();
    }
    return 0;
}

unsigned int getDHTCount() {
    return _dhts.size();
}

double getDHTTemperature() { return getDHTTemperature(0); }
unsigned int getDHTHumidity() { return getDHTHumidity(0); }

// -----------------------------------------------------------------------------
// Setup & Loop
// -----------------------------------------------------------------------------

void dhtSetup() {

    createDHT(DHT_PIN, DHT_TYPE);

    apiRegister("/api/temperature", "temperature", [](char * buffer, size_t len) {
        dtostrf(_dhtTemperature, len-1, 1, buffer);
    });
    apiRegister("/api/humidity", "humidity", [](char * buffer, size_t len) {
        snprintf(buffer, len, "%d", _dhtHumidity);
    });

}

void dhtLoop() {

    // Check if we should read new data
    static unsigned long last_update = 0;
    if ((millis() - last_update > DHT_UPDATE_INTERVAL) || (last_update == 0)) {
        last_update = millis();

        // Read sensor data
        double t = getDHTTemperature();
        unsigned int h = getDHTHumidity();

        // Check if readings are valid
        if (isnan(h) || isnan(t)) {

            DEBUG_MSG("[DHT] Error reading sensor\n");

        } else {

            _dhtTemperature = t;
            _dhtHumidity = h;

            char temperature[6];
            char humidity[6];
            dtostrf(t, 4, 1, temperature);
            itoa(h, humidity, 10);

            DEBUG_MSG("[DHT] Temperature: %s\n", temperature);
            DEBUG_MSG("[DHT] Humidity: %s\n", humidity);

            // Send MQTT messages
            mqttSend(getSetting("dhtTmpTopic", DHT_TEMPERATURE_TOPIC).c_str(), temperature);
            mqttSend(getSetting("dhtHumTopic", DHT_HUMIDITY_TOPIC).c_str(), humidity);

            // Send to Domoticz
            #if ENABLE_DOMOTICZ
            {
                domoticzSend("dczTmpIdx", 0, temperature);
                int status;
                if (h > 70) {
                    status = HUMIDITY_WET;
                } else if (h > 45) {
                    status = HUMIDITY_COMFORTABLE;
                } else if (h > 30) {
                    status = HUMIDITY_NORMAL;
                } else {
                    status = HUMIDITY_DRY;
                }
                char buffer[2];
                sprintf(buffer, "%d", status);
                domoticzSend("dczHumIdx", humidity, buffer);
            }
            #endif

            // Update websocket clients
            char buffer[100];
            sprintf_P(buffer, PSTR("{\"dhtVisible\": 1, \"dhtTmp\": %s, \"dhtHum\": %s}"), temperature, humidity);
            wsSend(buffer);

        }

    }

}

#endif
