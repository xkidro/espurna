/*

I2C MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_I2C

#include "brzo_i2c.h"

// -----------------------------------------------------------------------------
// Cache
// -----------------------------------------------------------------------------

bool _i2cEnabled = false;

// -----------------------------------------------------------------------------
// Methods
// -----------------------------------------------------------------------------

void i2cScan() {

    uint8_t address;
    uint8_t response;
    uint8_t buffer[1];
    int nDevices = 0;

    for (address = 1; address < 128; address++) {

        brzo_i2c_start_transaction(address, I2C_SCL_FREQUENCY);
        brzo_i2c_ACK_polling(1000);
        response = brzo_i2c_end_transaction();

        if (response == 0) {
            DEBUG_MSG("[I2C] Device found at address 0x%02X\n", address);
            nDevices++;
        } else if (response != 32) {
            //DEBUG_MSG("[I2C] Unknown error at address 0x%02X\n", address);
        }
    }

    if (nDevices == 0) DEBUG_MSG("[I2C] No devices found");

}

bool i2cEnabled() {
    return _i2cEnabled;
}

void i2cSetup() {

    _i2cEnabled = getSetting("i2cEnabled", 0).toInt() == 1;
    if (!_i2cEnabled) return;

    unsigned char sdaPin, sclPin;

    brzo_i2c_setup(
        sdaPin = getSetting("i2cSDAGPIO", I2C_SDA_PIN).toInt(),
        sclPin = getSetting("i2cSCLGPIO", I2C_SCL_PIN).toInt(),
        I2C_CLOCK_STRETCH_TIME
    );

    DEBUG_MSG("[I2C] I2C enabled in %d (SDA) and %d (SCL)\n", sdaPin, sclPin);

    i2cScan();

}

#endif
