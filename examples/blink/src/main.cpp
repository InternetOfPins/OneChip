// Blink — Arduino Uno (ATmega328P)
// Built-in LED: digital pin 13 = PB5, bit mask 0x20
//
// Demonstrates OnePin + OneChip port composition with a user-defined HAPI layer.

#ifdef ARDUINO
  #include <Arduino.h>
#endif

#include <onePin/onePin.h>
#include <chips/avr/avrPort.h>

using namespace onePin;
using namespace hapi;
using namespace hw::avr;

using Unit = AvrOutPin::Unit;

// User layer: tracks port state, toggles a bit on blink().
struct Blinker {
  template<typename O>
  struct Part : O {
    using O::O;
    Unit _state = 0;
    void blink(Unit mask) {
      _state ^= mask;
      O::port(_state);
    }
  };
};

// Full composition: AvrOutPin (terminal) ← PortB ← Blinker
using Led = APIOf<AvrOutPin, Blinker, chip::PortB>;
Led led;

static constexpr Unit kLed = 1 << 5;  // PB5 = pin 13

#ifdef ARDUINO
  void setup() { led.dir(kLed); }
  void loop()  { led.blink(kLed); delay(500); }
#else
  int main() { led.dir(kLed); led.blink(kLed); led.blink(kLed); return 0; }
#endif
