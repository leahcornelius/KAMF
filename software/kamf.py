import hashlib
import os
import serial
import sys
import argparse
import signal
import time

# connects to device over serial port and provides a more human readable interface
# Makes use of the erase ('e'), read ('r') and program block ('pb') commands provided by the device
# but wraps them in a more user friendly interface rather than requiring the user to send raw bytes over minicom

serial_port = '/dev/ttyUSB0'
baud_rate = 115200
DEVICE_READY_MESSAGE = "RTR"

ACK_MESSAGE = "ACK"
NACK_MESSAGE = "NCK"
SEND_DATA_MESSAGE = "SD"
RECEIVE_DATA_MESSAGE = "RD"
ABORT_ACK_MESSAGE = "ABT"
DATA_END_MESSAGE = "ED"
VERBOSE = False
DISABLE_PROGRESS_BAR = False


serial_connection = None
read_speed = 255  # bytes per second
rainbow = ['\033[91m', '\033[93m', '\033[92m',
           '\033[94m', '\033[95m', '\033[96m', '\033[97m']

# Print iterations progress
# Credit: https://stackoverflow.com/questions/3173320/text-progress-bar-in-the-console


def rainbow_print(text):
    for i in range(0, len(text)):
        print(rainbow[i % len(rainbow)] + text[i], end='')
    print("\033[0m")


def print_color(text, color):
    colors = {
        'r': '\033[91m',
        'g': '\033[92m',
        'y': '\033[93m',
        'b': '\033[94m',
        'm': '\033[95m',
        'c': '\033[96m',
        'w': '\033[97m',
        '0': '\033[90m',
        'n': '\033[0m'
    }
    print(colors[color] + text + colors['n'])


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


def close_connection():
    if serial_connection is not None:
        serial_connection.close()


def exit_handler(sig, frame):
    print('Ctrl+C acknowledged, exiting...')
    close_connection()
    sys.exit(0)


def open_port():
    global serial_connection
    serial_connection = serial.Serial(serial_port, baud_rate,
                                      bytesize=serial.EIGHTBITS,
                                      parity=serial.PARITY_NONE,
                                      stopbits=serial.STOPBITS_ONE,
                                      timeout=1,
                                      xonxoff=0,
                                      rtscts=0
                                      )
    # Toggle DTR to reset Arduino
    serial_connection.setDTR(False)
    time.sleep(1)
    # toss any data already received, see
    # http://pyserial.sourceforge.net/pyserial_api.html#serial.Serial.flushInput
    serial_connection.flushInput()
    serial_connection.setDTR(True)


def handshake():
    print("Opened port, handshaking...")
    response = serial_connection.readline()
    readline = response.decode('utf-8')
    while DEVICE_READY_MESSAGE not in readline:
        response = serial_connection.readline()
        readline = response.decode('utf-8')
        if readline != '':
            print(readline, end='')

    print("Device connected successfully")
    return True


def top_menu():
    print("Options:")
    print("1. Erase memory (sets entire memory to 0xFF)")
    print("2. Read memory to file (reads specified memory range and saves to file)")
    print("3. Program memory from file (programs specified memory range from file)")
    print("4. Raw terminal (a simple minicom-like serial terminal for low level debugging)")
    print("5. Toggle verbose mode (like a light switch but with additional misery!)")
    print("6. Exit (This- do i really need to explain this one?)")
    selection = input("Select an option: ")
    return selection


def hex_input(prompt, default=None):
    print(prompt)
    selection = input()
    if selection == '' and default is not None:
        return default
    elif selection == '' and default is None:
        print("Invalid input, please try again")
        return hex_input(prompt, default)

    if selection.startswith('0x'):
        selection = selection[2:]

    try:
        return int(selection, 16)
    except ValueError:
        print("Invalid input, please try again")
        return hex_input(prompt, default)


def raw_terminal():
    print("Raw terminal, press Ctrl+C to exit")
    while True:
        try:
            readline = serial_connection.readline()
            print(readline.decode('utf-8'), end='')
            command = input(": ") + '\r'
            serial_connection.write(command.encode('utf-8'))
        except KeyboardInterrupt:
            break


def erase_device():
    serial_connection.write(b'e\r')
    response = serial_connection.readline()
    readline = response.decode('utf-8')
    serial_connection.write(b'y')
    while ACK_MESSAGE not in readline:
        response = serial_connection.readline()
        readline = response.decode('utf-8')
        readline = readline.strip()
        print_color(readline, 'b')
        if NACK_MESSAGE in readline:
            print_color("Erase failed, exiting...", 'r')
            return False

    print_color("Erase successful", 'g')
    return True


def read_until(message):
    response = serial_connection.readline()
    readline = response.decode('utf-8')
    while message not in readline:
        response = serial_connection.readline()
        if response == b'':
            continue

        readline = response.decode('utf-8')
        if readline != '':
            print(readline, end='')
        if NACK_MESSAGE in readline:
            print("Failed, exiting...")
            return False

    return True


def clear_serial_buffer():
    serial_connection.flushInput()
    serial_connection.flushOutput()


