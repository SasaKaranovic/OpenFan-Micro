#ifndef __HOST_COMMUNICATION_INC_H__
#define __HOST_COMMUNICATION_INC_H__

#define COMM_START_CHARACTER        '>'
#define COMM_END_CHARACTER          '\n'
#define COMM_ALT_END_CHARACTER      '\r'
#define COMM_MIN_MESSAGE_LEN        3
#define COMM_RESPONSE_CHARACTER     '<'
#define COMM_RX_ASCII_BUFFER_LEN    128
#define COMM_RX_HEX_BUFFER_LEN      COMM_RX_ASCII_BUFFER_LEN/2 - 1
#define COMM_TX_BUFFER_LEN          128

#define COMM_PROTOCOL_VERSION       2

typedef enum comm_cmd
{
    CMD_FIRST,
    CMD_FAN_ALL_GET_RPM     = 0x00,
    CMD_FAN_GET_RPM         = 0x01,
    CMD_FAN_SET_PWM         = 0x02,
    CMD_FAN_SET_ALL_PWM     = 0x03,
    CMD_FAN_SET_RPM         = 0x04,
    CMD_HW_INFO             = 0x05,
    CMD_FW_INFO             = 0x06,

    CMD_LAST                = CMD_FW_INFO,
} comm_cmd_t;

#endif /* __HOST_COMMUNICATION_INC_H__ */
