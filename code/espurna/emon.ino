/*

EMON MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_EMON

#include <EmonLiteESP.h>
#include "brzo_i2c.h"
#include <EEPROM.h>

// ADC121 Registers
#define ADC121_REG_RESULT       0x00
#define ADC121_REG_ALERT        0x01
#define ADC121_REG_CONFIG       0x02
#define ADC121_REG_LIMITL       0x03
#define ADC121_REG_LIMITH       0x04
#define ADC121_REG_HYST         0x05
#define ADC121_REG_CONVL        0x06
#define ADC121_REG_CONVH        0x07

// -----------------------------------------------------------------------------
// Cache
// -----------------------------------------------------------------------------

EmonLiteESP emon;
unsigned int _emonProvider = EMON_ANALOG_PROVIDER;
bool _emonEnabled = false;
double _emonCurrent = 0;
unsigned int _emonPower = 0;
unsigned char _emonAddr;


// -----------------------------------------------------------------------------
// Provider
// -----------------------------------------------------------------------------

unsigned int emonAnalogCallback() {
    return analogRead(_emonAddr);
}

unsigned int emonADC121Callback() {
    uint8_t buffer[2];
    brzo_i2c_start_transaction(_emonAddr, I2C_SCL_FREQUENCY);
    buffer[0] = ADC121_REG_RESULT;
    brzo_i2c_write(buffer, 1, false);
    brzo_i2c_read(buffer, 2, false);
    brzo_i2c_end_transaction();
    unsigned int value;
    value = (buffer[0] & 0x0F) << 8;
    value |= buffer[1];
    return value;
}

// -----------------------------------------------------------------------------
// EMON
// -----------------------------------------------------------------------------

void emonSetCurrentRatio(float value) {
    emon.setCurrentRatio(value);
}

unsigned int emonGetApparentPower() {
    return _emonPower;
}

double emonGetCurrent() {
    return _emonCurrent;
}

bool emonEnabled() {
    return _emonEnabled;
}

void emonSetup() {

    _emonEnabled = getSetting("emonEnabled", 0).toInt() == 1;
    if (!_emonEnabled) return;

    _emonProvider = getSetting("emonProvider", EMON_ANALOG_PROVIDER).toInt();
    if ((_emonProvider == EMON_ADC121_PROVIDER) && (!i2cEnabled())) _emonEnabled = false;
    if (!_emonEnabled) return;

    _emonAddr = getSetting("emonAddr", _emonProvider == EMON_ANALOG_PROVIDER ? EMON_INT_ADDRESS : EMON_ADC121_ADDRESS).toInt();

    emon.initCurrent(
        _emonProvider == EMON_ANALOG_PROVIDER ? emonAnalogCallback : emonADC121Callback,
        _emonProvider == EMON_ANALOG_PROVIDER ? EMON_INT_ADC_BITS : EMON_ADC121_ADC_BITS,
        _emonProvider == EMON_ANALOG_PROVIDER ? EMON_INT_REF_VOLTAGE : EMON_ADC121_REF_VOLTAGE,
        getSetting("emonRatio", EMON_CURRENT_RATIO).toFloat()
    );
    emon.setPrecision(_emonProvider == EMON_ANALOG_PROVIDER ? EMON_INT_CURR_PRECISION : EMON_ADC121_CURR_PRECISION);

    if (_emonProvider == EMON_ADC121_PROVIDER) {
        uint8_t buffer[2];
        buffer[0] = ADC121_REG_CONFIG;
        buffer[1] = 0x00;
        brzo_i2c_start_transaction(_emonAddr, I2C_SCL_FREQUENCY);
        brzo_i2c_write(buffer, 2, false);
        brzo_i2c_end_transaction();
    }

    apiRegister("/api/power", "power", [](char * buffer, size_t len) {
        snprintf(buffer, len, "%d", _emonPower);
    });

    DEBUG_MSG("[EMON] EMON enabled with provider #%d\n", _emonProvider);

}

void emonLoop() {

    if (!_emonEnabled) return;

    static unsigned long next_measurement = millis();
    static bool warmup = true;
    static byte measurements = 0;
    static double max = 0;
    static double min = 0;
    static double sum = 0;

    if (warmup) {
        warmup = false;
        emon.warmup();
    }

    if (millis() > next_measurement) {

        // Safety check: do not read current if relay is OFF
        // You could be monitoring another line with the current clamp...
        //if (!relayStatus(0)) {
        //    _emonCurrent = 0;
        //} else {
            _emonCurrent = emon.getCurrent(EMON_SAMPLES);
            _emonCurrent = _emonCurrent - (_emonProvider == EMON_ANALOG_PROVIDER ? EMON_INT_CURR_OFFSET : EMON_ADC121_CURR_OFFSET);
            if (_emonCurrent < 0) _emonCurrent = 0;
        //}

        if (measurements == 0) {
            max = min = _emonCurrent;
        } else {
            if (_emonCurrent > max) max = _emonCurrent;
            if (_emonCurrent < min) min = _emonCurrent;
        }
        sum += _emonCurrent;
        ++measurements;

        float mainsVoltage = getSetting("emonMains", EMON_MAINS_VOLTAGE).toFloat();

        char current_buf[6];
        dtostrf(_emonCurrent, 5, 2, current_buf);
        char *c = current_buf;
        while ((unsigned char) *c == ' ') ++c;

        DEBUG_MSG("[EMON] Current: %sA\n", c);
        DEBUG_MSG("[EMON] Power: %dW\n", int(_emonCurrent * mainsVoltage));

        // Update websocket clients
        char text[64];
        sprintf_P(text, PSTR("{\"emonVisible\": 1, \"powApparentPower\": %d}"), int(_emonCurrent * mainsVoltage));
        wsSend(text);

        // Send MQTT messages averaged every EMON_MEASUREMENTS
        if (measurements == EMON_MEASUREMENTS) {

            _emonPower = (int) ((sum - max - min) * mainsVoltage / (measurements - 2));
            char power_buf[6];
            snprintf(power_buf, 6, "%d", _emonPower);

            double energy_inc = (double) _emonPower * EMON_INTERVAL * EMON_MEASUREMENTS / 1000.0 / 3600.0;
            char energy_buf[10];
            dtostrf(energy_inc, 9, 2, energy_buf);
            char *e = energy_buf;
            while ((unsigned char) *e == ' ') ++e;

            mqttSend(getSetting("emonPowerTopic", EMON_APOWER_TOPIC).c_str(), power_buf);
            mqttSend(getSetting("emonEnergyTopic", EMON_ENERGY_TOPIC).c_str(), e);

            #if ENABLE_DOMOTICZ
            {
                char buffer[20];
                snprintf(buffer, 20, "%s;%s", power_buf, e);
                domoticzSend("dczPowIdx", 0, buffer);
            }
            #endif

            sum = measurements = 0;

        }

        next_measurement += EMON_INTERVAL;

    }

}

#endif
