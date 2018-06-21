// -----------------------------------------------------------------------------
// ECH1560 based power monitor
// Copyright (C) 2017-2018 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && ECH1560_SUPPORT

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"
#include "../config/debug.h"

class ECH1560Sensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        ECH1560Sensor(): BaseSensor(), _data() {
            _count = 3;
            _sensor_id = SENSOR_ECH1560_ID;
        }

        ~ECH1560Sensor() {
            _enableInterrupts(false);
        }

        // ---------------------------------------------------------------------

        void setCLK(unsigned char clk) {
            if (_clk == clk) return;
            _clk = clk;
            _dirty = true;
        }

        void setMISO(unsigned char miso) {
            if (_miso == miso) return;
            _miso = miso;
            _dirty = true;
        }

        void setInverted(bool inverted) {
            _inverted = inverted;
        }

        // ---------------------------------------------------------------------

        unsigned char getCLK() {
            return _clk;
        }

        unsigned char getMISO() {
            return _miso;
        }

        bool getInverted() {
            return _inverted;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;

            pinMode(_clk, INPUT);
            pinMode(_miso, INPUT);
            _enableInterrupts(true);

            _dirty = false;
            _ready = true;

        }

        void pre() {
            DEBUG_MSG("_clk_high_start: %lu\n", _clk_high_start);
            DEBUG_MSG("_pulse_width_max: %lu\n", _pulse_width_max);
            _pulse_width_max=0;
        }

        // Loop-like method, call it in your main loop
        void tick() {
            if (_dodecode) _decode();
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        String description() {
            char buffer[35];
            snprintf(buffer, sizeof(buffer), "ECH1560 (CLK,SDO) @ GPIO(%u,%u)", _clk, _miso);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            char buffer[6];
            snprintf(buffer, sizeof(buffer), "%u:%u", _clk, _miso);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_CURRENT;
            if (index == 1) return MAGNITUDE_VOLTAGE;
            if (index == 2) return MAGNITUDE_POWER_APPARENT;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _current;
            if (index == 1) return _voltage;
            if (index == 2) return _apparent;
            return 0;
        }

        void ICACHE_RAM_ATTR (unsigned char gpio) {

            (void) gpio;

            //noInterrupts();

            bool clock_high = (digitalRead(_clk) == HIGH);

            if (clock_high) {

                _clk_high_start = micros();

            } else {

                unsigned long pulse_width = micros() - _clk_high_start;
                if (pulse_width > _pulse_width_max) _pulse_width_max = pulse_width;
                // if the Clk was high between 1 and 2 ms than, its a start of a SPI-transmission
                if (1000 <= pulse_width && pulse_width <= 2000) {
                    _data_bit = _data_byte = 0;
                    for (unsigned char i=0; i<16; i++) _data[i] = 0;
                    _dosync = true;
                    //DEBUG_MSG("[ECH1560] start sync\n");
                }

            }

            // if we are trying to find the sync-time (CLK goes high for 1-2ms)
            if (_dosync & clock_high) {

                if (digitalRead(_miso) == HIGH) {
                    _data[_data_byte] = _data[_data_byte] | (1 << _data_bit);
                }

                ++_data_bit;
                if (8 == _data_bit) {
                    _data_bit = 0;
                    ++_data_byte;
                    if (16 == _data_byte) {
                        _dodecode = true;
                        _dosync = false;
                    }
                }

            }

            //interrupts();

        }

    protected:

        // ---------------------------------------------------------------------
        // Interrupt management
        // ---------------------------------------------------------------------

        void _attach(ECH1560Sensor * instance, unsigned char gpio, unsigned char mode);
        void _detach(unsigned char gpio);

        void _enableInterrupts(bool value) {

            static unsigned char _interrupt_clk = GPIO_NONE;

            if (value) {
                if (_interrupt_clk != _clk) {
                    if (_interrupt_clk != GPIO_NONE) _detach(_interrupt_clk);
                    _attach(this, _clk, CHANGE);
                    _interrupt_clk = _clk;
                }
            } else if (_interrupt_clk != GPIO_NONE) {
                _detach(_interrupt_clk);
                _interrupt_clk = GPIO_NONE;
            }

        }

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _decode() {

            _dodecode = false;

            DEBUG_MSG("_decode():\n");
            for (unsigned char i=5; i<16; i++) {
                DEBUG_MSG(" %02X", _data[i]);
                _data[i] = 0;
            }
            DEBUG_MSG("\n");

            return;

            unsigned char byte1 = 0;
            unsigned char byte2 = 0;
            unsigned char byte3 = 0;

            _bits_count = 0;
            while (_bits_count < 40); // skip the uninteresting 5 first bytes
            _bits_count = 0;

            while (_bits_count < 24) { // loop through the next 3 Bytes (6-8) and save byte 6 and 7 in byte1 and byte2

                if (_nextbit) {

                    if (_bits_count < 9) { // first Byte/8 bits in byte1

                        byte1 = byte1 << 1;
                        if (digitalRead(_miso) == HIGH) byte1 |= 1;
                        _nextbit = false;

                    } else if (_bits_count < 17) { // bit 9-16 is byte 7, store in byte2

                        byte2 = byte2 << 1;
                        if (digitalRead(_miso) == HIGH) byte2 |= 1;
                        _nextbit = false;

                    }

                }

            }

            DEBUG_MSG("[ECH1560] byte[1.1] = %u\n", byte1);
            DEBUG_MSG("[ECH1560] byte[1.2] = %u\n", byte2);

            if (byte2 != 3) { // if bit byte2 is not 3, we have reached the important part, U is allready in byte1 and byte2 and next 8 Bytes will give us the Power.

                // voltage = 2 * (byte1 + byte2 / 255)
                _voltage = 2.0 * ((float) byte1 + (float) byte2 / 255.0);

                // power:
                _bits_count = 0;
                while (_bits_count < 40); // skip the uninteresting 5 first bytes
                _bits_count = 0;

                byte1 = 0;
                byte2 = 0;
                byte3 = 0;

                while (_bits_count < 24) { //store byte 6, 7 and 8 in byte1 and byte2 & byte3.

                    if (_nextbit) {

                        if (_bits_count < 9) {

                            byte1 = byte1 << 1;
                            if (digitalRead(_miso) == HIGH) byte1 |= 1;
                            _nextbit = false;

                        } else if (_bits_count < 17) {

                            byte2 = byte2 << 1;
                            if (digitalRead(_miso) == HIGH) byte2 |= 1;
                            _nextbit = false;

                        } else {

                            byte3 = byte3 << 1;
                            if (digitalRead(_miso) == HIGH) byte3 |= 1;
                            _nextbit = false;

                        }
                    }
                }

                DEBUG_MSG("[ECH1560] byte[2.1] = %u\n", byte1);
                DEBUG_MSG("[ECH1560] byte[2.2] = %u\n", byte2);
                DEBUG_MSG("[ECH1560] byte[2.3] = %u\n", byte3);

                if (_inverted) {
                    byte1 = 255 - byte1;
                    byte2 = 255 - byte2;
                    byte3 = 255 - byte3;
                }

                // power = (byte1*255+byte2+byte3/255)/2
                _apparent = ( (float) byte1 * 255 + (float) byte2 + (float) byte3 / 255.0) / 2;
                _current = _apparent / _voltage;

                _dosync = false;

            }

            // If byte2 is not 3 or something else than 0, something is wrong!
            if (byte2 == 0) {
                _dosync = false;
            #if SENSOR_DEBUG
                DEBUG_MSG("Nothing connected, or out of sync!\n");
            #endif
            }
        }

        // ---------------------------------------------------------------------

        unsigned char _clk = 0;
        unsigned char _miso = 0;
        bool _inverted = false;

        volatile unsigned long _clk_high_start = 0;
        volatile long _bits_count = 0;
        volatile bool _dosync = false;
        volatile bool _nextbit = true;

        volatile unsigned char _data[16];
        volatile unsigned char _data_byte = 0;
        volatile unsigned char _data_bit = 0;
        volatile bool _dodecode = false;

        volatile unsigned long _pulse_width_max = 0;

        double _apparent = 0;
        double _voltage = 0;
        double _current = 0;

};

