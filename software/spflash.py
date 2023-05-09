# This script writes a intel hex formated file to the arduino based ROM/(s)RAM programer over serial
# It can also read he content of the memory and save it to a file
# 3 required arguments:
# port (string) - the path pointing to the arduino (ie /dev/ttyUSB0 etc)
# file (string) - the path of the file you wish to flash - this must be in intel hex format
# address offset (hexadecimal)- the address of the file to be treated as the first line to flash to the memory
# Additonal optional argument:
# output_path (string): if specified, the contents of the memory will first be read and saved to this file before writing the new file
# Eg:
# python3 spflash.py /dev/ttyACM0 test.hex 0xF000 memory_dump.bin

# Copyright: Leah Cornelius @ Cornelius Innovations 
# Date: May 2023

import serial
import sys
import argparse

from intelhex import IntelHex

# Constants/ config
BAUD_RATE = 9600
VERBOSE_WRITE = True
VERBOSE_READ = True
SKIP_VERIFY = False
SKIP_WRITE = False
SKIP_ERASE = False

# Global vars
file_name = None
file_stream = IntelHex()
file_content = {}

dump_existing_values = False
memory_dump_path = None

serial_port = None
serial_connection = None
serial_device_id = None

sram_size = 65536  # bytes
sram_word_size = 8  # bits
address_offset = 0


def max_word_value():
    return (1 << sram_word_size) - 1


def max_address_value():
    return sram_size - 1

# Print iterations progress
# Credit: https://stackoverflow.com/questions/3173320/text-progress-bar-in-the-console


