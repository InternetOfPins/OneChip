#ifdef __AVR__
  #include <chips/avr/avrDevice.h>
  using namespace hw::avr;
#elif defined(__arm__)
  #include <chips/stm32/stm32Device.h>
  using namespace hw::stm32;
#endif

using namespace onePin;
using namespace oneBit;

// ── AVR ──────────────────────────────────────────────────────────────────────
#ifdef __AVR__
  using SysTick = chip::SysTick0<>;
  using Led1    = AVR::OutPin<Pins<5>, chip::PortB>;
  using Board   = AVR::Board<Boot<SysTick>, Led1>;
  #ifdef IOP
  IOP_TIMER0_ISR(Board)
  #endif

// ── STM32 (Blue Pill STM32F103C8) ────────────────────────────────────────────
#elif defined(__arm__)
  using SysTick = chip::SysTick<>;
  using Led1    = STM32::InvOutPin<Pins<13>, chip::PortC>;  // PC13 active-LOW
  using Board   = STM32::Board<Boot<SysTick>, Led1>;
  #ifdef IOP
  IOP_SYSTICK_ISR(Board)
  #endif
#endif
// ─────────────────────────────────────────────────────────────────────────────

Led1 led1;
SysTick::Blink<100, 900> blink1;

int main() {
  Board::begin();
  Board::run([](){
    led1.set(blink1());
  });
}
