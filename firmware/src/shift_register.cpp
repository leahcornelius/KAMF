#include "shift_register.h"

void write_address_bits(boolean address_bits[ADDRESS_WIDTH]) {
  digitalWrite(SR_LATCH, LOW);
  for (int i = 0; i < ADDRESS_WIDTH; i++) {
    digitalWrite(SR_DATA, address_bits[i] ? HIGH : LOW);
    digitalWrite(SR_CLOCK, HIGH);
    digitalWrite(SR_CLOCK, LOW);
  }
  digitalWrite(SR_LATCH, HIGH);
}

void set_address_register_state(bool state) {
  digitalWrite(SR_OUTPUT_ENABLE, state ? SR_OE_ENABLE_LEVEL : !SR_OE_ENABLE_LEVEL);
}