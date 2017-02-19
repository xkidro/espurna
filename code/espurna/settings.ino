/*

SETTINGS MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "Embedis.h"
#include <EEPROM.h>
#include "spi_flash.h"
#include <StreamString.h>

#define AUTO_SAVE 0

Embedis embedis(Serial);

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------

unsigned long settingsSize() {
    bool zero = false;
    unsigned long size = 0;
    for (unsigned int i = SPI_FLASH_SEC_SIZE; i>0; i--) {
        size++;
        if (EEPROM.read(i) == 0) {
            if (zero) break;
            zero = true;
        } else {
            zero = false;
        }
    }
    return size-2;
}

void settingsSetup() {

    EEPROM.begin(SPI_FLASH_SEC_SIZE);

    Embedis::dictionary( F("EEPROM"),
        SPI_FLASH_SEC_SIZE,
        [](size_t pos) -> char { return EEPROM.read(pos); },
        [](size_t pos, char value) { EEPROM.write(pos, value); },
        #if AUTO_SAVE
            []() { EEPROM.commit(); }
        #else
            []() {}
        #endif
    );

    Embedis::hardware( F("WIFI"), [](Embedis* e) {
        StreamString s;
        WiFi.printDiag(s);
        e->response(s);
    }, 0);

    Embedis::command( F("RECONNECT"), [](Embedis* e) {
        wifiConfigure();
        wifiDisconnect();
        e->response(Embedis::OK);
    });

    Embedis::command( F("RESET"), [](Embedis* e) {
        e->response(Embedis::OK);
        ESP.reset();
    });

    Embedis::command( F("SAVE"), [](Embedis* e) {
        e->response(Embedis::OK);
        saveSettings();
    });

    Embedis::command( F("BOARD"), [](Embedis* e) {
        if (e->argc > 1) {
            int board = String(e->argv[1]).toInt();
            hwLoad(board);
        }
        e->response(getBoardFullName().c_str());
    });

    Embedis::command( F("BOARDS"), [](Embedis* e) {
        for (unsigned int i=1; i<BOARD_LAST; i++) {
            e->stream->printf("%2d: %s\n", i, getBoardFullName(i).c_str());
        }
        e->response(Embedis::OK);
    });

    Embedis::command( F("STATUS"), [](Embedis* e) {
        e->stream->printf("Free heap: %d bytes\n", ESP.getFreeHeap());
        e->response(Embedis::OK);
    });

    Embedis::command( F("EEPROM.DUMP"), [](Embedis* e) {
        for (unsigned int i = 0; i < SPI_FLASH_SEC_SIZE; i++) {
            if (i % 16 == 0) e->stream->printf("\n[%04X] ", i);
            e->stream->printf("%02X ", EEPROM.read(i));
        }
        e->stream->printf("\n");
        e->response(Embedis::OK);
    });

    Embedis::command( F("EEPROM.ERASE"), [](Embedis* e) {
        for (unsigned int i = 0; i < SPI_FLASH_SEC_SIZE; i++) {
            EEPROM.write(i, 0xFF);
        }
        EEPROM.commit();
        e->response(Embedis::OK);
    });

    Embedis::command( F("SETTINGS.SIZE"), [](Embedis* e) {
        e->response(String(settingsSize()));
    });

    DEBUG_MSG("[SETTINGS] EEPROM size: %d bytes\n", SPI_FLASH_SEC_SIZE);
    DEBUG_MSG("[SETTINGS] Settings size: %d bytes\n", settingsSize());

}

void settingsLoop() {
    embedis.process();
}

void moveSetting(const char * from, const char * to) {
    String value = getSetting(from);
    if (value.length() > 0) setSetting(to, value);
    delSetting(from);
}

template<typename T> String getSetting(const String& key, T defaultValue) {
    String value;
    if (!Embedis::get(key, value)) value = String(defaultValue);
    return value;
}

template<typename T> String getSetting(const String& key, unsigned int index, T defaultValue) {
    return getSetting(key + String(index), defaultValue);
}

String getSetting(const String& key) {
    return getSetting(key, "");
}

template<typename T> bool setSetting(const String& key, T value) {
    return Embedis::set(key, String(value));
}

template<typename T> bool setSetting(const String& key, unsigned int index, T value) {
    return setSetting(key + String(index), value);
}

bool delSetting(const String& key) {
    return Embedis::del(key);
}

bool delSetting(const String& key, unsigned int index) {
    return delSetting(key + String(index));
}

bool hasSetting(const String& key) {
    return getSetting(key).length() != 0;
}

bool hasSetting(const String& key, unsigned int index) {
    return getSetting(key, index, "").length() != 0;
}

void saveSettings() {
    DEBUG_MSG("[SETTINGS] Saving\n");
    #if not AUTO_SAVE
        EEPROM.commit();
    #endif
}
