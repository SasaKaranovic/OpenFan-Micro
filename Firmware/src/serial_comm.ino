#if OPENFAN_USE_USB_COMM

void serial_comm_tick(void)
{
    serial_comm_receive();
    host_comm_tick();
}

static void serial_comm_receive(void)
{
    int available = 0;
    uint8_t ch;
    available = Serial.available();

    if (available>0)
    {
        for (uint8_t i=0; i<available; i++)
        {
            ch = (uint8_t)Serial.read();
            host_comm_receive_data(&ch, 1);
        }
    }
}

void host_comm_process_request(comm_cmd_t cmd, uint8_t *pData, uint32_t nDataLen)
{
    uint16_t rpm = 0;
    pTxBuffer[0] = COMM_RESPONSE_CHARACTER;
    nTxBufferLen = 1;
    response_add_byte(cmd);
    response_add_chr('|');

    switch (cmd)
    {
        case CMD_FAN_ALL_GET_RPM:
            // Add RPM for first fan
            response_add_byte(0);
            response_add_chr(':');
            response_add_u16((uint16_t)fan_rpm);
            response_add_chr(';');
            // Fake remaining data
            for(uint8_t i=1; i<10; i++)
            {
                rpm=0;
                response_add_byte(i);
                response_add_chr(':');
                response_add_u16(0);
                if(i<10)
                {
                    response_add_chr(';');
                }
            }
            break;

        case CMD_FAN_GET_RPM:
            rpm=0;
            if (pData[0])
            {
                rpm = (uint16_t)fan_rpm;
            }
            response_add_byte(pData[0]);
            response_add_chr(':');
            response_add_u16(rpm);
            break;

        case CMD_FAN_SET_PWM:
            if (pData[0] == 0)
            {
                set_pwm(pData[1]);
            }
            response_add_byte(pData[0]);
            response_add_chr(':');
            response_add_byte(pData[1]);
            break;

        case CMD_FAN_SET_ALL_PWM:
            set_pwm(pData[1]);
            response_add_byte(pData[0]);
            break;

        case CMD_FAN_SET_RPM:
            // FIXME: This is unsupported at the moment
            response_add_byte(pData[0]);
            response_add_chr(':');
            response_add_byte(pData[1]);
            response_add_byte(pData[2]);
            break;

        case CMD_HW_INFO:
            response_add_str("\r\n");
            response_add_str("HW_REV:01\r\n");
            response_add_str("MCU:OpenFAN_Micro_ESP32\r\n");
            response_add_str("USB:NATIVE\r\n");
            response_add_str("FAN_CHANNELS_TOTAL:1\r\n");
            response_add_str("FAN_CHANNELS_ARCH:1\r\n");
            response_add_str("FAN_CHANNELS_DRIVER:GPIO_PWM\r\n");
            break;

        case CMD_FW_INFO:
            response_add_str("FW_REV:OpenFAN_Micro_02\r\n");
            response_add_str("PROTOCOL_VERSION:02\r\n");
            break;

        default:
            Serial.println("Unsupported or invalid command");
    }

    host_comm_send_response();
}


static void host_comm_send_response(void)
{
    pTxBuffer[nTxBufferLen++] = '\r';
    pTxBuffer[nTxBufferLen++] = '\n';
    Serial.write(pTxBuffer, nTxBufferLen);

    memset(pTxBuffer, '\0', COMM_TX_BUFFER_LEN);
    nTxBufferLen = 0;
}

void host_comm_receive_data(uint8_t *pData, uint32_t nDataLen)
{
    if (pData == NULL)
    {
        Serial.println("Invalid pointer!");
        return;
    }

    // We already received a package and parse is pending
    if (bRxComplete)
    {
        return;
    }

    // Receive data
    for (uint32_t i=0; i<nDataLen; i++)
    {
        if (( (pData[i] == COMM_END_CHARACTER) || (pData[i] == COMM_ALT_END_CHARACTER)) && (!bRxComplete))
        {
            bRxComplete = true;
            return;
        }
        else
        {
            pRxBuffer[nRxBufferLen++] = pData[i];
        }

    }
}

void host_comm_parse_package(uint8_t *pPackageStart, uint32_t nLen)
{
    // Convert entire array from ASCII to raw values
    if (ascii_array_to_hex_array(pPackageStart, nLen, pPackage, &nPackageLen))
    {
        host_comm_process_request((comm_cmd_t)pPackage[0], &pPackage[1], nPackageLen-1);
    }
}



