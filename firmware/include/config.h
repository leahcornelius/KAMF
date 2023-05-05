// EEPROM (or any form of sROM) config
#define ADDRESS_WIDTH 16
#define WORD_SIZE 8
#define MEMORY_SIZE 65536 // 2^16

// Address line shift-register config
const int ADDRESS_ORDER[ADDRESS_WIDTH] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 15, 14, 13, 12, 11, 10, 9}; // 0 is LSB, 15 is MSB
#define ADDRESS_SHIFT_REGISTER_SIZE 16 // 2 8-bit shift registers 

// System config
#define SERIAL_BAUD_RATE 115200 
#define WAIT_FOR_SERIAL true // If true, the program will not continue until a serial connection is established
#define ADDRESS_ENDIANNESS LITTLE_ENDIAN // LITTLE_ENDIAN or BIG_ENDIAN
#define DATA_ENDIANNESS LITTLE_ENDIAN // LITTLE_ENDIAN or BIG_ENDIAN

// Pin outs
#define SR_CLOCK A3 
#define SR_LATCH A2
#define SR_OUTPUT_ENABLE A1
#define SR_DATA A0

#define MEMORY_CHIP_SELECT 12
#define A9_HV_PIN 13
#define OE_HV_PIN 14
#define OE_LOGIC_VOLTAGE_PIN 15

const int DATA_BUS_PINS[WORD_SIZE] = { 10, 8, 7, 6, 5, 4, 3, 2};


// Meta settings

#define VERSION "0.3.1"
#define FIRMWARE_NAME "KAMF"
#define DEVICE_ID "KAMF"
#define VERSION_STRING FIRMWARE_NAME " v" VERSION 
// Logic levels
#define SR_OE_ENABLE_LEVEL LOW // Active low
#define HV_ENABLE_LEVEL HIGH // Active high
#define MEMORY_CHIP_SELECT_LEVEL LOW // Active low
