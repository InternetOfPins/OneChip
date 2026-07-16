#include <avr/interrupt.h>
#include <chips/avr/avrDevice.h>
using namespace hw::avr;
using namespace hw::avr::chip;  // Explicitly bring chip definitions into scope
using namespace onePin;
using namespace oneBit;

// ── Bare-metal IOP: Timer0-based SysTick, manual interrupt handling ──────────
using SysTick = SysTick0<>;
using Led1    = AVR::OutPin<Pins<0>, PortB>;  // PB0 works across all tiny targets
using Board   = AVR::Board<Boot<SysTick>, Led1>;

IOP_TIMER0_ISR(Board)  // Timer0 overflow ISR wired to Board

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
