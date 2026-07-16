#if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  // ATtiny: simple direct port manipulation
  #include <avr/io.h>

  int main() {
    // Set PB0 as output
    DDRB |= (1 << PB0);

    while(true) {
      // Simple blink: toggle every ~32000 CPU cycles
      for(volatile uint32_t i = 0; i < 32000; i++);
      PORTB ^= (1 << PB0);
    }
    return 0;
  }

#else
  // ATmega: use Board pattern with tinyTimeUtils
  #include <chips/avr/avrDevice.h>
  using namespace hw::avr;
  using namespace onePin;
  using namespace oneBit;

  using SysTick = hw::avr::chip::SysTick0<>;
  using Led1 = hw::avr::AVR::OutPin<Pins<5>, hw::avr::chip::PortB>;
  using Board = hw::avr::AVR::Board<Boot<SysTick>, Led1>;

  Led1 led1;
  SysTick::Blink<10, 90> blink1;  // 100ms on, 900ms off

  int main() {
    Board::begin();
    Board::run([]() {
      led1.set(blink1());
    });
    return 0;
  }
#endif
