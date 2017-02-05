/*

BUTTON MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

// -----------------------------------------------------------------------------
// BUTTON
// -----------------------------------------------------------------------------

#include <DebounceEvent.h>
#include <vector>

typedef struct {
    DebounceEvent * button;
    unsigned int actions;
    unsigned int relayID;
} button_t;

std::vector<button_t> _buttons;

#ifdef MQTT_BUTTON_TOPIC
void buttonMQTT(unsigned char id, uint8_t event) {
    if (id >= _buttons.size()) return;
    String mqttGetter = getSetting("mqttGetter", MQTT_USE_GETTER);
    char buffer[strlen(MQTT_BUTTON_TOPIC) + mqttGetter.length() + 3];
    sprintf(buffer, "%s/%d%s", MQTT_BUTTON_TOPIC, id, mqttGetter.c_str());
    char payload[2];
    sprintf(payload, "%d", event);
    mqttSend(buffer, payload);
}
#endif

unsigned char buttonAction(unsigned char id, unsigned char event) {
    if (id >= _buttons.size()) return BUTTON_MODE_NONE;
    unsigned int actions = _buttons[id].actions;
    if (event == BUTTON_EVENT_PRESSED) return (actions >> 12) & 0x0F;
    if (event == BUTTON_EVENT_CLICK) return (actions >> 8) & 0x0F;
    if (event == BUTTON_EVENT_DBLCLICK) return (actions >> 4) & 0x0F;
    if (event == BUTTON_EVENT_LNGCLICK) return (actions) & 0x0F;
    return BUTTON_MODE_NONE;
}

unsigned int buttonActions(unsigned char id) {

    unsigned char pressAction = getSetting("btnPress", id, BUTTON_MODE_NONE).toInt();
    unsigned char clickAction = getSetting("btnClick", id, BUTTON_MODE_TOGGLE).toInt();
    unsigned char dblClickAction = getSetting("btnDblClick", id, (id == 1) ? BUTTON_MODE_AP : BUTTON_MODE_NONE).toInt();
    unsigned char lngClickAction = getSetting("btnLngClick", id, (id == 1) ? BUTTON_MODE_RESET : BUTTON_MODE_NONE).toInt();

    unsigned int value;
    value  = pressAction << 12;
    value += clickAction << 8;
    value += dblClickAction << 4;
    value += lngClickAction;
    return value;
}


void buttonSetup() {

    // Do not configure buttons for Sonoff Dual
    if (getBoard() == BOARD_ITEAD_SONOFF_DUAL) return;

    unsigned char index = 1;
    while (index < MAX_HW_DEVICES) {
        unsigned char pin = getSetting("btnGPIO", index, GPIO_INVALID).toInt();
        if (pin == GPIO_INVALID) break;
        unsigned char relayId = getSetting("btnRelay", index, 0).toInt();
        unsigned int actions = buttonActions(index);
        _buttons.push_back({new DebounceEvent(pin), actions, relayId});
        ++index;
    }

    unsigned char ledPulse = getSetting("ledPulseGPIO", GPIO_INVALID).toInt();
    if (ledPulse != GPIO_INVALID) {
        pinMode(ledPulse, OUTPUT);
        byte relayPulseMode = getSetting("relayPulseMode", String(RELAY_PULSE_MODE)).toInt();
        digitalWrite(ledPulse, relayPulseMode != RELAY_PULSE_NONE);
    }

    DEBUG_MSG("[BUTTON] Number of buttons: %d\n", _buttons.size());

}

uint8_t mapEvent(uint8_t event) {
    if (event == EVENT_PRESSED) return BUTTON_EVENT_PRESSED;
    if (event == EVENT_CHANGED) return BUTTON_EVENT_CLICK;
    if (event == EVENT_SINGLE_CLICK) return BUTTON_EVENT_CLICK;
    if (event == EVENT_DOUBLE_CLICK) return BUTTON_EVENT_DBLCLICK;
    if (event == EVENT_LONG_CLICK) return BUTTON_EVENT_LNGCLICK;
    return BUTTON_EVENT_NONE;
}

void buttonLoop() {

    if (getBoard() == BOARD_ITEAD_SONOFF_DUAL) {

        if (Serial.available() >= 4) {

            unsigned char value;
            if (Serial.read() == 0xA0) {
                if (Serial.read() == 0x04) {
                    value = Serial.read();
                    if (Serial.read() == 0xA1) {

                        // RELAYs and BUTTONs are synchonized in the SIL F330
                        // The on-board BUTTON2 should toggle RELAY0 value
                        // Since we are not passing back RELAY2 value
                        // (in the relayStatus method) it will only be present
                        // here if it has actually been pressed
                        if ((value & 4) == 4) {
                            value = value ^ 1;
                            #ifdef MQTT_BUTTON_TOPIC
                                buttonMQTT(0, BUTTON_EVENT_CLICK);
                            #endif
                        }

                        // Otherwise check if any of the other two BUTTONs
                        // (in the header) has been pressent, but we should
                        // ensure that we only toggle one of them to avoid
                        // the synchronization going mad
                        // This loop is generic for any PSB-04 module
                        for (unsigned int i=0; i<relayCount(); i++) {

                            bool status = (value & (1 << i)) > 0;

                            // relayStatus returns true if the status has changed
                            if (relayStatus(i, status)) break;

                        }

                    }
                }
            }
        }

    } else {

        for (unsigned int id=0; id < _buttons.size(); id++) {
            if (_buttons[id].button->loop()) {

                uint8_t event = mapEvent(_buttons[id].button->getEvent());
                DEBUG_MSG("[BUTTON] Pressed #%d, event: %d\n", id, event);
                if (event == 0) continue;

                #ifdef MQTT_BUTTON_TOPIC
                    buttonMQTT(id, event);
                #endif

                unsigned char action = buttonAction(id, event);

                if (action == BUTTON_MODE_TOGGLE) {
                    if (_buttons[id].relayID > 0) {
                        relayToggle(_buttons[id].relayID - 1);
                    }
                }
                if (action == BUTTON_MODE_AP) createAP();
                if (action == BUTTON_MODE_RESET) ESP.reset();
                if (action == BUTTON_MODE_PULSE) relayPulseToggle();

            }
        }

    }

}
