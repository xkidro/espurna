/*

POW MODULE
Support for Sonoff POW HLW8012-based power monitor

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_HLW8012

#include <HLW8012.h>
#include <Hash.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

HLW8012 hlw8012;
bool _hlwEnabled = false;
double _energy = 0;

// -----------------------------------------------------------------------------
// POW
// -----------------------------------------------------------------------------

// When using interrupts we have to call the library entry point
// whenever an interrupt is triggered
void hlw8012_cf1_interrupt() {
    hlw8012.cf1_interrupt();
}

void hlw8012_cf_interrupt() {
    hlw8012.cf_interrupt();
}

void hlwEnable(bool status) {
    _hlwEnabled = status;
    if (_hlwEnabled) {
        #if HLW8012_USE_INTERRUPTS == 1
            attachInterrupt(getSetting("hlwCF1GPIO", HLW8012_CF1_PIN).toInt(), hlw8012_cf1_interrupt, CHANGE);
            attachInterrupt(getSetting("hlwCFGPIO", HLW8012_CF_PIN).toInt(), hlw8012_cf_interrupt, CHANGE);
        #endif
        DEBUG_MSG("[HLW8012] Enabled\n");
    } else {
        #if HLW8012_USE_INTERRUPTS == 1
            detachInterrupt(getSetting("hlwCF1GPIO", HLW8012_CF1_PIN).toInt());
            detachInterrupt(getSetting("hlwCFGPIO", HLW8012_CF_PIN).toInt());
        #endif
        DEBUG_MSG("[HLW8012] Disabled\n");
    }
}

// -----------------------------------------------------------------------------

void hlwSaveCalibration() {
    setSetting("powPowerMult", hlw8012.getPowerMultiplier());
    setSetting("powCurrentMult", hlw8012.getCurrentMultiplier());
    setSetting("powVoltageMult", hlw8012.getVoltageMultiplier());
}

void hlwRetrieveCalibration() {
    double value;
    value = getSetting("powPowerMult", 0).toFloat();
    if (value > 0) hlw8012.setPowerMultiplier((int) value);
    value = getSetting("powCurrentMult", 0).toFloat();
    if (value > 0) hlw8012.setCurrentMultiplier((int) value);
    value = getSetting("powVoltageMult", 0).toFloat();
    if (value > 0) hlw8012.setVoltageMultiplier((int) value);
}

void hlwSetExpectedActivePower(unsigned int power) {
    if (power > 0) {
        hlw8012.expectedActivePower(power);
        hlwSaveCalibration();
    }
}

void hlwSetExpectedCurrent(double current) {
    if (current > 0) {
        hlw8012.expectedCurrent(current);
        hlwSaveCalibration();
    }
}

void hlwSetExpectedVoltage(unsigned int voltage) {
    if (voltage > 0) {
        hlw8012.expectedVoltage(voltage);
        hlwSaveCalibration();
    }
}

void hlwReset() {
    hlw8012.resetMultipliers();
    hlwSaveCalibration();
}

// -----------------------------------------------------------------------------

unsigned int getActivePower() {
    return hlw8012.getActivePower();
}

unsigned int getApparentPower() {
    return hlw8012.getApparentPower();
}

unsigned int getReactivePower() {
    return hlw8012.getReactivePower();
}

double getCurrent() {
    return hlw8012.getCurrent();
}

unsigned int getVoltage() {
    return hlw8012.getVoltage();
}

unsigned int getPowerFactor() {
    return (int) (100 * hlw8012.getPowerFactor());
}

double getEnergy() {
    return _energy;
}

// -----------------------------------------------------------------------------

void retrieveEnergy() {
    unsigned long energy = EEPROM.read(EEPROM_POWER_COUNT + 1);
    energy = (energy << 8) + EEPROM.read(EEPROM_POWER_COUNT);
    if (energy == 0xFFFF) energy = 0;
    _energy = energy;
}

void saveEnergy() {
    unsigned int energy = (int) _energy;
    EEPROM.write(EEPROM_POWER_COUNT, energy & 0xFF);
    EEPROM.write(EEPROM_POWER_COUNT + 1, (energy >> 8) & 0xFF);
    EEPROM.commit();
}

void hlwSetup() {

    // Initialize HLW8012
    // void begin(unsigned char cf_pin, unsigned char cf1_pin, unsigned char sel_pin, unsigned char currentWhen = HIGH, bool use_interrupts = false, unsigned long pulse_timeout = PULSE_TIMEOUT);
    // * cf_pin, cf1_pin and sel_pin are GPIOs to the HLW8012 IC
    // * currentWhen is the value in sel_pin to select current sampling
    // * set use_interrupts to true to use interrupts to monitor pulse widths
    // * leave pulse_timeout to the default value, recommended when using interrupts
    #if HLW8012_USE_INTERRUPTS
        hlw8012.begin(
            getSetting("hlwCFGPIO", HLW8012_CF_PIN).toInt(),
            getSetting("hlwCF1GPIO", HLW8012_CF1_PIN).toInt(),
            getSetting("hlwSELGPIO", HLW8012_SEL_PIN).toInt(),
            getSetting("hlwSELMode", HLW8012_SEL_CURRENT).toInt(),
            true
        );
    #else
        hlw8012.begin(
            getSetting("hlwCFGPIO", HLW8012_CF_PIN).toInt(),
            getSetting("hlwCF1GPIO", HLW8012_CF1_PIN).toInt(),
            getSetting("hlwSELGPIO", HLW8012_SEL_PIN).toInt(),
            getSetting("hlwSELMode", HLW8012_SEL_CURRENT).toInt(),
            false,
            1000000
        );
    #endif

    // These values are used to calculate current, voltage and power factors as per datasheet formula
    // These are the nominal values for the Sonoff POW resistors:
    // * The CURRENT_RESISTOR is the 1milliOhm copper-manganese resistor in series with the main line
    // * The VOLTAGE_RESISTOR_UPSTREAM are the 5 470kOhm resistors in the voltage divider that feeds the V2P pin in the HLW8012
    // * The VOLTAGE_RESISTOR_DOWNSTREAM is the 1kOhm resistor in the voltage divider that feeds the V2P pin in the HLW8012
    hlw8012.setResistors(
        getSetting("hlwCurrRes", HLW8012_CURRENT_R).toInt(),
        getSetting("hlwVoltResUp", HLW8012_VOLTAGE_R_UP).toInt(),
        getSetting("hlwVoltResDown", HLW8012_VOLTAGE_R_DOWN).toInt()
    );

    // Retrieve calibration values
    hlwRetrieveCalibration();

    // Recover energy reading
    retrieveEnergy();

    // API definitions
    apiRegister("/api/power", "power", [](char * buffer, size_t len) {
        snprintf(buffer, len, "%d", getActivePower());
    });
    apiRegister("/api/energy", "energy", [](char * buffer, size_t len) {
        snprintf(buffer, len, "%ld", (unsigned long) _energy);
    });
    apiRegister("/api/current", "current", [](char * buffer, size_t len) {
        dtostrf(getCurrent(), len-1, 2, buffer);
    });
    apiRegister("/api/voltage", "voltage", [](char * buffer, size_t len) {
        snprintf(buffer, len, "%d", getVoltage());
    });

}

void hlwLoop() {

    static unsigned long last_update = 0;
    static unsigned char report_count = HLW8012_REPORT_EVERY;

    static unsigned long power_sum = 0;
    static double current_sum = 0;
    static unsigned long voltage_sum = 0;
    static bool powWasEnabled = false;

    // POW is disabled while there is no internet connection
    // When the HLW8012 measurements are enabled back we reset the timer
    if (!_hlwEnabled) {
        powWasEnabled = false;
        return;
    }
    if (!powWasEnabled) {
        last_update = millis();
        powWasEnabled = true;
    }

    if (millis() - last_update > HLW8012_UPDATE_INTERVAL) {

        last_update = millis();

        unsigned int power = getActivePower();
        unsigned int voltage = getVoltage();
        double current = getCurrent();
        unsigned int apparent = getApparentPower();
        unsigned int factor = getPowerFactor();
        unsigned int reactive = getReactivePower();

        power_sum += power;
        current_sum += current;
        voltage_sum += voltage;

        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();

        root["powVisible"] = 1;
        root["powActivePower"] = power;
        root["powCurrent"] = current;
        root["powVoltage"] = voltage;
        root["powApparentPower"] = apparent;
        root["powReactivePower"] = reactive;
        root["powPowerFactor"] = factor;

        String output;
        root.printTo(output);
        wsSend(output.c_str());

        if (--report_count == 0) {

            power = power_sum / HLW8012_REPORT_EVERY;
            current = current_sum / HLW8012_REPORT_EVERY;
            voltage = voltage_sum / HLW8012_REPORT_EVERY;
            apparent = current * voltage;
            reactive = (apparent > power) ? sqrt(apparent * apparent - power * power) : 0;
            factor = (apparent > 0) ? 100 * power / apparent : 100;
            if (factor > 100) factor = 100;
            double window = (double) HLW8012_REPORT_EVERY * HLW8012_UPDATE_INTERVAL / 1000.0 / 3600.0;
            _energy += power * window;
            saveEnergy();

            mqttSend(getSetting("powPowerTopic", HLW8012_POWER_TOPIC).c_str(), String(power).c_str());
            mqttSend(getSetting("powEnergyTopic", HLW8012_ENERGY_TOPIC).c_str(), String(_energy).c_str());
            mqttSend(getSetting("powCurrentTopic", HLW8012_CURRENT_TOPIC).c_str(), String(current).c_str());
            mqttSend(getSetting("powVoltageTopic", HLW8012_VOLTAGE_TOPIC).c_str(), String(voltage).c_str());
            mqttSend(getSetting("powAPowerTopic", HLW8012_APOWER_TOPIC).c_str(), String(apparent).c_str());
            mqttSend(getSetting("powRPowerTopic", HLW8012_RPOWER_TOPIC).c_str(), String(reactive).c_str());
            mqttSend(getSetting("powPFactorTopic", HLW8012_PFACTOR_TOPIC).c_str(), String(factor).c_str());

            #if ENABLE_DOMOTICZ
            {
                char buffer[20];
                snprintf(buffer, 20, "%d;%ld", power, (unsigned long) _energy);
                domoticzSend("dczPowIdx", 0, buffer);
            }
            #endif

            power_sum = current_sum = voltage_sum = 0;
            report_count = HLW8012_REPORT_EVERY;

        }

        // Toggle between current and voltage monitoring
        #if HLW8012_USE_INTERRUPTS == 0
            hlw8012.toggleMode();
        #endif

    }

}

#endif
