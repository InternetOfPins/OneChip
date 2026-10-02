# OneChip API Reference

Platform abstraction layer for microcontroller hardware (AVR, ESP32, STM32, RISC-V).
Provides zero-overhead hardware access via compile-time resolution.

## Quick Start

```cpp
#include <chips/avr/avrDevice.h>
using namespace hw::avr;

// Define hardware
using Led = AVR::OutPin<Pins<5>, chip::PortB>;
Led led;

// Use it
led.dir(1<<5);  // Set PB5 as output
led.on();       // PB5 high
led.off();      // PB5 low
```

---

## Hardware Families

### AVR (8-bit: ATmega/ATtiny)

**Include**: `<chips/avr/avrDevice.h>` (ATmega) or `<chips/avr/attiny.h>` (ATtiny)

**Devices**: ATmega328P, ATmega2560, ATtiny45, ATtiny85

#### Ports & GPIO

| Return | Function | Params | Description |
|--------|----------|--------|-------------|
| `OutPin<M,P>` | `AVR::OutPin` | `Pins<N>, Port` | Output pin abstraction |
| `InPin<M,P>` | `AVR::InPin` | `Pins<N>, Port` | Input pin (pull-up capable) |
| `IOPin<M,P>` | `AVR::IOPin` | `Pins<N>, Port` | Bidirectional pin |
| `uint8_t` | `Port::pin()` | – | Read PORT register (GPIO state) |
| `uint8_t` | `Port::ddr()` | – | Read DDR register (direction) |
| `void` | `Port::dir_out(uint8_t m)` | mask | Set pins as outputs |
| `void` | `Port::dir_in(uint8_t m)` | mask | Set pins as inputs |
| `void` | `Port::port(uint8_t v)` | value | Write PORT register |

**Port Aliases** (via `namespace chip`):
- `chip::PortA`, `chip::PortB`, `chip::PortC`, `chip::PortD` (device-specific)

#### Timers & PWM

| Return | Function | Params | Description |
|--------|----------|--------|-------------|
| `OC<addr,tccr,...>` | `AVR::OC0A`, `OC0B`, `OC1A`, `OC1B` | – | PWM output compare pin |
| `void` | `OC::enable()` | – | Start PWM on pin |
| `void` | `OC::disable()` | – | Stop PWM on pin |

#### Interrupts & Edge Detection

| Return | Function | Params | Description |
|--------|----------|--------|-------------|
| `PcIntC<P0,P1,P2>` | `chip::OnChange` | Pins 0..2 | PORTC pin-change interrupt (ATmega) |
| `PcIntC<P0,P1,P2>` | `chip::OnRise` | Pins 0..2 | Rising edge only (PORTC) |
| `PcIntC<P0,P1,P2>` | `chip::OnFall` | Pins 0..2 | Falling edge only (PORTC) |
| `void` | `ChangeSource::begin()` | – | Enable interrupt source |
| `uint8_t` | `ChangeSource::read()` | – | Read current pin state |
| `bool` | `ChangeSource::changed()` | – | Did state change since last read? |

#### System Clock & Timing

| Return | Function | Params | Description |
|--------|----------|--------|-------------|
| `SysTick0<Hz>` | `chip::SysTick0` | CPU frequency (default 16 MHz) | System tick (Timer0-based) |
| `uint32_t` | `SysTick::millis()` | – | Milliseconds since boot |
| `bool` | `SysTick::Period<ms>::elapsed()` | ms value | Periodic timer check |
| `bool` | `SysTick::Blink<on_ms, off_ms>::operator()` | – | Return LED state for on/off cycle |

**Board Wrapper**:
| Return | Function | Params | Description |
|--------|----------|--------|-------------|
| `void` | `Board::begin()` | – | Initialize all peripherals + enable interrupts |
| `void` | `Board::run(lambda)` | fn | Poll loop with SysTick (Arduino-compatible) |

---

### ESP8266 (Xtensa LX106)

**Include**: `<chips/esp8266/esp8266Device.h>` (Arduino framework)

**Devices**: Wemos/LOLIN D1 mini, NodeMCU, ESP-01

| Return | Function | Params | Description |
|--------|----------|--------|-------------|
| `OutPin<N>` | `hw::esp8266::OutPin` | GPIO number | Digital output (begin/on/off/set/get) |
| `InPin<N,Pull>` | `hw::esp8266::InPin` | GPIO number, pull-up | Digital input |
| core | `hw::esp8266::Esp8266SpiCore` | – | HSPI core (SCK 14, MISO 12, MOSI 13): `spi_init`, `spi_setup(hz, mode)`, `spi_transfer`; CS is the layer above (`oneBus::CsPin`, `oneBus::SpiSlots`) |
| `Spi<Hz>` | `hw::esp8266::esp8266::Spi` | clock | `SpiMaster<Hz>` over that core |
| `TwiMaster` | `hw::esp8266::Esp8266TwiMaster<SDA,SCL,Hz>` | pins, clock | I2C over Arduino Wire (bit-banged) |

---

### ESP32 (32-bit Xtensa)

**Include**: `<chips/esp32/esp32Device.h>`

**Devices**: LOLIN32, LOLIN S2, LOLIN C3

#### GPIO

