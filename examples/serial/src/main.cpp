#ifdef __AVR__
  #include <chips/avr/avrDevice.h>
  #include <oneBus/uart.h>
  using namespace hw::avr;
#elif defined(__arm__)
  #include <chips/stm32/stm32Device.h>
  #include <oneBus/uart.h>
  using namespace hw::stm32;
#endif

using namespace onePin;
using namespace oneBit;

// ── AVR (Uno / Mega) ─────────────────────────────────────────────────────────
#ifdef __AVR__
  using SysTick = chip::SysTick0<>;
  using Ser     = chip::Serial0<9600>;      // USART0 — D0/D1 on Uno
  using Board   = AVR::Board<Boot<SysTick>>;
  #ifdef IOP
  IOP_TIMER0_ISR(Board)
  #endif

// ── STM32 (Blue Pill — 72 MHz via CMSIS SystemInit PLL) ─────────────────────
#elif defined(__arm__)
  using SysTick = chip::SysClk<>;                  // 72 MHz default (f1)
  using Ser     = chip::Serial0<9600>;              // USART1 — PA9 TX / PA10 RX
  using Board   = STM32::Board<Boot<SysTick>>;
  #ifdef IOP
  IOP_SYSTICK_ISR(Board)
  #endif
#endif
// ─────────────────────────────────────────────────────────────────────────────

SysTick::Period<500> period;

int main() {
  Board::begin();
  Ser::begin();
  Ser::println("IOP Serial ready");
  Board::run([]() {
    if (period()) Ser::println("tick");
    if (Ser::available()) Ser::putch(Ser::getch());  // echo
  });
}
