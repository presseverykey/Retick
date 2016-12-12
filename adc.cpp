#include "Arduino.h"
#include "adc.h"

#define WAIT_ADC_SYNC while (ADC->STATUS.bit.SYNCBUSY) {}
#define WAIT_ADC_RESET while (ADC->CTRLA.bit.SWRST) {}

void adc_reset() {
  WAIT_ADC_SYNC
  ADC->CTRLA.bit.ENABLE = 0;  //disable DAC
  WAIT_ADC_SYNC
  ADC->CTRLA.bit.SWRST = 1;   //reset ADC
  WAIT_ADC_SYNC
  WAIT_ADC_RESET
}

void adc_init() {
  //make sure the digital interface clock is enabled (should be already after reset)
  PM->APBCMASK.reg |= PM_APBCMASK_ADC;

  // Enable ADC conversion clock, take from GCLK0
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID( GCM_ADC ) |
                      GCLK_CLKCTRL_GEN_GCLK0     |
                      GCLK_CLKCTRL_CLKEN ;

  //reset ADC to defaults
  adc_reset();

  //Control B: Prescaler (/256), resolution selection (16 bit), correction enable (no), freerun mode (no)
  ADC->CTRLB.reg = 0x610;
  WAIT_ADC_SYNC

  //sample time in half clock cycles will be set before conversion, set to max (slow but high impedance)
  ADC->SAMPCTRL.reg = 0x3f;
  WAIT_ADC_SYNC

  //enable accumulation / averaging: 1024 samples to 16 bit resolution
  ADC->AVGCTRL.reg = 0x0a;
  WAIT_ADC_SYNC

  adc_read(0x18, true);  //dummy read a generic channel, discard result
}

uint32_t adc_read(uint8_t channel, bool onevolt) {

  if (onevolt) { //one volt full scale: use internal 1.0V reference, gain 1
    ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_1X_Val;
    ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_INT1V_Val;
    WAIT_ADC_SYNC
    ADC->INPUTCTRL.reg = channel | 0x1800; //select channel as MUXPOS, MUXNEG = internal GND
    WAIT_ADC_SYNC
  } else {  //3.3V full: use half supply as reference, set gain to 0.5
    ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_DIV2_Val;
    ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_INTVCC1_Val;
    WAIT_ADC_SYNC
    ADC->INPUTCTRL.reg = channel | 0x1900; //select channel as MUXPOS, MUXNEG = IO GND
    WAIT_ADC_SYNC
  }  
  
  ADC->SAMPCTRL.reg = 0x3f;            //make sure we're sampling slowly
  WAIT_ADC_SYNC

  ADC->CTRLA.bit.ENABLE = 1;              //enable ADC
  WAIT_ADC_SYNC

  ADC->SWTRIG.bit.START = 1;                // Start ADC conversion
  while (!(ADC->INTFLAG.bit.RESRDY)) {}     //wait until ADC conversion is done
  WAIT_ADC_SYNC
  uint32_t val = ADC->RESULT.reg;

  ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY;  // clear result ready flag
  WAIT_ADC_SYNC
  
  ADC->CTRLA.bit.ENABLE = 0; //disable ADC
  WAIT_ADC_SYNC

  return val;
}


