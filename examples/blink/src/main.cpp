#include <chips/avr/avrDevice.h>
#include <chips/avr/avrSysClock.h>
#include <chips/avr/frameworkClock.h>
#include <OneBit.h>

using namespace hapi;
using namespace hw::avr; using namespace oneBit;

using Chip = AVR;

#ifdef IOP
  using SysTick = chip::SysTick0<>;
#else
  using SysTick = FrameworkClock;
#endif

using Led1  = Chip::OutPin<Pins<5>, chip::PortB>;  // pin 13 — built-in
using Led2  = Chip::OutPin<Pins<4>, chip::PortB>;  // pin 12
using Board = Chip::Board<onePin::Boot<SysTick>, Led1, Led2>;

#ifdef IOP
ISR(TIMER0_OVF_vect) { Board::onOverflow(); }
#endif

Led1 led1;
Led2 led2;

SysTick::Blink<10, 290> blink1;
SysTick::Blink<500>     blink2;

int main() {
  Board::begin();
  Board::run([](){
    led1.set(blink1());
    led2.set(blink2());
  });
}
