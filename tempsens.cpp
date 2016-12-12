#include "Arduino.h"
#include "tempsens.h"
#include "adc.h"

#define ADC_TEMP_CHANNEL 0x18

void temp_init() {
  SYSCTRL->VREF.reg |= SYSCTRL_VREF_TSEN;   //enable temperature sensor, route to ADC
  adc_init();
}

uint32_t temp_read_raw() {  //returns 16 bit.
  return adc_read(ADC_TEMP_CHANNEL, true);
}

int32_t temp_raw_to_mdeg(uint32_t raw) {
  int32_t adc = raw;
  //use device factory calibration values for temperature conversion (simplified)
  uint32_t* tmpLogBase = (uint32_t*)0x00806030;
  uint32_t tmpLog0 = tmpLogBase[0];
  uint32_t tmpLog1 = tmpLogBase[1];
  uint8_t roomInt = tmpLog0 & 0xff;
  uint8_t roomFrac = (tmpLog0 >> 8) & 0x0f;
  uint8_t hotInt = (tmpLog0 >> 12) & 0xff;
  uint8_t hotFrac = (tmpLog0 >> 20) & 0x0f;
  int32_t roomADC = ((tmpLog1 >> 8) & 0xfff) << 4;
  int32_t hotADC = ((tmpLog1 >> 20) & 0xfff) << 4;
  int32_t roomMdeg = 1000 * roomInt + 100 * roomFrac;
  int32_t hotMdeg = 1000 * hotInt + 100 * hotFrac;
  int32_t mdeg = roomMdeg + ((hotMdeg-roomMdeg) * (adc-roomADC)) / (hotADC-roomADC);
  return mdeg;
}


