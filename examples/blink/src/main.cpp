#include <chips/avr/avrDevice.h>
#include <chips/avr/avrSysClock.h>

using namespace hw::avr;
using namespace onePin;
using namespace oneBit;

using Chip   = AVR;
using SysTick = chip::SysTick0<>;

using Led1  = Chip::OutPin<Pins<5>, chip::PortB>;
using Led2  = Chip::OutPin<Pins<4>, chip::PortB>;
using Board = Chip::Board<Boot<SysTick>, Led1, Led2>;

#ifdef IOP
IOP_TIMER0_ISR(Board)
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
