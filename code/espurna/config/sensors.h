//--------------------------------------------------------------------------------
// Custom RF module
// Check http://tinkerman.cat/adding-rf-to-a-non-rf-itead-sonoff/
// Enable support by passing ENABLE_RF=1 build flag
//--------------------------------------------------------------------------------

#define RF_PIN                  14
#define RF_CHANNEL              31
#define RF_DEVICE               1

//--------------------------------------------------------------------------------
// DHTXX temperature/humidity sensor
// Enable support by passing ENABLE_DHT=1 build flag
//--------------------------------------------------------------------------------

#define DHT_PIN                 14
#define DHT_UPDATE_INTERVAL     60000
#define DHT_TYPE                DHT22
#define DHT_TIMING              11
#define DHT_TEMPERATURE_TOPIC   "/temperature"
#define DHT_HUMIDITY_TOPIC      "/humidity"

#define HUMIDITY_NORMAL         0
#define HUMIDITY_COMFORTABLE    1
#define HUMIDITY_DRY            2
#define HUMIDITY_WET            3

//--------------------------------------------------------------------------------
// DS18B20 temperature sensor
// Enable support by passing ENABLE_DS18B20=1 build flag
//--------------------------------------------------------------------------------

#define DS_PIN                  14
#define DS_UPDATE_INTERVAL      60000
#define DS_TEMPERATURE_TOPIC    "/temperature"

//--------------------------------------------------------------------------------
// Custom current sensor
// Check http://tinkerman.cat/your-laundry-is-done/
// Check http://tinkerman.cat/power-monitoring-sonoff-th-adc121/
// Enable support by passing ENABLE_EMON=1 build flag
//--------------------------------------------------------------------------------

#define EMON_ANALOG_PROVIDER    0
#define EMON_ADC121_PROVIDER    1

#define EMON_DEFAULT_PROVIDER   EMON_ANALOG_PROVIDER

// EMON_ANALOG_PROVIDER
#define EMON_INT_ADDRESS        0
#define EMON_INT_ADC_BITS       10
#define EMON_INT_REF_VOLTAGE    1.0
#define EMON_INT_CURR_PRECISION 1
#define EMON_INT_CURR_OFFSET    0.25

// EMON_ADC121_PROVIDER
#define EMON_ADC121_ADDRESS         0x50
#define EMON_ADC121_ADC_BITS        12
#define EMON_ADC121_REF_VOLTAGE     3.3
#define EMON_ADC121_CURR_PRECISION  2
#define EMON_ADC121_CURR_OFFSET     0.10

#define EMON_CURRENT_RATIO      30
#define EMON_SAMPLES            1000
#define EMON_INTERVAL           10000
#define EMON_MEASUREMENTS       6
#define EMON_MAINS_VOLTAGE      230
#define EMON_CURRENT_RATIO      30
#define EMON_APOWER_TOPIC       "/apower"
#define EMON_ENERGY_TOPIC       "/energy"
#define EMON_CURRENT_TOPIC      "/current"

//--------------------------------------------------------------------------------
// HLW8012 power sensor (Sonoff POW)
// Enable support by passing ENABLE_HLW8012=1 build flag
//--------------------------------------------------------------------------------

#define HLW8012_USE_INTERRUPTS      1
#define HLW8012_SEL_PIN             5
#define HLW8012_CF1_PIN             13
#define HLW8012_CF_PIN              14
#define HLW8012_SEL_CURRENT         HIGH
#define HLW8012_CURRENT_R           0.001
#define HLW8012_VOLTAGE_R_UP        ( 5 * 470000 ) // Real: 2280k
#define HLW8012_VOLTAGE_R_DOWN      ( 1000 ) // Real 1.009k
#define HLW8012_POWER_TOPIC         "/power"
#define HLW8012_CURRENT_TOPIC       "/current"
#define HLW8012_VOLTAGE_TOPIC       "/voltage"
#define HLW8012_APOWER_TOPIC        "/apower"
#define HLW8012_RPOWER_TOPIC        "/rpower"
#define HLW8012_PFACTOR_TOPIC       "/pfactor"
#define HLW8012_ENERGY_TOPIC        "/energy"
#define HLW8012_UPDATE_INTERVAL     5000
#define HLW8012_REPORT_EVERY        12
