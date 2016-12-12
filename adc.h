#ifndef _ADC_H_
#define _ADC_H_

/* initialize the ADC */
void adc_init();

/* reset the ADC to factory settings */
void adc_reset();

/* read an ADC channel synchronously.
@param adc_channel channel to read
@param onevolt if true, this is an internal measurement against internal 1.0V reference. Otherwise, it is an external 3.3V read. */
uint32_t adc_read(uint8_t adc_channel, bool onevolt);

#endif
