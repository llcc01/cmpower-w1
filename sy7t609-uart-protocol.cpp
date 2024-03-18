#include <Arduino.h>

#include "sy7t609-uart-protocol.h"

#define SSI_HEADER (0xAA)
#define SSI_DEFAULT_FRAME_SIZE   (3)
#define SSI_MAX_PAYLOAD_SIZE     (7)
#define SSI_READ_PAYLOAD_SIZE    (4)
#define SSI_REPLY_PAYLOAD_SIZE   (3)
#define SSI_WRITE_PAYLOAD_SIZE   (7)

#define CMD_CLEAR_ADDRESS            (0xA0)
#define CMD_SELECT_REGISTER_ADDRESS  (0xA3)
#define CMD_READ_REGITSTER_3BYTES    (0xE3)
#define CMD_WRITE_RETISTER_3BYTES    (0xD3)

enum sy7t609_reply_code {
    REPLY_ACK_WITH_DATA           = 0xAA,
    REPLY_AUTO_REPORTING_HEADER   = 0xAE,
    REPLY_ACK_WITHOUT_DATA        = 0xAD,
    REPLY_NEGATIVE_ACK            = 0xB0,
    REPLY_COMMAND_NOT_IMPLEMENTED = 0xBC,
    REPLY_CHECKSUM_FAILED         = 0xBD,
    REPLY_BUFFER_OVERFLOW         = 0xBF
};

typedef struct ssi_command_packet_frame {
    uint8_t header;
    uint8_t byte_count;
    uint8_t payload[SSI_MAX_PAYLOAD_SIZE];
    uint8_t checksum;
} ssi_command_packet_frame_t;

typedef struct ssi_reply_packet_frame {
    uint8_t reply_code;
    uint8_t payload[SSI_REPLY_PAYLOAD_SIZE];
    uint8_t checksum;
} ssi_reply_packet_frame_t;

static uint8_t getChecksumForCommand(ssi_command_packet_frame_t packet);

static uint8_t getChecksumForCommand(ssi_command_packet_frame_t packet)
{
    uint8_t data;
    uint8_t checksum = 0;

    data = packet.header;
    checksum += data;

    data = packet.byte_count;
    checksum += data;

    uint8_t i;
    uint8_t len = packet.byte_count - SSI_DEFAULT_FRAME_SIZE;
    for (i = 0; i < len; i++) {
        data = packet.payload[i];
        checksum += data;
    }

    checksum = ~checksum + 1;

    return checksum;
}

static uint8_t getChecksumForReply(ssi_reply_packet_frame_t packet)
{
    uint8_t data;
    uint8_t checksum = 0;

    data = packet.reply_code;
    checksum += data;

    uint8_t i;
    uint8_t len = SSI_REPLY_PAYLOAD_SIZE;
    for (i = 0; i < len; i++) {
        data = packet.payload[i];
        checksum += data;
    }

    checksum = ~checksum + 1;

    return checksum;
}

bool readRegister(uint16_t addr, uint32_t* out_value)
{
    ssi_command_packet_frame_t packet;
    
    packet.header = SSI_HEADER;
    
    packet.byte_count = SSI_DEFAULT_FRAME_SIZE + SSI_READ_PAYLOAD_SIZE;

    packet.payload[0] = CMD_SELECT_REGISTER_ADDRESS;
    packet.payload[1] = addr & 0xFF;
    packet.payload[2] = (addr >> 8) & 0xFF;
    packet.payload[3] = CMD_READ_REGITSTER_3BYTES;

    packet.checksum = getChecksumForCommand(packet);

    Serial.flush();

    Serial.write(packet.header);
    Serial.write(packet.byte_count);

    uint8_t i;
    for (i = 0; i < SSI_READ_PAYLOAD_SIZE; i++) {
        Serial.write(packet.payload[i]);
    }

    Serial.write(packet.checksum);

    size_t reply_len = 0;
    uint8_t reply_buffer[6];
    reply_len = Serial.readBytes(reply_buffer, 6);

    if (reply_len != 6) {
        //Serial Error
        return false;
    }

    if (reply_buffer[0] != REPLY_ACK_WITH_DATA) {
        //SSI Error
        return false;
    }

    ssi_reply_packet_frame_t reply_packet;
    memcpy(&reply_packet, reply_buffer, 6);
    
    uint8_t checksum = getChecksumForReply(reply_packet);

    if (reply_packet.checksum != checksum) {
        //Checksum Error
        return false;
    }

    *out_value = ((uint32_t)reply_packet.payload[2]<<16) | ((uint32_t)reply_packet.payload[1]<<8) | (uint32_t)reply_packet.payload[0];

    return true;
}

bool writeRegister(uint16_t addr, uint32_t value)
{
    ssi_command_packet_frame_t packet;
    
    packet.header = SSI_HEADER;

    packet.byte_count = SSI_DEFAULT_FRAME_SIZE + SSI_WRITE_PAYLOAD_SIZE;

    packet.payload[0] = CMD_SELECT_REGISTER_ADDRESS;
    packet.payload[1] = addr & 0xFF;
    packet.payload[2] = (addr >> 8) & 0xFF;
    packet.payload[3] = CMD_WRITE_RETISTER_3BYTES;
    packet.payload[4] = value & 0xFF;
    packet.payload[5] = (value >> 8) & 0xFF;
    packet.payload[6] = (value >> 16) & 0xFF;

    packet.checksum = getChecksumForCommand(packet);

    Serial.flush();

    Serial.write(packet.header);
    Serial.write(packet.byte_count);

    uint8_t i;
    for (i = 0; i < SSI_WRITE_PAYLOAD_SIZE; i++) {
        Serial.write(packet.payload[i]);
    }

    Serial.write(packet.checksum);

    size_t reply_len = 0;
    uint8_t reply = 0;
    reply_len = Serial.readBytes(&reply, 1);


    if (reply_len != 1) {
        //Serial Error
        return false;
    }

    if (reply != REPLY_ACK_WITHOUT_DATA) {
        //SSI Error
        return false;
    }

    return true;
}
