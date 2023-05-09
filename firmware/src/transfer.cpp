#include <config.h>

#ifdef BULK_TRANSFER_CODE
// This code manages transfers to and from the device over UART
// This includes "one off" (eg read single byte) as well as bulk (eg write file to ROM) transfers
#include <Arduino.h>

#define BLOCK_SIZE 4       // Size of a single "packet" of data in bytes
#define BULK_BLOCK_COUNT 8 // The number of back-to-back blocks expected in bulk transfer
#define BULK_BYTE_SIZE BULK_BLOCK_COUNT *BLOCK_SIZE

byte block_buffer[BLOCK_SIZE] = {0x00};
byte bulk_transfer_buffer[BULK_BYTE_SIZE] = {0x00};

uint16_t transfer_state = 0; // 0 - IDLE, 1 - awaiting single block, 2 - awaitng next block (bulk), 3 - reading byte, 4 - awaiting next byte, 5 - processing block, 10 - error condition
// 16 bit address
uint16_t address_registerA = 0x0000;
uint16_t address_registerB = 0x0000;
// We only support 8-bit data/word width
uint8_t data_registerA = 0x00;
uint8_t data_registerB = 0x00;

byte checksum_register = 0x00;

void combine_bytes(uint16_t *output, byte high, byte low)
{
    *output = high;         // MSb
    *output = *output << 8; // left shift MSb
    *output |= low;         // OR with LSb
}

byte process_block(uint16_t *address_register, uint8_t *data_register)
{
    byte result = 0;
    // First check the checksum of the block
    for (int block_index = 0; block_index < BLOCK_SIZE; block_index++)
    {
        result += block_buffer[block_index];
    }
    if (result == 0)
    {
        // Get the address (first two bytes)
        combine_bytes(address_register, block_buffer[0], block_buffer[1]);
        // Get the data byte
        *data_register = block_buffer[2];
    }
    return result;
}

void single_block_transfer()
{
    transfer_state = 1;
    for (int block_index = 0; block_index < BLOCK_SIZE; block_index++)
    {
        transfer_state = 3;
        block_buffer[block_index] = Serial.read();
        transfer_state = 4;
    }

    byte process_status = process_block(&address_registerA, &data_registerA);
    transfer_state = process_status != 0 ? 10 : 0;
}

int bulk_transfer(uint16_t *address_array[], uint8_t *data_array[])
{
    // Read BULK_BYTE_SIZE bytes into buffer
    for (int byte_index = 0; byte_index < BULK_BYTE_SIZE; byte_index++)
    {
        bulk_transfer_buffer[byte_index] = Serial.read();
    }
    // Process each block and load values into provided arrays
    int block_start_index = 0;
    for (int block_index = 0; block_index < BULK_BLOCK_COUNT; block_index++)
    {
        block_start_index = block_index * 4;
        for (int block_byte_index = 0; block_byte_index < BLOCK_SIZE; block_byte_index++)
        {
            checksum_register += bulk_transfer_buffer[block_start_index + block_byte_index];
        }
        if (checksum_register != 0)
        {
            return checksum_register;
        }
        combine_bytes(address_array[block_index], bulk_transfer_buffer[block_start_index], bulk_transfer_buffer[block_start_index + 1]);
        *data_array[block_index] = bulk_transfer_buffer[block_start_index + 2];
    }
    return 0;
}
#endif