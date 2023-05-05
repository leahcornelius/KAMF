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
#include <shift_register.h>
#include <w27c.h>
#include <CommandParser.h>

typedef CommandParser<> MyCommandParser;

MyCommandParser parser;

int bit = 0;
boolean address_bits[ADDRESS_WIDTH] = {false};
boolean data_bits[WORD_SIZE] = {false};

void serial_print_message(const char* message) {
  Serial.printf("(%s) ", DEVICE_ID);
  Serial.println(message);
}

// Bus manipulation functions

void _read_data_bus() { 
  for (int i = 0; i < WORD_SIZE; i++) {
    data_bits[i] = digitalRead(DATA_BUS_PINS[i]);
  }  
}

void _write_data_bus() {
  for (int i = 0; i < WORD_SIZE; i++) {
    digitalWrite(DATA_BUS_PINS[i], data_bits[i]);
  }
}

void _set_data_bus_mode(bool io) {
  for (int i = 0; i < WORD_SIZE; i++) {
    pinMode(DATA_BUS_PINS[i], io ? INPUT : OUTPUT);
  }
}

void set_data(byte data) {
  for (int i = 0; i < WORD_SIZE; i++) {
    data_bits[i] = (data >> i) & 1; // Get the bit at position i
  }
}

void set_address(uint16_t address) {
  boolean address_bits[ADDRESS_WIDTH] = {false};
  for (int i = 0; i < ADDRESS_WIDTH; i++) {
    bit = ADDRESS_ORDER[i];
    address_bits[bit] = (address << i) & 1;
  }
  write_address_bits(address_bits);
}

void enable_memory(bool enabled) {
  digitalWrite(MEMORY_CHIP_SELECT, enabled ? MEMORY_CHIP_SELECT_LEVEL : !MEMORY_CHIP_SELECT_LEVEL);
}

void set_OE_pin_state(int state) {
  if (state == LOW) {
    digitalWrite(OE_HV_PIN, !HV_ENABLE_LEVEL);
    digitalWrite(OE_LOGIC_VOLTAGE_PIN, LOW);
  } else if (state == HIGH) {
    digitalWrite(OE_HV_PIN, !HV_ENABLE_LEVEL);
    digitalWrite(OE_LOGIC_VOLTAGE_PIN, HIGH);
  } else if (state == HIGH_VOLTAGE){
    digitalWrite(OE_HV_PIN, HV_ENABLE_LEVEL);
    digitalWrite(OE_LOGIC_VOLTAGE_PIN, LOW);
  }
}

// Read cycle functions
void start_read_cycle() {
  enable_memory(false);
  _set_data_bus_mode(true);
  set_OE_pin_state(LOW);
}
void end_read_cycle() {
}



// Write/program cycle functions
void start_program_cycle() {
  enable_memory(false);
  _set_data_bus_mode(false);
  set_OE_pin_state(HIGH_VOLTAGE);
}
void end_program_cycle() {
  set_OE_pin_state(HIGH);
}

void init_pins() {
  pinMode(MEMORY_CHIP_SELECT, OUTPUT);
  pinMode(OE_HV_PIN, OUTPUT);
  pinMode(A9_HV_PIN, OUTPUT);
  pinMode(SR_CLOCK, OUTPUT);
  pinMode(SR_DATA, OUTPUT);
  pinMode(SR_LATCH, OUTPUT);
  pinMode(SR_OUTPUT_ENABLE, OUTPUT);
  for (int i = 0; i < WORD_SIZE; i++) {
    // Put the data pins into INPUT mode by default (high impedance, prevents bus contention during setup)
    pinMode(DATA_BUS_PINS[i], INPUT);
  }
  
  // Now set default pin levels
  digitalWrite(MEMORY_CHIP_SELECT, !MEMORY_CHIP_SELECT_LEVEL);
  digitalWrite(OE_HV_PIN, !HV_ENABLE_LEVEL);
  digitalWrite(A9_HV_PIN, !HV_ENABLE_LEVEL);
  digitalWrite(SR_OUTPUT_ENABLE, !SR_OE_ENABLE_LEVEL);  
  digitalWrite(SR_CLOCK, LOW);
  digitalWrite(SR_DATA, LOW);
  digitalWrite(SR_LATCH, LOW);
}

void write_byte(uint16_t address, byte data) {
  // must call start_program_cycle() before calling this function!!
  set_address(address);
  set_address_register_state(true);
  set_data(data);
  _write_data_bus();
  enable_memory(true);
  delayMicroseconds(PROGRAM_CE_PULSE_WIDTH);
  enable_memory(false);
  set_address_register_state(false);
}

void read_byte(uint16_t address) {
  // must call start_read_cycle() before calling this function!!
  set_address(address);
  set_address_register_state(true);
  enable_memory(true);
  delayMicroseconds(READ_CE_PULSE_WIDTH);
  _read_data_bus();
  enable_memory(false);
  set_address_register_state(false);
}

bool erase_chip() {
  serial_print_message("Please ensure 14v is applied to high voltage input pin, then enter 'y' to continue or 'n' to cancel");
  while (!Serial.available()) {
    // Wait for user input
  }
  if (Serial.read() == 'y') {
    serial_print_message("Erasing chip...");
    digitalWrite(A9_HV_PIN, HV_ENABLE_LEVEL);
    set_address_register_state(true);
    set_address(0);
    set_data(0xFF);
    set_OE_pin_state(HIGH_VOLTAGE);
    _write_data_bus();
    enable_memory(true);
    delay(ERASE_CE_PULSE_WIDTH);
    enable_memory(false);
    _set_data_bus_mode(true);
    set_address_register_state(false);
    set_OE_pin_state(LOW);
    digitalWrite(A9_HV_PIN, !HV_ENABLE_LEVEL);
    serial_print_message("Done - chip erased");
  } else {
    serial_print_message("Cancelled chip erase");
    return false;
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial && WAIT_FOR_SERIAL) {
    // Wait for serial connection
  }
  serial_print_message(VERSION_STRING);
  init_pins();
  serial_print_message("rtr");
  parser.registerCommand("p", "uu", cmd_write);
  parser.registerCommand("e", "", cmd_erase);
  parser.registerCommand("r", "u", cmd_read);
  parser.registerCommand("br", "", cmd_begin_read);
  parser.registerCommand("bp", "", cmd_begin_program);
  parser.registerCommand("ep", "", cmd_end_program);
  parser.registerCommand("er", "", cmd_end_read);
}

void loop() {}



  