def printProgressBar(iteration, total, prefix='', suffix='', decimals=1, length=100, fill='█', printEnd="\r"):
    """
    Call in a loop to create terminal progress bar
    @params:
        iteration   - Required  : current iteration (Int)
        total       - Required  : total iterations (Int)
        prefix      - Optional  : prefix string (Str)
        suffix      - Optional  : suffix string (Str)
        decimals    - Optional  : positive number of decimals in percent complete (Int)
        length      - Optional  : character length of bar (Int)
        fill        - Optional  : bar fill character (Str)
        printEnd    - Optional  : end character (e.g. "\r", "\r\n") (Str)
    """
    percent = ("{0:." + str(decimals) + "f}").format(100 *
                                                     (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print(f'\r{prefix} |{bar}| {percent}% {suffix}', end=printEnd)
    # Print New Line on Complete
    if iteration == total:
        print()

# Address should be an integer between 0 and SRAM_SIZE
# Returns the data at the address as an integer

def create_test_pattern():
    # Create a test pattern, set the data at address 0 to address % 256
    for i in range(0, sram_size):
        file_stream[i] = i % 256
    

def read_byte(address):
    global serial_connection
    if not serial_connection:
        raise ValueError("Serial connection not open")

    if (address < 0 or address >= sram_size):
        raise ValueError("Address out of range: {} (decimal: {}) [{} -> {}]".format(
            hex(address), address, 0, hex(sram_size-1)))

    command = "r {}\r".format(address)
    serial_connection.write(command.encode())
    _, response = read_line()
    if (VERBOSE_READ):
        print("({}): {}".format(serial_device_id, response), end='')

    # address: data (both hex)
    response_address = int(response.split(':')[0].strip(), 16)
    response_data = int(response.split(':')[1].strip(), 16)
    if (response_address != address):
        raise ValueError("Address mismatch: expected {} but got {}".format(
            hex(address), hex(response_address)))

    return response_data


def write_byte(address, data):
    global serial_connection
   # print ("Writing {} to {} ({}: {})".format(hex(data), hex(address), address, data))
    
    if not serial_connection:
        raise ValueError("Serial connection not open")

    if (address < 0 or address >= sram_size):
        raise ValueError("Address out of range: {} (decimal: {}) [{} -> {}]".format(
            hex(address), address, 0, hex(sram_size-1)))

    if (data < 0 or data > max_word_value()):
        raise ValueError("Data out of range: {} (decimal: {}) [{} -> {}]".format(
            hex(data), data, 0, hex(max_word_value())))

    command = "p {} {}\r".format(address, data)
    serial_connection.write(command.encode())
    _, response = read_line()
    if (VERBOSE_WRITE):
        print("({}): {}".format(serial_device_id, response))

    response_address = int(response.split(':')[0].strip(), 16)
    response_data = int(response.split(':')[1].strip(), 16)
    if (response_address != address):
        raise ValueError("Address mismatch: expected {} but got {}".format(
            hex(address), hex(response_address)))

    if (response_data != data):
        raise ValueError("Data mismatch: expected {} but got {}".format(
            hex(data), hex(response_data)))

    return True

# Reads the entire memory and saves it to a file

def start_read_cycle():
    serial_connection.write("br\r".encode())
    print("SRR: {}".format(read_line()))

def end_read_cycle():
    serial_connection.write("er\r".encode())
    print("ERR: {}".format(read_line()))
    

def start_write_cycle():
    serial_connection.write("bp\r".encode())
    print("SWR: {}".format(read_line()))
    

def end_write_cycle():
    serial_connection.write("ep\r".encode())
    print("EWR: {}".format(read_line()))

def dump_memory():
    global memory_dump_path
    if not memory_dump_path:
        raise ValueError("No memory dump path specified")

    start_read_cycle()
    print("Dumping memory to {}".format(memory_dump_path))
    printProgressBar(0, sram_size, prefix='Progress:',
                     suffix='Complete', length=50)
    with open(memory_dump_path, "wb") as memory_dump_file:
        for address in range(sram_size):
            data = read_byte(address)
            memory_dump_file.write(data.to_bytes(1, byteorder='big'))
            printProgressBar(address + 1, sram_size,
                             prefix='Progress:', suffix='Complete', length=50)
            
    end_read_cycle()

    print("Memory dump complete")

def erase_memory():
    sel = input("Ensure 14v is present on Vpp and press y to continue (or any other key to abort):")
    if (sel == 'y'):
        print("Erasing memory...")
        serial_connection.write("e\r".encode())
        print(read_line())
        serial_connection.write("y".encode())
        print(read_line())


        print("Memory erase complete")


def flash_memory():
    global file_content, address_offset

    if not SKIP_WRITE:
        print("Flashing memory...")
        printProgressBar(0, sram_size, prefix='Progress:',
                        suffix='Complete', length=50)
        start_write_cycle()
        sram_address = 0
        for address in file_content:
            if (address < address_offset):
                continue
            data = file_content[address]
            write_byte(sram_address, data)
            sram_address += 1
            printProgressBar(sram_address, sram_size,
                            prefix='Progress:', suffix='Complete', length=50)
        end_write_cycle()
        print("Memory flash complete")
    else:
        print("Skipping flash")
    
    if not SKIP_VERIFY:
        print("Verifying memory...")
        printProgressBar(0, sram_size,
                         prefix='Progress:', suffix='Complete', length=50)
        sram_address = 0
        start_read_cycle()
        for address in file_content:
            if (address < address_offset):
                continue
            data = file_content[address]
            read_data = read_byte(sram_address)
            if (data != read_data):
                print("Verification failed at address {}: expected {} but got {}".format(
                    hex(sram_address), hex(data), hex(read_data)))
                write_byte(sram_address, data)
                #address -= 1
            else:
                printProgressBar(sram_address+1, sram_size,
                                 prefix='Progress:', suffix='Complete', length=50)

            sram_address += 1
        end_read_cycle()
        print("Memory verification complete")
    else:
        print("Skipping verify")

def read_line():
    global serial_device_id
    while (serial_connection.in_waiting == 0):
        pass
    response = serial_connection.readline().decode()
    data = response.strip()
    if VERBOSE_READ:

        print("{}: {}".format(serial_device_id, data))
    return serial_device_id, data

def main():
    global serial_connection, file_stream, file_content, address_offset, dump_existing_values, memory_dump_path, serial_device_id, serial_port, file_name
    print("Cornelius Innovations - Memory Basher V2.3")
    print("File name: {}".format(file_name))
    print("Serial Port: {}, Baudrate: {}".format(serial_port, BAUD_RATE))

    print("Openning file...")
    file_stream.fromfile(file_name, format='hex')
    file_content = file_stream.todict()
    print("File opened successfully")
    print("File stats: {} bytes, {} lines".format(
        len(file_content), len(file_stream)))

    print("Opening serial connection...")
    try:
        serial_device_id = serial_port.split('/')[-1]

        print("Opened serial connection to {}".format(serial_port))
        serial_connection = serial.Serial(serial_port, BAUD_RATE, timeout=1)
        # Wait for a RTS signal
        serial_read_line = ""
        while ("rtr" not in serial_read_line):
            if (serial_connection.in_waiting > 0):
                serial_read_line = serial_connection.readline().decode("utf-8")
                print("({}): {} ".format(serial_device_id, serial_read_line), end='')
        print("Serial connection opened successfully")
    except Exception as e:
        print("Failed to open serial connection:")
        print(e)
        return

    if (dump_existing_values):
        dump_memory()
    else:
        print("Skipping memory dump")
    

    if (not SKIP_ERASE):
        erase_memory()
    else:
        print("Skipping memory erase")
    
    flash_memory()
    print("Goodbye!")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="Bit Basher",
        description='Cornelius Innovations - Bit Basher V2.3')
    parser.add_argument('serial_port', type=str, help='Serial port of the device')
    parser.add_argument('file_name', type=str, help='File to flash')
    parser.add_argument('address_offset', type=str, help='Address offset')
    parser.add_argument('--dump', type=str, help='Dump memory to file')
    parser.add_argument('--no-write', help='Disables writing to the chip (will only verify existing values)', action='store_true')
    parser.add_argument('--verbose', help='Enables verbose output', action='store_true')
    parser.add_argument('--skip-verify', help='Disables post-write verifcation', action='store_true')
    parser.add_argument('--skip-erase', help='Disables memory erase', action='store_true')
    args = parser.parse_args()
    serial_port = args.serial_port
    file_name = args.file_name
    address_offset = args.address_offset
    if (address_offset.startswith("0x")):
        address_offset = int(address_offset[2:], 16)
    elif (address_offset.startswith("0b")):
        address_offset = int(address_offset[2:], 2)
    else:
        address_offset = int(address_offset)

    if (args.dump):
        memory_dump_path = args.dump
        dump_existing_values = True
    
    SKIP_VERIFY = args.skip_verify
    SKIP_WRITE = args.no_write
    SKIP_ERASE = args.skip_erase
    VERBOSE_WRITE = args.verbose
    VERBOSE_READ = args.verbose
        
    main()
