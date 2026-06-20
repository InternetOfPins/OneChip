#include <avr/interrupt.h>
#include <onePin/onePin.h>
#include <chips/avr/avrPort.h>
#include <chips/avr/avrSysClock.h>
#include <OneBit.h>

using namespace onePin; using namespace hapi;
using namespace hw::avr; using namespace oneBit;

using SysTick = chip::SysTick0<>;
using Led     = APIOf<AvrOutPin, Out, Mask<Pins<5>>, chip::PortB>;
using Board   = Device<Boot<SysTick>, Led>;

ISR(TIMER0_OVF_vect) { SysTick::on_overflow(); }

Led led;

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
