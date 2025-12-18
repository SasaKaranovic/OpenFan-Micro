#include <ESPmDNS.h>
#include <EEPROM.h>
#include <WiFiManager.h>
#include <CRC.h>
#include "ESPAsyncWebServer.h"
#include "WiFi.h"
#include "version.h"
#include "config_openfan.h"

WiFiManager wifiManager;
eeprom_data_t eepromData;
AsyncWebServer server(80);

volatile int WiFi_status = WL_IDLE_STATUS;
String deviceHostname = "sk-openfan-micro";
String mdnsName = "MyOpenFan";
char MACString[MAX_LEN_MAC_STRING] = {0};
uint32_t nLed_Tick = 0;
uint32_t nRPM_Tick = 0;
uint32_t nWiFi_Tick = 0;
volatile uint32_t fan_int_cnt = 0;
uint32_t fan_millis_start = 0;
uint32_t fan_millis_end = 0;
uint32_t fan_rpm = 0;
uint32_t nFanSave_Tick = 0;
uint8_t u8FanPercent = 0;
bool bRPM_counting = false;
bool bStartUpBoost = true;
uint32_t nStartUpBoost_Timeout = 0;

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    delay(1000);
    Serial.println("OpenFAN - Micro");
    Serial.println("Version: " + String(VERSION_MAJOR) + "-" + String(VERSION_MINOR) + "-" + String(VERSION_PATCH) );

    eeprom_init();
    eeprom_load();

    set_device_mdns_name();
    WiFi.setHostname(mdnsName.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_TX_POWER_LEVEL);

    int txPower = WiFi.getTxPower();
    Serial.print("TX power: ");
    Serial.println(txPower);

    delay(1000);

    // Setup GPIO
    pinMode(PIN_LED_ACT, OUTPUT);
    digitalWrite(PIN_LED_ACT, HIGH);

    pinMode(PIN_LDO_EN, OUTPUT);
    digitalWrite(PIN_LDO_EN, LOW);

    pinMode(PIN_LDO_LED, OUTPUT);

    if (eepromData.enable_12v)
    {
        Serial.println("Enabling 12V DC-DC");
        fan_enable_12V(true);
    }
    else
    {
        Serial.println("Keeping 12V DC-DC disabled");
        fan_enable_12V(false);
    }

    fan_setup();

    bool res;
    res = wifiManager.autoConnect("SK-OpenFAN");

    wifiManager.setHostname(mdnsName.c_str());
    wifiManager.setConnectRetries(4);

    if(!res) {
        Serial.println("Failed to connect or hit timeout");
        ESP.restart();
    }
    else {
        Serial.println("Connected.");
    }

    Serial.print("WiFi IP: ");
    Serial.println(WiFi.localIP());

    get_mac_string(MACString, MAX_LEN_MAC_STRING);

    if (!MDNS.begin(mdnsName.c_str()))
    {
        Serial.println("Error starting mDNS");
    }
    else
    {
        Serial.println((String) "mDNS http://" + mdnsName + ".local");

        // Advertise services
        MDNS.addService("http", "tcp", 80);
    }

    Serial.print("Starting WebServer...");
    WebServer_Setup();
    Serial.println("done.");

    Serial.print("WiFi IP: ");
    Serial.println(WiFi.localIP());

    // Debug message to signal we are initialized and entering loop
    Serial.println("Ready to go.");
    digitalWrite(PIN_LED_ACT, LOW);

    // Enable fan PWM boost
    bStartUpBoost = true;
    nStartUpBoost_Timeout = millis() + FAN_STARTUP_BOOST_TIME;
    Serial.print("Turning ON fan starup boost for ");
    Serial.print(FAN_STARTUP_BOOST_TIME);
    Serial.println(" miliseconds");
    set_pwm(FAN_STARTUP_BOOST_PWM);
}


void loop()
{
    if (eepromData.enable_act_led)
    {
        if (nLed_Tick <= millis())
        {
            digitalWrite(PIN_LED_ACT, !digitalRead(PIN_LED_ACT));
            nLed_Tick = millis() + LED_BLINK_PERIOD;
        }
    }

    // Update RPM
    fan_tick();

    // Check WiFi status
    wifi_check();
}


void wifi_check(void)
{
    wl_status_t WiFiStatus;

    if (nWiFi_Tick <= millis())
    {
        WiFiStatus = WiFi.status();
        if ((WiFiStatus != WL_CONNECTED))
        {
            Serial.println("WiFi is disconnected... Reconnecting to WiFi...");
            if (WiFi.disconnect())
            {
                Serial.println("Disconnected. Attempting reconnect...");
            }
            else
            {
                Serial.println("Failed to disconnect within 1000 ms...");
            }

            WiFi.reconnect();
        }

        nWiFi_Tick = millis() + WIFI_CONN_CHECK_TICK;
    }
}

bool get_mac_bytes(char *mac, uint8_t nLen)
{
    if(mac == NULL || nLen < 6)
    {
        return false;
    }

    for(int i=0; i<6; i++)
    {
        mac[i] = ((ESP.getEfuseMac() >> (i*8)) & 0xff);
    }
    return true;
}

bool get_mac_string(char *string, uint8_t nLen)
{
    if(string == NULL)
    {
        return false;
    }

    char uChipId[MAX_LEN_MAC_BYTES] = {0};
    char strDeviceMAC[16] = {0};

    if(!get_mac_bytes(&uChipId[0], MAX_LEN_MAC_BYTES))
    {
        return false;
    }

    snprintf(string, nLen, "%02X%02X%02X%02X%02X%02X",
            uChipId[0], uChipId[1], uChipId[2], uChipId[3], uChipId[4], uChipId[5]);

    return true;
}


void set_device_mdns_name(void)
{
    char uChipId[MAX_LEN_MAC_BYTES] = {0};
    char strName[128] = {0};

    // Check if EEPROM name is valid
    if (
        (strnlen(eepromData.deviceName, MAX_CUSTOM_NAME)<1) ||
        (!is_valid_name((const uint8_t *)&eepromData.deviceName, MAX_CUSTOM_NAME))
       )
    {
        Serial.print("EEPROM Name Length: ");
        Serial.println(strlen(eepromData.deviceName));

        Serial.print("Invalid EEPROM name: ");
        Serial.println(String(eepromData.deviceName));
        Serial.println("Resetting EEPROM name.");
        eeprom_initialize_name();
    }

    get_mac_bytes(uChipId, MAX_LEN_MAC_BYTES);

    Serial.print("EEPROM name: ");
    Serial.println(String(eepromData.deviceName));

    snprintf(strName, 128, "uOpenFan-%s", eepromData.deviceName);
    mdnsName = String(strName);
}

bool is_valid_name(const uint8_t *pData, uint8_t nLen)
{
    for(uint8_t i=0; i<nLen; i++)
    {
        if ( ((pData[i]<'0') && (pData[i]>'9')) &&
             ((pData[i]<'A') && (pData[i]>'Z')) &&
             ((pData[i]<'a') && (pData[i]>'z')) )
        {
            return false;
        }
    }

    return true;
}