| Return | Function | Params | Description |
|--------|----------|--------|-------------|
| `OutPin<M,P>` | `ESP32::OutPin` | `Pins<N>, Port` | GPIO digital output |
| `InPin<M,P>` | `ESP32::InPin` | `Pins<N>, Port` | GPIO digital input |
| `void` | `GpioInt<GPIO...>::begin()` | – | Attach GPIO interrupt handler |
| `uint32_t` | `GpioInt<GPIO...>::read()` | – | Read GPIO level |
| `bool` | `GpioInt<GPIO...>::changed()` | – | Level changed since last read? |

#### WiFi & BLE

- `BTOut`, `BtFmt` — BLE GATT record output
- `BTRec` — BLE characteristic descriptor

#### Timing

| Return | Function | Params | Description |
|--------|----------|--------|-------------|
| `uint32_t` | `millis()` | – | Milliseconds since boot |

---

### STM32 (32-bit ARM Cortex-M)

**Include**: `<chips/stm32/stm32Device.h>`

**Devices**: STM32F103 (Blue Pill), STM32F030, STM32L0

#### GPIO

| Return | Function | Params | Description |
|--------|----------|--------|-------------|
| `OutPin<M,P>` | `STM32::OutPin` | `Pins<N>, Port` | GPIO push-pull output |
| `InPin<M,P>` | `STM32::InPin` | `Pins<N>, Port` | GPIO input with pull-up |
| `void` | `ExtiInt<PIN...>::begin()` | – | Set up EXTI interrupt lines |
| `uint32_t` | `ExtiInt<PIN...>::read()` | – | Read current GPIO state |
| `bool` | `ExtiInt<PIN...>::changed()` | – | External interrupt pending? |

#### Timing

| Return | Function | Params | Description |
|--------|----------|--------|-------------|
| `uint32_t` | `SysClk::millis()` | – | Milliseconds since boot |

---

## Platform-Agnostic Input (OnePin + OneChip Integration)

**Interrupt source abstraction** — same driver code on all platforms:

```cpp
// Define platform pins
using MyInt = chip::OnChange<A0, A1, A2>;  // Resolves per platform

// Check for changes
if (MyInt::changed()) {
  uint8_t state = MyInt::read();  // Get pin state
  handle(state);
}
```

**Supported markers** (platform-independent):
- `chip::OnChange<Pins...>` — Any edge (rise or fall)
- `chip::OnRise<Pins...>` — Rising edge only  
- `chip::OnFall<Pins...>` — Falling edge only

---

## Integration with OneMenu

**Input Drivers** (platform-agnostic, work on all chips):

| Type | Purpose | Include |
|------|---------|---------|
| `EncoderIn<ChangeSource, Steps>` | Quadrature rotary encoder + button | `<oneMenu/encoderIn.h>` |
| `DebouncedButton<ChangeSource, Ms>` | Debounced push-button | `<oneMenu/debouncedButton.h>` |
| `EdgeDetector<ChangeSource>` | Rise/fall edge detector | `<oneMenu/edgeDetector.h>` |

**Example**: Encoder on any platform
```cpp
using MyEnc = EncoderIn<chip::OnChange<A0, A1, A2>, 4>;
MyEnc encoder;

// In poll loop
if (encoder.available()) {
  CKE cmd = encoder.cmd();  // Up, Down, or Enter
  handle_menu_command(cmd);
}
```

---

## Memory Efficiency Examples

| Target | Flash | RAM | Purpose |
|--------|-------|-----|---------|
| ATmega328P (Uno) + tinyTimeUtils | 562 B | 9 B | LED blink |
| ATtiny45 (direct port) | 126 B | 0 B | Minimal blink |
| ESP32 | ~1-2 KB | ~50 B | GPIO + WiFi ready |
| STM32F103 | ~1-3 KB | ~100 B | GPIO + timer ready |

---

## Compile-Time Platform Resolution

OneChip automatically selects the correct implementation based on compiler target:

```cpp
#include <chips/avr/avrDevice.h>

// Resolves to:
// - __AVR__ → hw::avr (ATmega/ATtiny)
// - __arm__ → hw::stm32 (STM32)
// - ESP32 → hw::esp32 (Xtensa)
```

No runtime branching—zero overhead.

---

## Common Patterns

### Initialize a Pin
```cpp
using namespace hw::avr;
using Led = AVR::OutPin<Pins<5>, chip::PortB>;
Led led;
led.dir(1<<5);    // Configure as output
```

### Edge-Triggered Interrupt
```cpp
using Int = chip::OnChange<0, 1, 2>;
Int::begin();     // Enable interrupt source
if (Int::changed()) {
  uint8_t state = Int::read();
  process(state);
}
```

### Timer-Based Blinking
```cpp
using SysTick = chip::SysTick0<>;
using Led = AVR::OutPin<Pins<5>, chip::PortB>;
using Board = AVR::Board<Boot<SysTick>, Led>;

Led led;
SysTick::Blink<100, 900> blink;  // 100ms on, 900ms off

int main() {
  Board::begin();
  Board::run([](){ led.set(blink()); });
}
```

---

## See Also

- [OnePin](../../../OnePin/docs/README.md) — GPIO abstraction
- [OneBit](../../../OneBit/docs/README.md) — Bit masking utilities
- [OneMenu](../../../OneMenu/docs/README.md) — Menu UI framework
- [HAPI](../../../HAPI/docs/README.md) — Hardware abstraction patterns
