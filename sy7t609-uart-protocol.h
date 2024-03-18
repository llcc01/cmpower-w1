#ifndef __SY7T609_UART_PROTOCOL_H__
#define __SY7T609_UART_PROTOCOL_H__

#include <Arduino.h>

/*
 * The SY7T609+S1 firmware implements a "simple serial interfac" (SSI)
 * on the UART interface.
 */

/* Command-Response Mode
 * In this protocol, the host is the master and must initiate communications.
 * The master should first set the device's register address pointer berfore
 * performing read or write operations.
 *
 * After sending the synchronization header code (0xAA), the master sends
 * (in the following order) the byte counts (bytes in payload),
 * the payload and then the checksum that provides data integrity check.
 *
 * # a generic command packet #
 * | Header | Byte | Payload | Checksum |
 * | (0xAA) | Count|         |          |
 *
 *
 * Register Address Pointer Selection
 * |           PAYLOAD          |
 * |  0xA3   | Register Address |
 * | Command | (2 Bytes)        |
 *
 * Read Command
 * # Case1 #                # Case2 #
 * |    PAYLOAD    |        |              PAYLOAD                  |
 * |     0xE3      |        |   0xE0    |          0x1F             |
 * |    Command    |        |  Command  | (Number of Bytes to Read) |
 * Case1) To read 0 to 15 bytes, the command byte is completed with
 *        the number of bytes to read. (ex. to read 3 bytes)
 * Case2) In order to read a larger number of bytes(up to 255), the command
 *        0xE0 must be used. (ex. to read 31 bytes)
 *
 * Write Command
 * # Case1 #
 * |              PAYLOAD              |
 * |   0xD3    |        Data           |
 * |  Command  | (Number of Bytes = 3) |
 *
 * # Case2 #
 * |                     PAYLOAD                    |
 * |   0xD0    |                 Data               |
 * |  Command  | (Number of Bytes = Byte Count - 4) |
 * Case1) To read 0 to 15 bytes, the command byte is completed with
 *        the number of bytes to write. (ex. to write 3 bytes)
 * Case2) In order to read a larger number of bytes(up to 255), the command
 *        0xD0 must be used. (ex. to read 31 bytes)
 */

enum sy7t609_register_map {
  ADDR_COMMAND = 0x0000,
  ADDR_FW_VER = 0x0003,
  ADDR_CONTROL = 0x0006,
  // Metering Address
  ADDR_VAVG = 0x002D,
  ADDR_IAVG = 0x0030,
  ADDR_VRMS = 0x0033,
  ADDR_IRMS = 0x0036,
  ADDR_POWER = 0x0039,
  ADDR_VAR = 0x003C,
  ADDR_FREQUENCY = 0x0042,
  ADDR_AVG_POWER = 0x0045,
  ADDR_PF = 0x0048,
  ADDR_EPPCNT = 0x0069,  // Positive Active Energy Count
  ADDR_EPMCNT = 0x006C,  // Negative Active Energy Count
  ADDR_IPEAK = 0x008A,
  ADDR_VPEAK = 0x0093,
  // I/O Control Address
  ADDR_DIO_DIR = 0x0099,
  ADDR_DIO_SET = 0x009F,
  ADDR_DIO_RST = 0x00A2,
  // Calibration Address
  ADDR_BUCKETL = 0x00C0,
  ADDR_BUCKETH = 0x00C3,
  ADDR_IGAIN = 0x00D5,
  ADDR_VGAIN = 0x00D8,
  ADDR_ISCALE = 0x00ED,
  ADDR_VSCALE = 0x00F0,
  ADDR_PSCALE = 0x00F3,
  ADDR_ACCUM = 0x0105,
  ADDR_IRMS_TARGET = 0x0117,
  ADDR_VRMS_TARGET = 0x011A
};

enum command_register_code {
  CMD_REG_AUTO_REPORTING = 0xAE0001,          // Auto Reporting
  CMD_REF_AUTO_REPORTING_DISABLE = 0xAE0000,  // Auto Reporting Disable
  CMD_REG_CLEAR_ENGERGY_COUNTERS = 0xEC0000,  // Clear All Energy Counters
  CMD_REG_SOFT_RESET = 0xBD0000,              // Invoke Soft-Reset
  CMD_REG_SAVE_TO_FLASH = 0xACC200,
  CMD_REG_CLEAR_FLASH_STORAGE_0 = 0xACC000,
  CMD_REG_CLEAR_FLASH_STORAGE_1 = 0XACC100,
  CMD_REG_CALIBRATION_VOLTAGE = 0xCA0020,
  CMD_REG_CALIBRATION_CURRENT = 0xCA0010,
  CMD_REG_CALIBRATION_ALL = 0xCA0030
};

#define COMMAND_REGISTER_CALIBRATION_MASK (0xFF0000)
#define CONTROL_REGISTER_MASK (0x001815)

bool readRegister(uint16_t addr, uint32_t* value);
bool writeRegister(uint16_t addr, uint32_t value);

#endif  //__SY7T609_UART_PROTOCOL_H__