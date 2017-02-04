/*

LED MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

// -----------------------------------------------------------------------------
// LED
// -----------------------------------------------------------------------------

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

    unsigned char index = 1;
    while (index < MAX_HW_DEVICES) {
        unsigned char pin = getSetting("ledGPIO", index, GPIO_INVALID).toInt();
        if (pin == GPIO_INVALID) break;
        bool inverse = getSetting("ledLogic", index, 0).toInt() == 1;
        _leds.push_back((led_t) { pin, inverse });
        pinMode(pin, OUTPUT);
        ledStatus(index-1, false);
        ++index;
    }

    ledConfigure();

    mqttRegister(ledMQTTCallback);

    DEBUG_MSG("[LED] Number of leds: %d\n", _leds.size());
    DEBUG_MSG("[LED] Led auto indicator is %s\n", ledAuto ? "ON" : "OFF" );

}

void ledLoop() {
    if (ledAuto) showStatus();
}
