#ifndef _TEMPSENS_H_
#define _TEMPSENS_H_

/* initialize temperature sensor */
void temp_init();

/* measure temperature. returns a 16 bit voltage 0..1V */
uint32_t temp_read_raw(); 

/* convert a raw temperature to millidegrees, using the device factory calibration values */
int32_t temp_raw_to_mdeg(uint32_t raw); //returns millidegrees

#endif
