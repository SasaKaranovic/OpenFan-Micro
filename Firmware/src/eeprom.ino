void eeprom_init(void)
{
   EEPROM.begin(sizeof(eeprom_data_t));
}

bool eeprom_load(void)
{
    bool ret;
    // Read EEPROM data
    EEPROM.readBytes(0, (void *)&eepromData, sizeof(eeprom_data_t));

    // Calculate CRC
    uint8_t calc_crc = calcCRC8((const uint8_t*)&eepromData, (sizeof(eeprom_data_t)-1) );

    if (eepromData.valid == calc_crc)
    {
        Serial.println("EEPROM CRC is valid");
        ret = true;
    }
    else
    {
        eeprom_reset();
        ret = false;
    }
    Serial.println("Calibration in EEPROM is invalid. Resetting!");
    set_pwm(eepromData.last_percent);

    return ret;
}

void eeprom_reset(void)
{
    eeprom_initialize_name();
    eepromData.enable_12v = false;
    eepromData.enable_act_led = true;
    eepromData.last_percent = 100;
    eepromData.impulse_per_rev = FAN_INT_PER_REV;

    eeprom_save();
}

void eeprom_initialize_name(void)
{
    memset(eepromData.deviceName, 0, NAME_MAX_LEN);
    strncpy(eepromData.deviceName, DEFAULT_HOSTNAME, DEFAULT_HOSTNAME_LEN);
}

void eeprom_save(void)
{
    uint8_t calc_crc = calcCRC8((const uint8_t*)&eepromData, (sizeof(eeprom_data_t)-1) );
    eepromData.valid = calc_crc;

    EEPROM.writeBytes(0, (void *)&eepromData, sizeof(eeprom_data_t));
    EEPROM.commit();
}
