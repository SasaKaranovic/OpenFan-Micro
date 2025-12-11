char tmpBuffer[200] = {0};

// Helper function that allows us to replace template variable in .html file
// with a value from our code.
String template_const_processor(const String& var)
{
    if (var == "FW_VERSION") {
        return String(String(VERSION_MAJOR) + "." + String(VERSION_MINOR) + "." + String(VERSION_PATCH));
    }
    else if (var == "BUILD_DATE")
    {
        return String(__DATE__);
    }
    else if (var == "BUILD_TIME")
    {
        return String(__TIME__);

    }
    else if (var == "DEVICE_NAME")
    {
        return String(mdnsName);
    }
    else if (var == "CURRENT_PWM")
    {
        return String(u8FanPercent);
    }
    else if (var == "DEVICE_MAC")
    {
        return String(MACString);
    }
    else if (var == "FAN_VOLTAGE")
    {
        if (eepromData.enable_12v)
        {
            return String("12");
        }
        return String("5");
    }
    else if (var == "ACT_LED")
    {
        if (eepromData.enable_act_led)
        {
            return String("blinking");
        }
        return String("off");
    }
    else if (var == "CURRENT_RPM")
    {
        return String(fan_rpm);
    }

    Serial.print("Unknown template variable: ");
    Serial.println(var);
    return String("__MISSING_VAR__");
}

