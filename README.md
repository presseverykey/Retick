# *Retick*: Fixing the Arduino loop paradigm

Everyone loves Arduino. Everyone uses Arduino (well, maybe except for me). Arduino has become a rich and extensive ecosystem that makes embedded development easy for beginners, hiding most of the low level stuff beneath the surface and shallowing the steep learning curve usually involved in advanced electronics projects.

However, Arduino's simple programming model comes at a price: It is fundamentally wrong. Many things can be achieved with it, but it makes it impossible to use the hardware's full potential. Sometimes, it is simply annoying and counterproductive. The makers of Arduino are well aware of this fact, they deliberately sacrifice the full hardware potential for the simplicity of the programming model. They explicitly mention in their documentation that it's not the right way to do it, but the easy way.

Let's see why: Most problems of Arduino's programming model arise from the fact that it is strictly sequential and synchronous. That's nice and easy to use, but it has problems in at least two aspects (there are more, but I will focus on just two of them):

## Fast loop

Everything you do happens in the "main thread" (the microcontroller's regular execution mode, outside interrupts), inside two functions: setup() and loop(). setup() is executed once in the beginning and intended for configuration at startup, loop() is triggered repeatedly and should contain the things your program does on a regular basis. loop() is executed as fast as possible. In other words: When your loop() function returns, it is immediately called again.

Running as fast as possible is great, right? So why should that be a bad thing? There are several reasons:

First: It's not needed in most cases. Most embedded applications perform controlling tasks. They measure sensors or wait for certain events and then react accordingly by triggering certain actions. Say, for example, you want to build an automatic light switch: Your application should monitor an ambient light sensor and turn on the room's lights if it becomes too dark outside. So your loop would read the light sensor, compare that value to a threshold value (or do some more fancy calculations) and finally set an output pin that controls a light relay or something similar. Over and over again. Assume that reading your light sensor takes one microsecond. Comparing and outputting the relay pin is neglectible in computing time. So your loop would sense the light at almost a million times per second. That's not bad per se, but it is complete overkill. It would be more than sufficient to sense the light level, say, a hundred times per second. It would still feel perfectly instantaneous to humans. So running the loop faster does not make it a better light controller. This holds true for most controlling applications: The required minimum speed is determined by the application, not by your microcontroller's speed. If it is fast enough: fine. If not: Buy a faster microcontroller (or use it more efficiently).

There are some applications that need continuous or repeated actions in the microsecond range. But in these cases, it's usually better (in the non-Arduino world) to delegate these tasks to one of the microcontroller's peripherals. They are specifically built for these things and really good to do this sort of tasks efficiently without the processor's intervention at all. Another option is not to use repeated polling at all, but to let peripherals notify the processor of relevant events. In other words: Fast busy polling is a bad thing and avoidable in most cases.

Second: It prevents power management. If we would throttle our light sensor to sense the ambient light at 100Hz, this would leave the processor idle for more than 99% of the time. This idle ratio is not unusual. Modern microcontrollers offer convenient sleep modes with just a fraction of the power consumption compared to active code execution. So if the controller could distinguish between useful work and just hanging around, it could go to a lower power state in its idle time. This could easily reduce the power consumption of our light sensor. But the loop-as-fast-as-possible approach keeps the controller busy 100% of the time. There's no way for the processor to know when it can save energy.

For example, this is an issue for battery operated devices or energy harvesting applications. Ever wondered why your battery-powered Arduino gadget runs out of juice so fast? That's why. But it's not only the amount of energy that enters the system. The same amount of energy has to leave the system again - as heat. Your processor is not likely to burst into flames, but it may be an issue.

A real example: I stumbled across this problem reently when I wanted to use the internal temperature sensor of a SAMD21 microcontroller on an Adafruit Feather M0 board to measure the ambient temperature. The kind of project required the use of the Arduino ecosystem. The temperature readings (after I managed to get them) were basically quite nice - low noise and apparently fast reaction to ambient temperature changes. But they were too high by a couple of degrees, even after correcting them with the calibration data that's put into every single device at production time (thanks, Atmel!). Why? The internal temperature sensor obviously measures the temperature inside the chip. It is warmer there because the Arduino loop keeps the processor busy all the time and generates heat for no good reason. Wouldn't it be nice to put the processor into sleep just before the measurement? Granted, it would still not be a precision thermometer, but we could reduce our error offset, maybe enabling it for everyday low precision thermometer applications - without having to add an external device (in the real, non-Arduino world, saving hardware components means lower cost and higher profit, however in this case, it means that we could keep on using an existing prototype design that's already in the field).

Third: It may be inconvenient. Your loop() function will end up being called at a frequency determined by its execution time (and other processor load). If the execution time is constant, it will be called regularly. If it is not constant, it will be called at irregular intervals. This makes timed tasks more troublesome to write than needed. Say, for example, you want to smoothly dim an LED stripe from zero to full brightness in one second. So in your loop function you have to remember the start time, read the current time, calculate time intervals and the current brightness. This usually involves lots of millis() calls and linear interpolation calculations. It would make life much easier if we could get our loop() function to be called in a defined interval (say, 100 times a second and just increase the brightness by 1% per iteration). In fact, I found myself more than once writing timing code to throttle my loop execution just to get a defined interval. It does make life easier.

## Blocking peripherals

The second issue is similar in nature. It is meant as a simplification for beginners, but may cause problems when you want more. The good thing: You usually don't have to deal with the inner guts of the microcontroller's peripherals: No register setup, no interrupt handling, no concurrency, nothing. You call a function to do something with a peripheral, and it usually returns when that task is done. This makes it increadibly easy to do things in a sequential fashion (say, reading a light sensor, then calculating something and then setting an output pin). Great!
So what the heck is wrong with that again? First: It makes it extremely difficult to do more than one thing in parallel. For example, your application is blocked when you write something to a serial interface (say, UART, SPI or I2C), so you can't do anything if some input event happens during that time. And second, as in the loop case before, the processor is actively blocking and waiting for the peripheral to complete its task, generating heat from electricity with all transistors it has to offer.

## What can we do?

These problems don't exist per se. As mentioned above, they were deliberately introduced by the Arduino people to get an otherwise easy to use environment. Their success makes it seem to be a good choice. However, when you touch the technical limits of the ecosystem, it would be nice to at least alleviate the problems a bit. An obvious approach to solving the problems is not to use the Arduino programming model at all, but that would leave us with all the difficulties and low level stuff that we tried to avoid in the first place. So let's try to keep as much of the Arduino simplicity while improving at least a bit on efficiency. The goals:

- Keep the Arduino simplicity, don't add too much more complexity
- Keep backwards compatibilty, don't change existing semantics
- Offer a simple, low-overhead migration path that works for many existing projects
- Enable at least a bit of power management
- Offer a pragmatic solution that makes real life easier
- Add on top of the Arduino runtime, don't change it (for now)

Given these goals, there's not much we can do about the blocking peripherals issue. Accessing peripherals in a non-blocking way would require a significantly different and more complex programming model - callbacks, interrupt management, re-entrant code, some sort of scheduling or dynamic event queueing, dealing with racing conditions and so forth. If you want to do that, take the bare metal programming model for your microcontroller or use an RTOS. It's the professional way to do it, but it's a long and winding road.

Furthermore, blocking peripheral access and strict sequencing are even burdens when you try to make things better: We know that loop() may take a very short or very long time to finish. Say, for example, you have an internet-connected device. An Arduino programmer would typically put a whole internet transaction into one loop() call. This may take several seconds to finish (btw: no one would do this outside the Arduino community). If there's no internet communication going on in a loop() cycle, it may be very short. We have to live with this wide variance.

But we can do something about the loop-as-fast-as-possible thing. What if we could say that we want our regular tasks to be throttled to a regular or maximum frequency? We could continue to code almost the same way as we did before, even take advantage of a regular heartbeat, and the processor would know that it can save energy between the loop() calls? Great!

## *Retick*
*Retick* basically keeps everything as is. It just adds two things:

- `tick()` is a throttled version of `loop()`. You simply put everything you would usually do in `loop()` in `tick()`. Same thing.</li>
- `retick()` is the way for the programmer to say that he or she prefers throttled calls to `tick()` instead of the traditional `loop()` mechanism. You typically write this as one single statement inside your old `loop()` function. *Retick* can be called with no arguments (which will either take the default interval or the one previously specified) or with one argument that specifies the throttle interval in milliseconds.

So this traditional program:

```C
void setup() {
	//do your typical one-time setup here
}

void loop() {
	//do your typical repetitive task here
}
```

becomes:

```C
void setup() {
	//do your typical one-time setup here
}

void loop() {
	retick(10);
}

void tick() {
	//do your typical repetitive task here
}
```

That's it. `tick()` now does the same thing as `loop()`, but it's only executed every 10ms max (100 times per second). The processor automatically saves energy between the calls.

## What happens if...

... your `tick()` function takes more than the specified interval to finish? It's executed directly again. Say you specified an interval of 10ms, but your `tick()` function took 12ms to finish. Then `tick()` is called directly again, so it will be delayed by 2ms in comparison to the specified interval. Obviously, the processor cannot save energy since there's no time between the calls. If your next `tick()` execution is fast again (say, 1ms), you will return to the usual 10ms grid. If your `tick()` call regularly takes more than the specified interval to finish, you will fully saturate the processor, ending up in a program that does pretty much the same thing as the old loop() style.
Delayed executions don't queue up. For example, if you specified a tick interval of 10ms, your `tick()` function usually executes within 1ms but every once in a while takes one full second to complete, the next `tick()` will be called once immediately after the long call, but then return to the normal 10ms interval. You don't get the missed 100 calls in fast succession. The ticks inbetween are dropped. This is usually the behaviour you want: Check once if you missed something, then return to normal.

<h2>Can I...</h2>
... change the interval or change the start of it? Yes, you can. Simply call `retick()` again inside your `tick()` function. So if you know at the end of your current tick that you want to be called again in one second from now, call retick(1000). The countdown starts at the time you call `retick()`, so you will get the next `tick()` call in one second and then in second intervals (unless you change the interval again by calling `retick()` over and over). This mechanism can be used to change the interval or to specify a fixed sleep duration instead of a fixed tick interval. For this to happen, simply call `retick()` without arguments at the very end of your `tick()` function. This will reload the countdown, so for example, if you specified 10ms as tick interval, your `tick()` takes 5ms to execute and you call `retick()` without arguments at the end, the effective interval will be roughly 15ms (5ms execution + previously specified 10ms starting at the end of your tick).

## Current status

*Retick* currently exists in a very early proof-of-concept state. It is only implemented for Atmel Cortex M0 processors and only tested on SAMD21-based Adafruit Feather M0 boards (in fact, it's barely tested at all, I played around a bit and it worked for me). Feel free to improve, extend it to your architecture or make it a better Arduino citizen.

## How it's done

Cortex M processors have a common dedicated timer peripheral for this type of regular tasks: SysTick. Unfortunately, this timer is already used by the Arduino implementation, mainly for two tasks: Updating the millisecond counter (aka `millis()`) and monitoring the soft-reset feature (returning to programming the device when you press the upload button in your Arduino IDE while your sketch is running on the target device). SysTick runs at 1kHz. There is a hook to attach user code to it, however, we cannot attach `tick()` code execution to it - if `tick()` exceeds the 1ms mark, systick calls would be dropped and our `millis()` would break.
So instead we use another timer peripheral: TCC1 (fortunately, the SAMD21 has plenty of timers). It is configured as a basic repetitive timer which triggers interrupts at given intervals. The timer interrupt itself does not execute the `tick()` callback, instead, it just sets a flag that tick has to be called.
When `retick()` is called for the first time, the timer is reset, initialized and configured to do its job. After that, the main thread goes into an endless loop that calls WFI ("wait for interrupt", a dedicated Cortex M assembly instruction) in each cycle. WFI will put the processor to a light sleep mode and return whenever an interrupt was executed. When the tick-needs-to-be-called flag was set, the flag is cleared, `tick()` is executed once and then the processor goes to sleep again in the next iteration. This mechanism allows `tick()` to run in the regular execution context, outside interrupts, keeping the remaining Arduino world untouched. So in fact, the first call to `retick()` never returns. Following calls to `retick()` don't go into that loop since it is already running.

## Known limitations

As mentioned, *Retick* uses the TCC1 timer for its operation. As a consequence, functions that rely on this timer will not work and are likely to break functionality. This might be the case for certain PWM pins. Make sure you don't use hardware based PWM on pins that utilize TCC1 as their PWM source.
There might be some issues with interrupt disabling and/or timer overflows, however, I have not stumbled across them yet.
Since the Arduino runtime uses the SysTick timer in 1ms intervals (and the USB interrupts if you're connected to the Arduino IDE), you will not get the full energy saving potential - the controller still wakes up a couple of thousand times per second. If you want to go truly low energy, you would need to put more effort into either tweaking the Arduino runtime or put the processor into a lower sleep mode. This is out of scope of this project.
The *Retick* timing mechanism is intended for throttling and non-critical timing, not for precise time measurements. You can do basic timing with it, but if you need high precision, use something else (i.e. a dedicated timer).

## Usage

Currently, *Retick* comes in to simple source files: `retick.h` and `retick.cpp`. Simply copy them from tha sample project (which reads the Feather M0's internal temperature sensor once per second) into your project, add `#include "retick.h"` to the top of your main file, move all your stuff from 'loop()' to 'tick()' and add a single line `retick()` inside your otherwise empty `loop()` function. That's it.

## Conclusion
*Retick* certainly doesn't solve all of Arduino's limitations and does not make it a professional development platform for production use, but it might help make this world a slightly better place.
