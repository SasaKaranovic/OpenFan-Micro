#ifndef __CONFIG_OPEN_FAN_CFG_H_INC__
#define __CONFIG_OPEN_FAN_CFG_H_INC__

#define BOARD_REVISION          2
#define DEFAULT_HOSTNAME        "MyOpenFan\0"
#define DEFAULT_HOSTNAME_LEN    10
#define NAME_MAX_LEN            64
#define MAX_CUSTOM_NAME         12
#define MIN_CUSTOM_NAME         3
#define PIN_FAN_PWM             5
#define PIN_FAN_TACH            4
#define PWM_CH_FAN              0
#define PIN_LDO_EN              0
#define PIN_LDO_LED             3
#define PIN_LED_ACT             10
#define FAN_INT_PER_REV         2   // Number of TACH interrupts per single rotation
#define FAN_PWM_SAVE_PERIOD     10000
#define FAN_PWM_FREQ            25000
#define FAN_PWM_RESOLUTION      8
#define LED_BLINK_PERIOD        500
#define FAN_RPM_PAUSE_PERIOD    500
#define FAN_RPM_SAMPLE_PERIOD   500
#define FAN_STARTUP_BOOST_TIME  5000
#define FAN_STARTUP_BOOST_PWM   100
#define WIFI_TX_POWER_LEVEL     WIFI_POWER_8_5dBm
#define WIFI_CONN_CHECK_TICK    5000
#define MAX_LEN_MAC_BYTES       6
#define MAX_LEN_MAC_STRING      16
#define MAX_LEN_DEVICE_NAME     128

// Persistent data storage
typedef struct eeprom_data
{
    bool enable_12v;
    bool enable_act_led;
    uint8_t last_percent;
    uint8_t impulse_per_rev;
    char deviceName[NAME_MAX_LEN+1];
    uint8_t valid;
} eeprom_data_t;

#endif