void host_comm_tick(void)
{
    bool bValidMsg = false;

    if (bRxComplete)
    {
        if(nRxBufferLen < COMM_MIN_MESSAGE_LEN)
        {
            Serial.print("Invalid message lenght d=");
            Serial.println(nRxBufferLen, DEC);
            host_comm_rearm();
            return;
        }

        // Search for start character
        for(uint32_t i=0; i<nRxBufferLen; i++)
        {
            if(pRxBuffer[i] == COMM_START_CHARACTER)
            {
                host_comm_parse_package(&pRxBuffer[i+1], nRxBufferLen-i-1);
                bValidMsg = true;
                break;
            }
        }

        if(!bValidMsg)
        {
            Serial.println("No start character found. Ignoring package.");
        }

        host_comm_rearm();
        return;
    }
}


static void host_comm_rearm(void)
{
    memset(pRxBuffer, '\0', COMM_RX_ASCII_BUFFER_LEN);
    memset(pPackage, '\0', COMM_RX_HEX_BUFFER_LEN);
    nRxBufferLen = 0;
    nPackageLen = 0;
    bRxComplete = false;
}

static uint8_t buffer_to_uint8(uint8_t *pBuffer)
{
    if(pBuffer==NULL)
    {
        Serial.println("Invalid pointer!");
        return 0;
    }

    uint8_t upper = ascii_to_hex(pBuffer[0]);
    uint8_t lower = ascii_to_hex(pBuffer[1]);

    return (upper<<4) | lower;
}

// Convert ASCII HEX character 0-9 or A-F to HEX value
static uint8_t ascii_to_hex(uint8_t ascii)
{

    if( (ascii>='0') && (ascii<='9') )
    {
        return ascii - '0';
    }
    else if ( (ascii >= 'A') && (ascii <= 'F') )
    {
        return ascii - 'A' + 10;
    }
    else if ( (ascii >= 'a') && (ascii <= 'f') )
    {
        return ascii - 'a' + 10;
    }
    else
    {
        Serial.println("Invalid character!");
        return 0;
    }
}

// Convert ASCII HEX array to HEX array
// This function will rewrite an ASCII HEX array (2 characters per byte) to hex array (1 byte per byte)
// The resulting array will be half the size and contain RAW HEX values
static bool ascii_array_to_hex_array(uint8_t *pASCII, uint32_t nLen, uint8_t *pTarget, uint32_t *nHexLen)
{
    if(pASCII==NULL)
    {
        Serial.println("Invalid pointer!");
        return false;
    }

    if(nLen<=0)
    {
        Serial.println("Invalid buffer length");
        return false;
    }

    uint8_t raw = 0;
    uint32_t raw_pos = 0;

    // Lenght is converted from USART ASCII package
    // so we need to multiply it by 2 to account for
    // ASCII values (2 byte)
    for (uint32_t i=0; i<nLen; i+=2)
    {
        // Convert ASCII bytes ie. FA to raw hex
        raw = (ascii_to_hex(pASCII[i]) << 4);
        raw |= ascii_to_hex(pASCII[i+1]);

        // Finally update raw byte
        pTarget[raw_pos++] = raw;
    }

    *nHexLen = raw_pos;
    return true;
}


static void response_add_data(uint8_t *pData, uint32_t nLen)
{
    if(pData==NULL)
    {
        Serial.println("Invalid pointer!");
        return;
    }

    for(uint32_t i=0; i<nLen; i++)
    {
        response_add_byte(pData[i]);
    }
}

static void response_add_byte(uint8_t data)
{
    sprintf((char *)&pTxBuffer[nTxBufferLen], "%02X", data);
    nTxBufferLen += 2;
}

static void response_add_u32(uint32_t data)
{
    sprintf((char *)&pTxBuffer[nTxBufferLen], "%08lX", data);
    nTxBufferLen += 8;
}

static void response_add_u16(uint16_t data)
{
    sprintf((char *)&pTxBuffer[nTxBufferLen], "%04X", data);
    nTxBufferLen += 4;
}

static void response_add_str(const char *pStr)
{
    uint32_t nLen = strlen(pStr);
    strncpy((char *)&pTxBuffer[nTxBufferLen], pStr, nLen);
    nTxBufferLen += nLen;
}

static void response_add_chr(char data)
{
    pTxBuffer[nTxBufferLen++] = data;
}



#endif
