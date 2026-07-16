#include <avr/interrupt.h>
#include <chips/avr/avrDevice.h>
using namespace hw::avr;
using namespace hw::avr::chip;  // Explicitly bring chip definitions into scope
using namespace onePin;
using namespace oneBit;

// ── IOP framework: Timer0-based SysTick ──────────────────────────────────────
using SysTick = SysTick0<>;
using Led1    = AVR::OutPin<Pins<0>, PortB>;  // PB0 works across all targets
using Board   = AVR::Board<Boot<SysTick>, Led1>;

#ifdef IOP
IOP_TIMER0_ISR(Board)  // Bare-metal: Timer0 overflow ISR wired to Board
#endif

Led1 led1;
SysTick::Blink<100, 900> blink1;

int main() {
  Board::begin();          // Initialize SysTick + LED
  sei();                   // Enable global interrupts

  while (1) {
    led1.set(blink1());    // Update LED state based on millis()
  }
}
// ─────────────────────────────────────────────────────────────────────────────