void WebServer_Setup(void)
{
    server_init_handlers();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->redirect("/index.html");
    });

    server.on("/api/v0/openfan/status", HTTP_GET, [] (AsyncWebServerRequest *request) {
        snprintf(tmpBuffer, 200, "{\"status\":\"ok\", \"data\": { \"act_led_enabled\": \"%s\", \"fan_is_12v\": \"%s\" } }",
                 (eepromData.enable_act_led ? "true" : "false"),
                 (eepromData.enable_12v ? "true" : "false")
                 );
        request->send(200, "application/json", tmpBuffer);
    });

    server.on("/api/v0/openfan/name/set", HTTP_GET, [] (AsyncWebServerRequest *request) {
        if (request->hasParam("name"))
        {
            String devName = request->getParam("name")->value();
            uint8_t len = strlen(devName.c_str());
            len = (len < MAX_CUSTOM_NAME) ? len : MAX_CUSTOM_NAME;

            if (len < MIN_CUSTOM_NAME)
            {
                request->send(400, "application/json", "{\"status\":\"fail\", \"message\": \"Name is too short!\" }");
            }
            else if (len > MAX_CUSTOM_NAME)
            {
                request->send(400, "application/json", "{\"status\":\"fail\", \"message\": \"Name is too long!\" }");
            }
            else
            {
                memset(eepromData.deviceName, NAME_MAX_LEN, 0);
                remove_illegal_chars_and_copy(eepromData.deviceName, devName.c_str(), len);
                eeprom_save();

                Serial.print("Saved length: ");
                Serial.println(strlen(eepromData.deviceName));

                request->send(200, "application/json", "{\"status\":\"ok\", \"message\": \"Device renamed. Restarting to apply...\" }");

                delay(1000);
                ESP.restart();
            }
        }
        else
        {
            request->send(400, "application/json", "{\"status\":\"fail\", \"message\": \"Missing argument!\" }");
        }
    });

    server.on("/api/v0/led/enable", HTTP_GET, [] (AsyncWebServerRequest *request) {
        eepromData.enable_act_led = true;
        eeprom_save();
        request->send(200, "application/json", "{\"status\":\"ok\", \"message\": \"Activity LED enabled\" }");
    });

    server.on("/api/v0/led/disable", HTTP_GET, [] (AsyncWebServerRequest *request) {
        eepromData.enable_act_led = false;
        digitalWrite(PIN_LED_ACT, LOW);
        eeprom_save();
        request->send(200, "application/json", "{\"status\":\"ok\", \"message\": \"Activity LED disabled\" }");
    });


    server.on("/api/v0/fan/status", HTTP_GET, [] (AsyncWebServerRequest *request) {
        int len;
        len = snprintf(tmpBuffer, 200, "{\"status\":\"ok\",\"data\":{\"rpm\":%d,\"pwm_percent\":%d}}", fan_rpm, u8FanPercent);
        request->send(200, "application/json", tmpBuffer);
    });

    server.on("/api/v0/fan/0/set", HTTP_GET, [] (AsyncWebServerRequest *request) {
        // Check if PWM value is set
        if (request->hasParam("value"))
        {
            int len;
            uint8_t pwm_value;
            pwm_value = request->getParam("value")->value().toInt();
            set_pwm(pwm_value);
            len = snprintf(tmpBuffer, 200, "{\"status\":\"ok\", \"message\": \"Setting PWM to %d\" }", pwm_value);
            request->send(200, "application/json", tmpBuffer);
        }
        else
        {
            request->send(400, "application/json", "{\"status\":\"fail\", \"message\": \"Missing argument!\" }");
        }

    });

    server.on("/api/v0/fan/voltage/high", HTTP_GET, [] (AsyncWebServerRequest *request) {
        if ( request->hasParam("confirm") )
        {
            fan_configure_as_12V();
            request->send(200, "application/json", "{\"status\":\"ok\", \"message\": \"Switching fan output to 12V\" }");
        }
        else
        {
            request->send(400, "application/json", "{\"status\":\"fail\", \"message\": \"Missing argument `confirm`!\"}");
        }
    });

    server.on("/api/v0/fan/voltage/low", HTTP_GET, [] (AsyncWebServerRequest *request) {
        if ( request->hasParam("confirm") )
        {
            fan_configure_as_5V();
            request->send(200, "application/json", "{\"status\":\"ok\", \"message\": \"Switching fan output to 5V\" }");
        }
        else
        {
            request->send(400, "application/json", "{\"status\":\"fail\", \"message\": \"Missing argument `confirm`!\"}");
        }
    });

    server.on("/api/v0/wifi/info", HTTP_GET, [] (AsyncWebServerRequest *request) {
        int len;
        len = snprintf(tmpBuffer, 200, "{\"status\":\"ok\",\"data\":{\"SSID\":\"%s\",\"RSSI\":\"%4d\", \"CH\":\"%2d\"}}", WiFi.SSID().c_str(), WiFi.RSSI(), WiFi.channel());
        request->send(200, "application/json", tmpBuffer);
    });

    server.on("/api/v0/wifi/reset", HTTP_GET, [] (AsyncWebServerRequest *request) {
        if ( request->hasParam("confirm") )
        {
            Serial.println("WiFi reset request received.");
            request->send(200, "application/json", "{\"status\":\"ok\", \"message\": \"WiFi configuration has been reset. Please connect to OpenFAN Micro WiFi and re-configure the device.\"}");
            wifiManager.resetSettings();
        }
        else
        {
            request->send(400, "application/json", "{\"status\":\"fail\", \"message\": \"Missing argument `confirm`!\"}");
        }
    });

    server.on("/api/v0/device/reboot", HTTP_GET, [] (AsyncWebServerRequest *request) {
        if ( request->hasParam("confirm") )
        {
            Serial.println("Device reboot request received. Rebooting...");
            ESP.restart();
        }
        else
        {
            request->send(400, "application/json", "{\"status\":\"fail\", \"message\": \"Missing argument `confirm`!\"}");
        }
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        Serial.print("404:");
        Serial.println(request->url());
        request->send(404);
    });

    server.begin();
}


void remove_illegal_chars_and_copy(char *pDest, const char *pSource, uint8_t nLen)
{
    if(pSource == NULL || pDest == NULL)
    {
        Serial.println("Null pointer given. Aborting!");
        return;
    }
    // Remove all non-ascii characters
    // by looping through string, if valid character is found copy
    // if illegal character is found, skip
    uint8_t destPos = 0;

    for(uint8_t i=0; i<nLen; i++)
    {
        if (
            ((pSource[i]>='0') && (pSource[i]<='9')) ||
            ((pSource[i]>='A') && (pSource[i]<='Z')) ||
            ((pSource[i]>='a') && (pSource[i]<='z'))
        )
        {
            pDest[destPos++] = pSource[i];
        }
    }
    pDest[destPos] = 0;
}
