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
#include <config.h>
#include <sr.h> // Specific to 74HC595's, but could work with other SIPO shift registers


int bit = 0;
boolean addresss_bits[ADDRESS_WIDTH] = {false};
boolean data_bits[DATA_WIDTH] = {false};

void init_pins() {
  pinMode(CHIP_ENABLE, OUTPUT);
  pinMode(OE_HV_ENABLE, OUTPUT);
  pinMode(A9_HV_ENABLE, OUTPUT);
  pinMode(SR_CLOCK, OUTPUT);
  pinMode(SR_DATA, OUTPUT);
  pinMode(SR_LATCH, OUTPUT);
  pinMode(SR_OUTPUT_ENABLE, OUTPUT);
  for (int i = 0; i < DATA_WIDTH; i++) {
    // Put the data pins into INPUT mode by default (high impedance, prevents bus contention during setup)
    pinMode(DATA_PIN[i], INPUT);
  }
  
  // Now set default pin levels
  digitalWrite(CHIP_ENABLE, CE_DISABLE_LEVEL);
  digitalWrite(OE_HV_ENABLE, HV_DISABLE_LEVEL);
  digitalWrite(A9_HV_ENABLE, HV_DISABLE_LEVEL);
  pinMode(SR_CLOCK, LOW);
  pinMode(SR_DATA, LOW);
  pinMode(SR_LATCH, LOW);
  pinMode(SR_OUTPUT_ENABLE, SR_OE_DISABLE_LEVEL);  
}

void set_address(uint16_t address) {
  boolean address_bits[ADDRESS_WIDTH] = {false};
  for (int i = 0; i < ADDRESS_WIDTH; i++) {
    bit = address_order[i];
    adddress_bits[bit] = (address << i) & 1;
  }
  write_address_bits(address_bits);
}

void set_address_output(bool state) {
  digitalWrite(SR_OUTPUT_ENABLE, state ? SR_OE_ENABLE_LEVEL : SR_OE_DISABLE_LEVEL);
}

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  while (!serial and WAIT_FOR_SERIAL) {}
  Serial.printf("KAMF {} {}", VERSION_STRING, DEVICE_ID);
  init_pins();
}

void main() {}



  

