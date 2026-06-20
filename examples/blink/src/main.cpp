#include <chips/avr/avrDevice.h>
#include <chips/avr/avrSysClock.h>
#include <OneBit.h>

using namespace hapi;
using namespace hw::avr; using namespace oneBit;

using Chip    = AVR;
using SysTick = chip::SysTick0<>;
using Led1    = Chip::OutPin<Pins<5>, chip::PortB>;  // pin 13 — built-in
using Led2    = Chip::OutPin<Pins<4>, chip::PortB>;  // pin 12
using Board   = Chip::Board<onePin::Boot<SysTick>, Led1, Led2>;

Led1 led1;
Led2 led2;

SysTick::Period<500> blink1;
SysTick::Period<250> blink2;

int main() {
  Board::begin();
  sei();
  Board::run([](){
    if (blink1()) led1.set(~led1.get());
    if (blink2()) led2.set(~led2.get());
  });
}
