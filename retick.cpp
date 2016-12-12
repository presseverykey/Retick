#include "Arduino.h"
#include "retick.h"

//internal functions

static void initTickTimer();
static void stopTickTimer();
static void startTickTimer(uint32_t ms);

static volatile bool tickStarted = false;
static volatile uint32_t tickMS = 10;
static volatile bool dotick = false;

#define TICK_IRQn    TCC1_IRQn
#define TICK_HANDLER TCC1_Handler
#define TICK_TCC     TCC1

#define WAIT_TIMER_SYNC while(TICK_TCC->SYNCBUSY.reg & TCC_SYNCBUSY_MASK) {}

void retick() {
  if (!tickStarted) {
    initTickTimer();
  }
  stopTickTimer();
  startTickTimer(tickMS);

  if (!tickStarted) {
    tickStarted = true;
    while (1) {
      __asm ( "WFI\n" );
      if (dotick) {
        dotick = false;
        tick();
      }
    }
  }
}

void retick(uint32_t ms) {
//  SysTick_Config( SystemCoreClock / 100 );  //throttle systick - will slow down millis but reduce MCU load
  tickMS = ms;
  retick();
}

extern "C" void TICK_HANDLER(void) {
  NVIC_ClearPendingIRQ(TICK_IRQn);
  TICK_TCC->INTFLAG.reg |= TCC_INTFLAG_CNT;
  WAIT_TIMER_SYNC
  dotick = true;
}

static void initTickTimer() {

  GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TCC0_TCC1));
  while (GCLK->STATUS.bit.SYNCBUSY) {}

  WAIT_TIMER_SYNC
  TICK_TCC->CTRLA.bit.ENABLE = 0;
  WAIT_TIMER_SYNC
  
  TICK_TCC->CTRLA.bit.SWRST = 1;
  WAIT_TIMER_SYNC

  TICK_TCC->CTRLA.bit.PRESCALER = TCC_CTRLA_PRESCALER_DIV1024_Val;
  WAIT_TIMER_SYNC

  //trigger at end of counting
  TICK_TCC->EVCTRL.bit.CNTSEL = TCC_EVCTRL_CNTSEL_END_Val;
  WAIT_TIMER_SYNC

  //enable counter on interrupt
  TICK_TCC->INTENSET.bit.CNT = 1;
  WAIT_TIMER_SYNC

  //enabe interrupt lowest prio
  NVIC_ClearPendingIRQ(TICK_IRQn);
  NVIC_EnableIRQ(TICK_IRQn);
  NVIC_SetPriority(TICK_IRQn, 3); 
}

static void stopTickTimer() {
  TICK_TCC->CTRLA.reg &= ~TCC_CTRLA_ENABLE;
  WAIT_TIMER_SYNC
  TICK_TCC->INTFLAG.reg |= TCC_INTFLAG_CNT;
  WAIT_TIMER_SYNC
  NVIC_ClearPendingIRQ(TICK_IRQn);
  
  dotick = false;
}

static void startTickTimer (uint32_t ms) {

  //Find best divider and counter period
  uint32_t msClockCycles = microsecondsToClockCycles(1000);
  uint64_t count = msClockCycles * ms;
  
  uint32_t dividerShift = 0;
  while (((count >> dividerShift) >= 0xffffffL) && (dividerShift <= 10)) {
    dividerShift++;
  }
  if (dividerShift > 10) {  //too long, take longest interval
    dividerShift = 10;
    count = 0xffffff;
  } else {
    count = count >> dividerShift;
  }
  
  TICK_TCC->CTRLA.bit.PRESCALER = dividerShift;
  WAIT_TIMER_SYNC
  TICK_TCC->PER.reg = count;
  WAIT_TIMER_SYNC
  TICK_TCC->COUNT.reg = 0;
  WAIT_TIMER_SYNC
  
  //start timer
  TICK_TCC->CTRLA.reg |= TCC_CTRLA_ENABLE;
}  

