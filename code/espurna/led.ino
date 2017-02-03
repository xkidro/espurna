/*

LED MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

// -----------------------------------------------------------------------------
// LED
// -----------------------------------------------------------------------------

#ifdef LED1_PIN

typedef struct {
    unsigned char pin;
    bool reverse;
} led_t;

std::vector<led_t> _leds;
bool ledAuto;

bool ledStatus(unsigned char id) {
    if (id >= _leds.size()) return false;
    bool status = digitalRead(_leds[id].pin);
    return _leds[id].reverse ? !status : status;
}

bool ledStatus(unsigned char id, bool status) {
    if (id >= _leds.size()) return false;
    bool s = _leds[id].reverse ? !status : status;
    digitalWrite(_leds[id].pin, _leds[id].reverse ? !status : status);
    return status;
}

bool ledToggle(unsigned char id) {
    if (id >= _leds.size()) return false;
    return ledStatus(id, !ledStatus(id));
}

void ledBlink(unsigned char id, unsigned long delayOff, unsigned long delayOn) {
    if (id >= _leds.size()) return;
    static unsigned long next = millis();
    if (next < millis()) {
        next += (ledToggle(id) ? delayOn : delayOff);
    }
}

void showStatus() {
    if (wifiConnected()) {
        if (WiFi.getMode() == WIFI_AP) {
            ledBlink(0, 2500, 2500);
        } else {
            ledBlink(0, 4900, 100);
        }
    } else {
        ledBlink(0, 500, 500);
    }
}

void ledMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    static bool isFirstMessage = true;

    String mqttSetter = getSetting("mqttSetter", MQTT_USE_SETTER);

    if (type == MQTT_CONNECT_EVENT) {
        char buffer[strlen(MQTT_LED_TOPIC) + mqttSetter.length() + 3];
        sprintf(buffer, "%s/+%s", MQTT_LED_TOPIC, mqttSetter.c_str());
        mqttSubscribe(buffer);
    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        String t = String(topic + mqttTopicRootLength());
        if (!t.startsWith(MQTT_LED_TOPIC)) return;
        if (!t.endsWith(mqttSetter)) return;

        // Get led ID
        unsigned int ledID = topic[strlen(topic) - mqttSetter.length() - 1] - '0';
        if (ledID >= ledCount()) {
            DEBUG_MSG("[LED] Wrong ledID (%d)\n", ledID);
            return;
        }

        // get value
        unsigned int value =  (char)payload[0] - '0';
        bool bitAuto = (value & 0x02) > 0;
        bool bitState = (value & 0x01) > 0;

        // Check ledAuto
        if (ledID == 0) {
            ledAuto = bitAuto ? bitState : false;
            setSetting("ledAuto", String() + (ledAuto ? "1" : "0"));
            if (bitAuto) return;
        }

        // Action to perform
        ledStatus(ledID, bitState);

    }

}

unsigned char ledCount() {
    return _leds.size();
}

void ledConfigure() {
    ledAuto = getSetting("ledAuto", String() + LED_AUTO).toInt() == 1;
}

void ledSetup() {

    #ifdef LED1_PIN
    {
        unsigned char pin = getSetting("ledGPIO0", LED1_PIN).toInt();
        bool inverse = getSetting("ledLogic0", LED1_PIN_INVERSE).toInt() == 1;
        _leds.push_back((led_t) { pin, inverse });
    }
    #endif
    #ifdef LED2_PIN
    {
        unsigned char pin = getSetting("ledGPIO1", LED2_PIN).toInt();
        bool inverse = getSetting("ledLogic1", LED2_PIN_INVERSE).toInt() == 1;
        _leds.push_back((led_t) { pin, inverse });
    }
    #endif
    #ifdef LED3_PIN
    {
        unsigned char pin = getSetting("ledGPIO2", LED3_PIN).toInt();
        bool inverse = getSetting("ledLogic2", LED3_PIN_INVERSE).toInt() == 1;
        _leds.push_back((led_t) { pin, inverse });
    }
    #endif
    #ifdef LED4_PIN
    {
        unsigned char pin = getSetting("ledGPIO3", LED4_PIN).toInt();
        bool inverse = getSetting("ledLogic3", LED4_PIN_INVERSE).toInt() == 1;
        _leds.push_back((led_t) { pin, inverse });
    }
    #endif

    for (unsigned int i=0; i < _leds.size(); i++) {
        pinMode(_leds[i].pin, OUTPUT);
        ledStatus(i, false);
    }

    ledConfigure();

    mqttRegister(ledMQTTCallback);

    DEBUG_MSG("[LED] Number of leds: %d\n", _leds.size());
    DEBUG_MSG("[LED] Led auto indicator is %s\n", ledAuto ? "ON" : "OFF" );

}

void ledLoop() {
    if (ledAuto) showStatus();
}

#else

void ledSetup() {};
void ledLoop() {};

#endif