// -----------------------------------------------------------------------------
// Interrupt helpers
// -----------------------------------------------------------------------------

ECH1560Sensor * _ech1560_sensor_instance[10] = {NULL};

void ICACHE_RAM_ATTR _ech1560_sensor_isr(unsigned char gpio) {
    unsigned char index = gpio > 5 ? gpio-6 : gpio;
    if (_ech1560_sensor_instance[index]) {
        _ech1560_sensor_instance[index]->handleInterrupt(gpio);
    }
}

void ICACHE_RAM_ATTR _ech1560_sensor_isr_0() { _ech1560_sensor_isr(0); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_1() { _ech1560_sensor_isr(1); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_2() { _ech1560_sensor_isr(2); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_3() { _ech1560_sensor_isr(3); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_4() { _ech1560_sensor_isr(4); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_5() { _ech1560_sensor_isr(5); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_12() { _ech1560_sensor_isr(12); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_13() { _ech1560_sensor_isr(13); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_14() { _ech1560_sensor_isr(14); }
void ICACHE_RAM_ATTR _ech1560_sensor_isr_15() { _ech1560_sensor_isr(15); }

static void (*_ech1560_sensor_isr_list[10])() = {
    _ech1560_sensor_isr_0, _ech1560_sensor_isr_1, _ech1560_sensor_isr_2,
    _ech1560_sensor_isr_3, _ech1560_sensor_isr_4, _ech1560_sensor_isr_5,
    _ech1560_sensor_isr_12, _ech1560_sensor_isr_13, _ech1560_sensor_isr_14,
    _ech1560_sensor_isr_15
};

void ECH1560Sensor::_attach(ECH1560Sensor * instance, unsigned char gpio, unsigned char mode) {
    if (!gpioValid(gpio)) return;
    _detach(gpio);
    unsigned char index = gpio > 5 ? gpio-6 : gpio;
    _ech1560_sensor_instance[index] = instance;
    attachInterrupt(gpio, _ech1560_sensor_isr_list[index], mode);
    #if SENSOR_DEBUG
        DEBUG_MSG("[SENSOR] GPIO%d interrupt attached to %s\n", gpio, instance->description().c_str());
    #endif
}

void ECH1560Sensor::_detach(unsigned char gpio) {
    if (!gpioValid(gpio)) return;
    unsigned char index = gpio > 5 ? gpio-6 : gpio;
    if (_ech1560_sensor_instance[index]) {
        detachInterrupt(gpio);
        #if SENSOR_DEBUG
            DEBUG_MSG("[SENSOR] GPIO%d interrupt detached from %s\n", gpio, _ech1560_sensor_instance[index]->description().c_str());
        #endif
        _ech1560_sensor_instance[index] = NULL;
    }
}

#endif // SENSOR_SUPPORT && ECH1560_SUPPORT