def dump_content(start_address, end_address, filename):
    serial_connection.write('dc {} {}\r'.format(
        start_address, end_address).encode('utf-8'))
    read_until(RECEIVE_DATA_MESSAGE)
    data = bytearray()
    if not DISABLE_PROGRESS_BAR and not VERBOSE:
        printProgressBar(0, end_address - start_address,
                         prefix='Reading memory:', length=50, suffix='0/{}'.format(end_address - start_address))

    read_bytes = 0
    expected_bytes = end_address - start_address
    while read_bytes < expected_bytes:
        # read a chunk of data
        chunk = serial_connection.read(1)
        data += chunk
        read_bytes += 1

        if not VERBOSE and not DISABLE_PROGRESS_BAR:
            printProgressBar(len(data), end_address - start_address,
                             prefix='Reading memory:', length=50, suffix='{}/{}'.format(read_bytes, expected_bytes))

    read_until(DATA_END_MESSAGE)
    print("Saving to file: {}".format(filename))
    clear_serial_buffer()

    with open(filename, 'wb') as f:
        f.write(data)
    print_color("Done!", 'g')


def program_device(start_address, end_address, filename):
    file_data = []
    read_byte = 0
    with open(filename, 'rb') as f:
        read_byte = f.read(1)
        while read_byte != b'':
            file_data.append(read_byte)
            read_byte = f.read(1)

    print("Loaded {} bytes from file".format(len(file_data)))
    if len(file_data) > (end_address - start_address):
        print_color(
            "WARNING: File is larger than specified memory range, data will be truncated; continue? (y/n)", 'r')

        selection = input(": ")
        if selection != 'y':
            return
    elif len(file_data) < (end_address - start_address):
        end_address = start_address + len(file_data)
        print_color("File is smaller than specified memory range, actual end address will be {}".format(
            hex(end_address)), 'y')

    print("Start address{} -> End address: {}".format(hex(start_address), hex(end_address)))

    serial_connection.write('pb {} {}\r'.format(
        start_address, end_address).encode('utf-8'))
    read_until(SEND_DATA_MESSAGE)

    data_index = 0
    current_address = start_address

    printProgressBar(current_address, end_address,
                     prefix='Sending bytes:', suffix='Complete', length=50)
    while current_address < end_address:
        # Send a chunk of 64 bytes, then wait for a . to be sent back
        chunk = file_data[data_index:data_index+64 % len(file_data)]
        for byte in chunk:
            serial_connection.write(byte)
        data_index += 64
        current_address += 64
        while serial_connection.read(1) != b'.':
            pass

        printProgressBar(current_address, end_address,
                         prefix='Sending bytes:', suffix='Complete', length=50)

    time.sleep(0.5)

    print("Sent bytes, awaiting confirmation...")
    read_until(ACK_MESSAGE)

    print("Device acknowledged data, read-back starting...")
    serial_connection.write('\r'.encode('utf-8'))
    read_until(RECEIVE_DATA_MESSAGE)
    read_back_bytes = bytearray()
    rb_index = 0
    printProgressBar(rb_index, end_address - start_address,
                     prefix='Read-back:', suffix='Complete', length=50)
    while True:
        # Read a single byte, which is the raw data read at the current address
        read_byte = serial_connection.read(1)
        read_back_bytes += bytearray(read_byte)
        rb_index += 1
        if rb_index >= (end_address - start_address):
            print("Read-back complete")
            break

        printProgressBar(rb_index, end_address - start_address,
                         prefix='Read-back:', suffix='Complete', length=50)

    read_until(DATA_END_MESSAGE)

    print("Read-back complete, saving to file...")
    with open('read-back.hex', 'wb') as f:
        f.write(read_back_bytes)

    print("Comparing read-back to original file...")
    with open(filename, 'rb') as f:
        original_bytes = f.read()
    
    clear_serial_buffer()

    if original_bytes == read_back_bytes:
        print_color(
            "Read-back matches original file, device programmed successfully!", 'g')
        return True
    else:
        sha1_original = hashlib.sha1(original_bytes).hexdigest()
        sha1_read_back = hashlib.sha1(read_back_bytes).hexdigest()

        print_color(
            "Read-back does not match original file, device programming failed!", 'r')

        print("Length original: {}, Length read-back: {}".format(
            len(original_bytes), len(read_back_bytes)))
        print("SHA1 original: {}, SHA1 read-back: {}".format(sha1_original, sha1_read_back))
        return False




