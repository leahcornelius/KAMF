/*
KAMF - this is intended to be ran on the atmega382p (arduino uno, nano etc) device.
PlatformIO is used, this will not work using the arduino IDE

You probably dont need to change anything in here, most things should be set using the config.h file (in firmware/includes)

Copyright 2023 Leah Cornelius

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include <Arduino.h>
#include <CommandParser.h>

#include <constants.h>
#include <config.h>
#include <w27c.h>

typedef CommandParser<10, 3, 10, 32, 64> MyCommandParser;

MyCommandParser parser;
uint16_t cmd_address;
uint8_t cmd_data;

int bit = 0;
boolean address_bits[ADDRESS_WIDTH] = {false};
boolean data_bits[WORD_SIZE] = {false};

uint32_t failures = 0;

char serial_input_buffer[64];
int serial_input_buffer_index = 0;
char response[MyCommandParser::MAX_RESPONSE_SIZE];

#ifdef STRICT_MODE
int state = 0;
#endif

void set_address_register_state(bool state)
{
  digitalWrite(SR_OUTPUT_ENABLE, state ? SR_OE_ENABLE_LEVEL : !SR_OE_ENABLE_LEVEL);
}

byte parse_data_bits()
{
  byte response = 0;
  for (int i = 0; i < WORD_SIZE; i++)
  {
    response |= (data_bits[i] << i);
  }
  return response;
}

// Bus manipulation functions

void _read_data_bus()
{
  for (int i = 0; i < WORD_SIZE; i++)
  {
    data_bits[i] = digitalRead(DATA_BUS_PINS[i]);
  }
}

void _write_data_bus()
{
  for (int i = 0; i < WORD_SIZE; i++)
  {
    digitalWrite(DATA_BUS_PINS[i], data_bits[i]);
  }
}

void _set_data_bus_mode(bool io)
{
  for (int i = 0; i < WORD_SIZE; i++)
  {
    pinMode(DATA_BUS_PINS[i], io ? INPUT : OUTPUT);
  }
}

void set_data(byte data)
{
  for (int i = 0; i < WORD_SIZE; i++)
  {
    data_bits[i] = (data >> i) & 1; // Get the bit at position i
  }
}

void set_address(uint16_t address)
{
  digitalWrite(SR_LATCH, LOW);
  // digitalWrite(SR_CLOCK, LOW);
  shiftOut(SR_DATA, SR_CLOCK, MSBFIRST, address >> 8);
  // digitalWrite(SR_CLOCK, LOW);
  shiftOut(SR_DATA, SR_CLOCK, MSBFIRST, address);
  digitalWrite(SR_LATCH, HIGH);
}

void reset_shift_register()
{
  set_address_register_state(true);

  digitalWrite(SR_LATCH, LOW);
  digitalWrite(SR_MASTER_RESET, LOW);
  delay(1);
  digitalWrite(SR_MASTER_RESET, HIGH);
  digitalWrite(SR_LATCH, HIGH);

  set_address_register_state(false);
}

void enable_memory(bool enabled)
{
  digitalWrite(MEMORY_CHIP_SELECT, enabled ? MEMORY_CHIP_SELECT_LEVEL : !MEMORY_CHIP_SELECT_LEVEL);
}

void set_OE_pin_state(int state)
{
  if (state == LOW or state == HIGH)
  {
    digitalWrite(OE_HV_PIN, !HV_ENABLE_LEVEL);
    digitalWrite(OE_LOGIC_VOLTAGE_PIN, state);
  }
  else if (state == HIGH_VOLTAGE)
  {
    digitalWrite(OE_HV_PIN, HV_ENABLE_LEVEL);
    digitalWrite(OE_LOGIC_VOLTAGE_PIN, LOW);
  }
}

void set_A9_pin_state(int state)
{
  if (state == HIGH or state == LOW)
  {
    digitalWrite(A9_HV_PIN, !HV_ENABLE_LEVEL);
  }
  else if (state == HIGH_VOLTAGE)
  {
    digitalWrite(A9_HV_PIN, HV_ENABLE_LEVEL);
  }
}

// Read cycle functions
void start_read_cycle()
{
#ifdef STRICT_MODE
  if (state != 0)
  {
    Serial.println("Error: start_read_cycle() called when state != 0");
    return;
  }
  state = 1;
#endif
  enable_memory(false);
  _set_data_bus_mode(true);
  set_A9_pin_state(LOW);
  set_OE_pin_state(HIGH);
}
void end_read_cycle()
{
#ifdef STRICT_MODE
  if (state != 1)
  {
    Serial.println("Error: end_read_cycle() called when state != 1");
  }
  state = 0;
#endif
  enable_memory(false);
  _set_data_bus_mode(true);
  set_OE_pin_state(HIGH);
  set_A9_pin_state(LOW);
}

// Write/program cycle functions
void start_program_cycle()
{
#ifdef STRICT_MODE
  if (state != 0)
  {
    Serial.println("Error: start_program_cycle() called when state != 0");
    return;
  }
  state = 2;
#endif
  enable_memory(false);
  _set_data_bus_mode(false);
  set_OE_pin_state(HIGH_VOLTAGE);
  set_A9_pin_state(LOW);
  set_address_register_state(true);
}

void end_program_cycle()
{
#ifdef STRICT_MODE
  if (state != 2)
  {
    Serial.println("Error: end_program_cycle() called when state != 2");
    return;
  }
  state = 0;
#endif
  enable_memory(false);
  set_A9_pin_state(LOW);
  set_OE_pin_state(HIGH);
  _set_data_bus_mode(true);
  set_address_register_state(false);
}

void init_pins()
{
  pinMode(MEMORY_CHIP_SELECT, OUTPUT);
  pinMode(OE_HV_PIN, OUTPUT);
  pinMode(OE_LOGIC_VOLTAGE_PIN, OUTPUT);
  pinMode(A9_HV_PIN, OUTPUT);
  pinMode(SR_CLOCK, OUTPUT);
  pinMode(SR_DATA, OUTPUT);
  pinMode(SR_LATCH, OUTPUT);
  pinMode(SR_OUTPUT_ENABLE, OUTPUT);
  pinMode(SR_MASTER_RESET, OUTPUT);
  for (int i = 0; i < WORD_SIZE; i++)
  {
    // Put the data pins into INPUT mode by default (high impedance, prevents bus contention during setup)
    pinMode(DATA_BUS_PINS[i], INPUT);
  }

  // Now set default pin levels
  digitalWrite(MEMORY_CHIP_SELECT, !MEMORY_CHIP_SELECT_LEVEL);
  digitalWrite(OE_HV_PIN, !HV_ENABLE_LEVEL);
  digitalWrite(OE_LOGIC_VOLTAGE_PIN, HIGH);
  digitalWrite(A9_HV_PIN, !HV_ENABLE_LEVEL);
  digitalWrite(SR_OUTPUT_ENABLE, !SR_OE_ENABLE_LEVEL);
  digitalWrite(SR_MASTER_RESET, HIGH);
}

void write_byte(uint16_t address, byte data)
{
// must call start_program_cycle() before calling this function!!
#ifdef STRICT_MODE
  if (state != 2)
  {
    Serial.println("Error: write_byte() called when state != 2");
    return;
  }
#endif
  set_address(address);
  set_data(data);
  _write_data_bus();
  delayMicroseconds(3);
  enable_memory(true);
#ifdef SLOW_MODE
  delay(PROGRAM_CE_PULSE_WIDTH);
#else
  delayMicroseconds(PROGRAM_CE_PULSE_WIDTH);
#endif
  enable_memory(false);
  delayMicroseconds(5); // Tdh
}

void read_byte(uint16_t address)
{
// must call start_read_cycle() before calling this function!!
#ifdef STRICT_MODE
  if (state != 1)
  {
    Serial.println("Error: read_byte() called when state != 1");
    return;
  }
#endif
  set_address(address);
  set_address_register_state(true);
  enable_memory(true);
  set_OE_pin_state(LOW);
#ifdef SLOW_MODE
  delay(READ_CE_PULSE_WIDTH);
#elif defined(READ_CE_PULSE_WIDTH)
  delayMicroseconds(READ_CE_PULSE_WIDTH);
#else
  // delay 62.5ns * 2
  NOP;
  NOP;
#endif
  _read_data_bus();
  enable_memory(false);
  set_OE_pin_state(HIGH);
 // set_address_register_state(false);
}

void print_hex(uint16_t number)
{
  sprintf(response, "%X", number);
  Serial.print("0x");
  Serial.print(response);
}

void read_byte_erase_verify(uint16_t address)
{
  set_address(address);
  set_OE_pin_state(LOW);
#ifdef SLOW_MODE
  delay(READ_CE_PULSE_WIDTH);
#elif defined(READ_CE_PULSE_WIDTH)
  delayMicroseconds(READ_CE_PULSE_WIDTH);
#else
  // delay 62.5ns * 2
  NOP;
  NOP;
#endif
  _read_data_bus();
  set_OE_pin_state(HIGH);
  NOP;
}

bool erase_chip(int max_attempts)
{
#ifdef STRICT_MODE
  if (state != 0)
  {
    Serial.println("Error: erase_chip() called when state != 0");
    return;
  }
#endif
  int attempts = 0;
  bool erase_verified = false;
  while ((not erase_verified) and (attempts < max_attempts))
  {
    attempts++;
    Serial.print("Erase attempt ");
    Serial.println(attempts, DEC);
    enable_memory(false);
    _set_data_bus_mode(false);
    set_A9_pin_state(HIGH_VOLTAGE);
    set_OE_pin_state(HIGH_VOLTAGE);
    set_address(0);
    set_address_register_state(true);
    set_data(0xFF);
    _write_data_bus();
    delay(1000);
    enable_memory(true);
    delay(ERASE_CE_PULSE_WIDTH);
    enable_memory(false);
    delayMicroseconds(5);
    _set_data_bus_mode(true);
    set_OE_pin_state(HIGH);
    set_A9_pin_state(LOW);
    enable_memory(true);
    delay(ERASE_COMMENCE_VERIFY_DELAY);
    Serial.println("Verifying erase");
    cmd_address = 0;
    bool failed = false;
    for (uint32_t i = 0; i < MEMORY_SIZE; i++)
    {
      read_byte_erase_verify(cmd_address);
      cmd_data = parse_data_bits();
      if (cmd_data != 0xFF)
      {
        Serial.print("(EV) error ");
        print_hex(cmd_address);
        Serial.print(" read ");
        print_hex(cmd_data);
        Serial.println();
        failed = true;
        break;
      }
      if (cmd_address % 0x1000 == 0)
      {
        Serial.print("(EV): ");
        print_hex(cmd_address);
        Serial.println();
      }
      cmd_address++;
    }

    if (failed)
    {
      Serial.println("EV failed, retrying");
      if (Serial.available())
      {
        if (Serial.read() == 'q')
        {
          Serial.println("Erase cancelled");
          erase_verified = false;
          attempts = max_attempts;
          break;
        }
      }
    }
    else
    {
      erase_verified = true;
    }
  }
  enable_memory(false);
  set_address_register_state(false);
  set_OE_pin_state(HIGH);
  set_A9_pin_state(LOW);
  _set_data_bus_mode(true);

  if (erase_verified)
  {
    Serial.println("Erase verified");
  }
  else
  {
    Serial.println("Erase failed");
  }
  return erase_verified;
}

byte pattern_generator(uint16_t address) {
  return 0x01; // NOP on the 680x
}

void cmd_program_test_pattern(MyCommandParser::Argument *args, char *response)
{
  // Creates a "test pattern" (binary upcount) up to the address specifed by argument 0
  // and programs this to the ROM; then reads it back and prints any errors. Assumes an erased ROM!
  uint16_t end_address = args[0].asUInt64;
  byte read_back_data = 0x00;
  Serial.print("Program test pattern up to ");
  print_hex(end_address);
  Serial.println("; confirm & 12v on Vpp? (y/n)");
  while (!Serial.available())
  {
  }
  if (Serial.read() == 'y')
  {
    Serial.println("Confirmed starting...");
    start_program_cycle();
    delay(1000);
    for (cmd_address = 0; cmd_address < end_address; cmd_address++)
    {
      cmd_data = pattern_generator(cmd_address);
      write_byte(cmd_address, cmd_data);
     // delayMicroseconds(3);
      if (cmd_address % 0x01 == 0)
      {
        Serial.print("(WP): ");
        print_hex(cmd_address);
        Serial.println();
      }
    }
    end_program_cycle();
    set_OE_pin_state(LOW);
    Serial.println("Finished writing pattern, starting program-verify...");
    delay(10);
    _set_data_bus_mode(true);
    set_address(0);
    set_address_register_state(true);
    enable_memory(true);

    failures = 0;
    for (cmd_address = 0; cmd_address < end_address; cmd_address++)
    {
      cmd_data = pattern_generator(cmd_address);
      set_address(cmd_address);
      // delay 62.5ns * 2
      delayMicroseconds(10);
      _read_data_bus();
      read_back_data = parse_data_bits();
      if (cmd_data != read_back_data)
      {
        Serial.print("(PV) addr: ");
        print_hex(cmd_address);
        Serial.print(" expected ");
        print_hex(cmd_data);
        Serial.print(" got ");
        print_hex(read_back_data);
        Serial.println();
        failures += 1;
      }
      else if (cmd_address % 0x01 == 0)
      {
        Serial.print("(PV): ");
        print_hex(cmd_address);
        Serial.println();
      }
    }
    Serial.print("PV done ");
    Serial.print(failures, DEC);
    Serial.println(" fails");
    enable_memory(false);
    set_address(0);
    set_address_register_state(false);
    set_OE_pin_state(HIGH);
    Serial.println("PV done, starting read-verify");
    start_read_cycle();
    failures = 0;
    for (cmd_address = 0; cmd_address < end_address; cmd_address++)
    {
      cmd_data = pattern_generator(cmd_address);
      read_byte(cmd_address);
      read_back_data = parse_data_bits();
      if (cmd_data != read_back_data)
      {
        Serial.print("(RB) addr: ");
        print_hex(cmd_address);
        Serial.print(" expected ");
        print_hex(cmd_data);
        Serial.print(" got ");
        print_hex(read_back_data);
        Serial.println();
        failures += 1;
      }
      else if (cmd_address % 0x10 == 0)
      {
        Serial.print("(RB): ");
        print_hex(cmd_address);
        Serial.println();
      }
    }
    end_read_cycle();
    Serial.print("RB done ");
    Serial.print(failures, DEC);
    Serial.println(" fails");
    strcpy(response, "Test pattern complete");
  }
  else
  {
    strcpy(response, "Write test pattern cancelled");
  }
}

void cmd_erase(MyCommandParser::Argument *args, char *response)
{
  Serial.println("Set VPP to 14v then enter 'y' to continue or 'n' to cancel");
  while (!Serial.available())
  {
    // Wait for user input
  }
  if (Serial.read() == 'y')
  {
    strcpy(response, erase_chip(10) ? "Erase complete" : "Erase Failed");
  }
  else
  {
    strcpy(response, "erase cancelled");
  }
}

void cmd_begin_read(MyCommandParser::Argument *args, char *response)
{
  start_read_cycle();
  strcpy(response, "read cycle started");
}

void cmd_end_read(MyCommandParser::Argument *args, char *response)
{
  end_read_cycle();
  strcpy(response, "read cycle finished");
}

void cmd_read(MyCommandParser::Argument *args, char *response)
{
  cmd_address = args[0].asUInt64;
  read_byte(cmd_address);
  sprintf(response, "%x: %x", cmd_address, parse_data_bits());
}

void cmd_echo(MyCommandParser::Argument *args, char *response)
{
  strcpy(response, args[0].asString);
}

void cmd_write_byte(MyCommandParser::Argument *args, char *response)
{
  cmd_address = args[0].asUInt64;
  cmd_data = args[1].asUInt64;
  write_byte(cmd_address, cmd_data);
  sprintf(response, "%x: %x", cmd_address, cmd_data);
}

void cmd_begin_program(MyCommandParser::Argument *args, char *response)
{
  start_program_cycle();
  strcpy(response, "program cycle started");
}

void cmd_end_program(MyCommandParser::Argument *args, char *response)
{
  end_program_cycle();
  strcpy(response, "program cycle finished");
}

void setup()
{
  init_pins();
  reset_shift_register();
  delay(1);
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println(VERSION_STRING);
  Serial.println("rtr");
  parser.registerCommand("r", "u", cmd_read);
  parser.registerCommand("br", "", cmd_begin_read);
  parser.registerCommand("er", "", cmd_end_read);
  parser.registerCommand("echo", "s", cmd_echo);

  parser.registerCommand("p", "uu", cmd_write_byte);
  parser.registerCommand("e", "", cmd_erase);
  parser.registerCommand("bp", "", cmd_begin_program);
  parser.registerCommand("ep", "", cmd_end_program);
  parser.registerCommand("wp", "u", cmd_program_test_pattern);
}

void loop()
{
  if (Serial.available())
  {
    serial_input_buffer[serial_input_buffer_index] = Serial.read();
    if (serial_input_buffer[serial_input_buffer_index] == '\r')
    {
      serial_input_buffer[serial_input_buffer_index] = '\0';
      parser.processCommand(serial_input_buffer, response);
      Serial.println(response);
      serial_input_buffer_index = 0;
      for (uint16_t i = 0; i < 64; i++)
      {
        serial_input_buffer[i] = 0x00;
      }
      for (uint16_t i = 0; i < MyCommandParser::MAX_RESPONSE_SIZE; i++)
      {
        response[i] = 0x00;
      }
      // Ctrl+c
    }
    else if (serial_input_buffer_index == 64 or serial_input_buffer[serial_input_buffer_index] == 3)
    {
      Serial.println("Command cancelled");
      serial_input_buffer_index = 0;
    }
    else
    {
      serial_input_buffer_index++;
    }
  }
}
