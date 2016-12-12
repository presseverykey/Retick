#include "Arduino.h"
#include "adc.h"
#include "tempsens.h"
#include "retick.h"

void setup() {
  Serial.begin(115200);
  temp_init();
  adc_init();
}

void loop() {
  retick(1000);
}

void tick() {
  uint32_t temp_raw = temp_read_raw();
  int32_t temp_mdeg = temp_raw_to_mdeg(temp_raw);
  Serial.print("tmp deg: ");
  Serial.print(temp_mdeg/1000.0f);
  Serial.print(" at ");
  Serial.print(millis());
  Serial.print("\n");
}

