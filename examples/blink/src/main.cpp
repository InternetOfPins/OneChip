#include <chips/avr/avrDevice.h>
using namespace hw::avr;
using namespace hw::avr::chip;  // Explicitly bring chip definitions into scope
using namespace onePin;
using namespace oneBit;

// ── All AVR targets: IOP framework with Timer0-based SysTick ─────────────────
using SysTick = SysTick0<>;
using Led1    = AVR::OutPin<Pins<0>, PortB>;  // PB0 works across all tiny targets
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
// ─────────────────────────────────────────────────────────────────────────────
