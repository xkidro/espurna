#include <pgmspace.h>

// New boards have to be added just before the BOARD_LAST line,
// add name and manufacturer and add configuration in the hardware.ino
typedef enum {

    BOARD_CUSTOM = 1,

    BOARD_NODEMCU_V3,
    BOARD_WEMOS_D1_RELAYSHIELD,
    BOARD_ITEAD_SONOFF,
    BOARD_ITEAD_SONOFF_TH,
    BOARD_ITEAD_SONOFF_SV,
    BOARD_ITEAD_SONOFF_TOUCH,
    BOARD_ITEAD_SONOFF_POW,
    BOARD_ITEAD_SONOFF_DUAL,
    BOARD_ITEAD_SONOFF_1CH,
    BOARD_ITEAD_SONOFF_4CH,
    BOARD_ITEAD_SLAMPHER,
    BOARD_ITEAD_S20,
    BOARD_ELECTRODRAGON_ESP_RELAY_BOARD,
    BOARD_WORKCHOICE_ECOPLUG,
    BOARD_JANGOE_WIFI_RELAY_NC,
    BOARD_JANGOE_WIFI_RELAY_NO,
    BOARD_OPENENERGYMONITOR_MQTT_RELAY,
    BOARD_JORGE_GARCIA_WIFI_RELAYS_BOARD_KIT,

    BOARD_LAST

} board_t;

PROGMEM const char man_unknown[] = "UNKNOWN";
PROGMEM const char man_custom[] = "CUSTOM";
PROGMEM const char man_nodemcu[] = "NODEMCU";
PROGMEM const char man_wemos[] = "WEMOS";
PROGMEM const char man_itead[] = "ITEAD STUDIO";
PROGMEM const char man_electrodragon[] = "ELECTRODRAGON";
PROGMEM const char man_workchoice[] = "WORKCHOICE";
PROGMEM const char man_jangoe[] = "JAN GOEDEKE";
PROGMEM const char man_openenergymonitor[] = "OPEN ENERGY MONITOR";
PROGMEM const char man_jorgegarcia[] = "JORGE GARCIA";

PROGMEM const char* const manufacturers[] = {
    man_unknown,
    man_custom, // BOARD_CUSTOM
    man_nodemcu, // BOARD_NODEMCU_V3
    man_wemos, // BOARD_WEMOS_D1_RELAYSHIELD
    man_itead, // BOARD_ITEAD_SONOFF
    man_itead, // BOARD_ITEAD_SONOFF_TH
    man_itead, // BOARD_ITEAD_SONOFF_SV
    man_itead, // BOARD_ITEAD_SONOFF_TOUCH
    man_itead, // BOARD_ITEAD_SONOFF_POW
    man_itead, // BOARD_ITEAD_SONOFF_DUAL
    man_itead, // BOARD_ITEAD_SONOFF_1CH
    man_itead, // BOARD_ITEAD_SONOFF_4CH
    man_itead, // BOARD_ITEAD_SLAMPHER
    man_itead, // BOARD_ITEAD_S20
    man_electrodragon, // BOARD_ELECTRODRAGON_ESP_RELAY_BOARD
    man_workchoice, // BOARD_WORKCHOICE_ECOPLUG
    man_jangoe, // BOARD_JANGOE_WIFI_RELAY_NC
    man_jangoe, // BOARD_JANGOE_WIFI_RELAY_NO
    man_openenergymonitor, // BOARD_OPENENERGYMONITOR_MQTT_RELAY
    man_jorgegarcia // BOARD_JORGE_GARCIA_WIFI_RELAYS_BOARD_KIT
};

const PROGMEM char board_unknown[] = "UNKNOWN";
const PROGMEM char board_custom[] = "CUSTOM";
const PROGMEM char board_nodemcu_v3[] = "LOLIN";
const PROGMEM char board_wemos_d1_relayshield[] = "D1 RELAY SHIELD";
const PROGMEM char board_itead_sonoff[] = "SONOFF";
const PROGMEM char board_itead_sonoff_th[] = "SONOFF TH";
const PROGMEM char board_itead_sonoff_sv[] = "SONOFF SV";
const PROGMEM char board_itead_sonoff_touch[] = "SONOFF TOUCH";
const PROGMEM char board_itead_sonoff_pow[] = "SONOFF POW";
const PROGMEM char board_itead_sonoff_dual[] = "SONOFF DUAL";
const PROGMEM char board_itead_sonoff_1ch[] = "SONOFF 1CH";
const PROGMEM char board_itead_sonoff_4ch[] = "SONOFF 4CH";
const PROGMEM char board_itead_slampher[] = "SLAMPHER";
const PROGMEM char board_itead_s20[] = "S20";
const PROGMEM char board_electrodragon_esp_relay_board[] = "ESP RELAY BOARD";
const PROGMEM char board_workchoice_ecoplug[] = "ECOPLUG";
const PROGMEM char board_jangoe_wifi_relay_nc[] = "WIFI RELAY NC";
const PROGMEM char board_jangoe_wifi_relay_no[] = "WIFI RELAY NO";
const PROGMEM char board_openenergymonitor_mqtt_relay[] = "MQTT RELAY";
const PROGMEM char board_jorge_garcia_wifi_relays_board_kit[] = "WIFI + RELAYS BOARD KIT";

const PROGMEM char * const boardnames[] = {
    board_unknown,
    board_custom, // BOARD_CUSTOM
    board_nodemcu_v3, // BOARD_NODEMCU_V3
    board_wemos_d1_relayshield, // BOARD_WEMOS_D1_RELAYSHIELD
    board_itead_sonoff, // BOARD_ITEAD_SONOFF
    board_itead_sonoff_th, // BOARD_ITEAD_SONOFF_TH
    board_itead_sonoff_sv, // BOARD_ITEAD_SONOFF_SV
    board_itead_sonoff_touch, // BOARD_ITEAD_SONOFF_TOUCH
    board_itead_sonoff_pow, // BOARD_ITEAD_SONOFF_POW
    board_itead_sonoff_dual, // BOARD_ITEAD_SONOFF_DUAL
    board_itead_sonoff_1ch, // BOARD_ITEAD_SONOFF_1CH
    board_itead_sonoff_4ch, // BOARD_ITEAD_SONOFF_4CH
    board_itead_slampher, // BOARD_ITEAD_SLAMPHER
    board_itead_s20, // BOARD_ITEAD_S20
    board_electrodragon_esp_relay_board, // BOARD_ELECTRODRAGON_ESP_RELAY_BOARD
    board_workchoice_ecoplug, // BOARD_WORKCHOICE_ECOPLUG
    board_jangoe_wifi_relay_nc, // BOARD_JANGOE_WIFI_RELAY_NC
    board_jangoe_wifi_relay_no, // BOARD_JANGOE_WIFI_RELAY_NO
    board_openenergymonitor_mqtt_relay, // BOARD_OPENENERGYMONITOR_MQTT_RELAY
    board_jorge_garcia_wifi_relays_board_kit // BOARD_JORGE_GARCIA_WIFI_RELAYS_BOARD_KIT
};

#ifndef DEFAULT_BOARD
#define DEFAULT_BOARD           BOARD_WEMOS_D1_RELAYSHIELD
#endif

#define GPIO_INVALID            99
#define MAX_HW_DEVICES          10
