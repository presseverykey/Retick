#ifndef _RETICK_H_
#define _RETICK_H_

//call this in loop() to enable ticks or inside tick() to retrigger/adjust interval. Leaves interval at default or as is.
void retick();

//call this in loop() to enable ticks or inside tick() to retrigger/adjust interval. Specifies new interval.
void retick(uint32_t ms);

//implement this with the actual work - similar to what you did in loop() before.
void tick(void);

#endif
