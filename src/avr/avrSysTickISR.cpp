#ifdef __AVR__
#include <avr/interrupt.h>
#include <chips/avr/avrSysClock.h>

using namespace hw::avr;

// Weak ISR — wired automatically when SysTick0/SysTick2 is used in Boot<>.
// Override with a strong ISR in the user file if custom behavior is needed.

void TIMER0_OVF_vect(void) __attribute__((signal, used, externally_visible, weak));
void TIMER0_OVF_vect(void) { chip::SysTick0<>::onOverflow(); }

void TIMER2_OVF_vect(void) __attribute__((signal, used, externally_visible, weak));
void TIMER2_OVF_vect(void) { chip::SysTick2<>::onOverflow(); }

#endif
