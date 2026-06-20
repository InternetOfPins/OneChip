#include <avr/interrupt.h>
#include <chips/avr/avrDevice.h>
#include <chips/avr/avrSysClock.h>
#include <OneBit.h>

using namespace hapi;
using namespace hw::avr; using namespace oneBit;

using Chip    = AVR;
using SysTick = chip::SysTick0<>;
using Led     = Chip::OutPin<Pins<5>, chip::PortB>;
using Board   = Chip::Board<onePin::Boot<SysTick>, Led>;

Led led;

ISR(TIMER0_OVF_vect) { SysTick::on_overflow(); }

int main() {
  Board::begin();
  sei();
  Board::run([](){
    static uint32_t last = 0;
    if (SysTick::millis() - last >= 500) {
      last = SysTick::millis();
      led.set(~led.get());
    }
  });
}
