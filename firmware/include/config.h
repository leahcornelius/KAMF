#if !defined(CONFIG_H)
#define CONFIG_H

// EEPROM (or any form of sROM) config
#define ADDRESS_WIDTH 16
#define WORD_SIZE 8
#define MEMORY_SIZE 65536 // 2^16

// Address line shift-register config
const int ADDRESS_ORDER[ADDRESS_WIDTH] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 15, 14, 13, 12, 11, 10, 9}; // 0 is LSB, 15 is MSB
#define ADDRESS_SHIFT_REGISTER_SIZE 16 // 2 8-bit shift registers 

// System config
#define SERIAL_BAUD_RATE 115200 
#define WAIT_FOR_SERIAL false // If true, the program will not continue until a serial connection is established
#define ADDRESS_ENDIANNESS LITTLE_ENDIAN // LITTLE_ENDIAN or BIG_ENDIAN
#define DATA_ENDIANNESS LITTLE_ENDIAN // LITTLE_ENDIAN or BIG_ENDIAN

// Pin outs
#define SR_CLOCK 2
#define SR_LATCH 3
#define SR_OUTPUT_ENABLE 13
#define SR_DATA 4

#define MEMORY_CHIP_SELECT A2
#define A9_HV_PIN A0
#define OE_HV_PIN A1
#define OE_LOGIC_VOLTAGE_PIN A3

const int DATA_BUS_PINS[WORD_SIZE] = { 12, 11, 10, 9, 8, 7, 6, 5 }; // 0 is LSB, 7 is MSB

// Compile time options
//#define STRICT_MODE // Causes the device to check the currently set mode before every operation (write, read, etc) - this is slow but useful for testing software on the sender's side 
#define SLOW_MODE // Causes the device to use delays instead of microsecond delays - this is useful for debugging but the chip should be removed and instead LED's or similar used as indicators
//#define BULK_TRANSFER_CODE // Include the code from transfer.cpp 

// Meta settings
#define VERSION "0.4.1"
#define FIRMWARE_NAME "KAMF"
#define DEVICE_ID "N1A" 
#define VERSION_STRING FIRMWARE_NAME " v" VERSION 
// Logic levels
#define SR_OE_ENABLE_LEVEL LOW // Active low
#define HV_ENABLE_LEVEL HIGH // Active high
#define MEMORY_CHIP_SELECT_LEVEL LOW // Active low
#endif // CONFIG_H