def main():
    global serial_connection, serial_port, baud_rate, VERBOSE, DISABLE_PROGRESS_BAR

    argparser = argparse.ArgumentParser(
        description='KAMF - the Kinda Awful Memory Flasher')
    argparser.add_argument(
        '-p', '--port', help='Serial port to connect to', default=serial_port)
    argparser.add_argument(
        '-b', '--baud', help='Baud rate to connect at', default=baud_rate)

    argparser.add_argument(
        '-v', '--verbose', help='Enable verbose output', default=False, action='store_true')
    argparser.add_argument(
        '-d', '--disable-progress-bar', help='Disable progress bar output', default=False, action='store_true')

    # Options to perform an action and then exit
    argparser.add_argument(
        '-e', '--erase', help='Erase the entire device', default=False, action='store_true')

    argparser.add_argument(
        '-r', '--read', help='Read the entire device', default=False, action='store_true')

    argparser.add_argument(
        '-w', '--write', help='Write a file to the device', default=False, action='store_true')

    argparser.add_argument(
        '-s', '--source', help='File to write to device', default=None)
    argparser.add_argument(
        '-o', '--readoutput', help='File to send the read data to', default="read_data.hex")
    argparser.add_argument(
        '--start-address', help='Start address for read/write operations', default=0)
    argparser.add_argument(
        '--end-address', help='End address for read/write operations', default=0xffff)

    args = argparser.parse_args()
    serial_port = args.port
    baud_rate = args.baud
    VERBOSE = args.verbose
    DISABLE_PROGRESS_BAR = args.disable_progress_bar
    signal.signal(signal.SIGINT, exit_handler)
    erase_mode = args.erase
    read_mode = args.read
    write_mode = args.write

    to_write_filename = args.source
    to_read_filename = args.readoutput
    start_address = args.start_address
    end_address = args.end_address

    rainbow_print("KAMF - the Kinda Awful Memory Flasher")
    rainbow_print("Developed by: Leah Cornelius")
    rainbow_print("Selected serial port: {} at {} baud".format(
        serial_port, baud_rate))
    if VERBOSE:

        print("Verbose output enabled - you fool")
    print("Opening connection...")
    try:
        open_port()
    except serial.SerialException:
        print("ERROR: Unable to open serial port, exiting...")
        sys.exit(1)

    if not handshake():
        print("Handshake failed, exiting...")
        sys.exit(1)

    if read_mode:
        if to_read_filename is None:
            print_color(
                "ERROR: No filename specified for read operation, exiting...", 'r')
            sys.exit(1)
        if start_address is None:
            print_color(
                "ERROR: No start address specified for read operation, exiting...", 'r')
            sys.exit(1)

        dump_content(start_address, end_address, to_read_filename)
        print_color("Read complete", 'g')
        if not erase_mode and not write_mode:
            sys.exit(0)

    if erase_mode:

        print_color("Erasing device, assuming 14v is applied to Vpp pin", 'y')
        if erase_device() == False:
            print_color("ERROR: Erase failed, exiting...", 'r')
            os.system("spd-say 'Erase failed, check console'")
            sys.exit(1)
        print_color("Erase complete", 'g')
        os.system("spd-say 'Erase complete'")

        if not write_mode:
            sys.exit(0)

    if write_mode:
        if to_write_filename is None:
            print_color(
                "ERROR: No filename specified for write operation, exiting...", 'r')
            sys.exit(1)
        if start_address is None:
            print_color(
                "ERROR: No start address specified for write operation, exiting...", 'r')
            sys.exit(1)
        
        if program_device(start_address, end_address, to_write_filename) == False:
            print_color("ERROR: Write failed, exiting...", 'r')
            os.system("spd-say 'Write failed, check console'")
            sys.exit(1)
        print_color("Write complete", 'g')
        os.system("spd-say 'Write complete'")
        sys.exit(0)

    # Loop until user exits
    while True:
        selection = top_menu()
        if selection == '1':
            print("Preparing to erase memory, please wait...")
            print_color(
                "Ensure that 14v is applied to the Vpp pin and you wish to continue (y/n): ", 'r')
            confirmation = input()
            if confirmation == 'y':
                erase_device()
            else:
                print("Aborting erase")
        elif selection == '2':
            start_address = hex_input(
                "Please enter the start address (in hex) to read from (default 0x0000): ", 0)
            end_address = hex_input(
                "Please enter the end address (in hex) to read from (default 0xFFFF): ", 0xFFFF)
            filename = input(
                "Please enter the filename to save to (default 'read.bin'): ")
            if filename == '':
                filename = 'read.bin'
            dump_content(start_address, end_address, filename)
        elif selection == '3':
            print_color(
                "Ensure that 12v is applied to the Vpp pin and you wish to continue (y/n): ", 'r')
            confirmation = input()

            if confirmation != 'y':
                print("Aborting programming")
                continue

            start_address = hex_input(
                "Please enter the start address (in hex) to program from (default 0x0000): ", 0)
            end_address = hex_input(
                "Please enter the end address (in hex) to program from (default 0xFFFF): ", 0xFFFF)
            filename = input(
                "Please enter the filename to read from (default 'corn8.bin'): ")
            if filename == '':
                filename = 'corn8.bin'
            program_device(start_address, end_address, filename)
        elif selection == '4':
            raw_terminal()
        elif selection == '5':
            VERBOSE = not VERBOSE
            print("Verbose output set to {}".format(VERBOSE))
        elif selection == '6':
            print("Exiting...")
            close_connection()
            sys.exit(0)
        else:
            print("Invalid selection, please try again")


if __name__ == "__main__":
    main()
