// ── ATtiny45 / ATtiny13 (minimal blink, direct registers) ──────────────────
#if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny13__)
#include <avr/io.h>

int main() {
  DDRB |= (1 << PB0);  // Configure PB0 as output
  while (1) {
    PORTB |= (1 << PB0);   // LED on
    for (volatile int i = 0; i < 40000; ++i) {}
    PORTB &= ~(1 << PB0);  // LED off
    for (volatile int i = 0; i < 360000; ++i) {}
  }
}

// ── Other AVR targets (ATmega328P & larger) ─────────────────────────────────
#else

#include <chips/avr/avrDevice.h>
using namespace hw::avr;
using namespace hw::avr::chip;  // Explicitly bring chip definitions into scope
using namespace onePin;
using namespace oneBit;

using SysTick = SysTick0<>;
using Led1    = AVR::OutPin<Pins<5>, PortB>;
using Board   = AVR::Board<Boot<SysTick>, Led1>;
#ifdef IOP
IOP_TIMER0_ISR(Board)
#endif

Led1 led1;
SysTick::Blink<100, 900> blink1;

int main() {
  Board::begin();
  Board::run([](){
    led1.set(blink1());
  });
}

#endif
// ─────────────────────────────────────────────────────────────────────────────
