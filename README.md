# OneChip

**HAPI Compatibility:** Works with new Check/Apply/ApplyPack API (2026-Q2)

Hardware register components for HAPI chains. Provides AVR and STM32 GPIO, timers, interrupts, and chip descriptors. No Arduino or ST HAL dependency — register addresses from datasheets directly.

---

## AVR — `chips/avr/`

### `avrPort.h` — GPIO ports

```cpp
// AVRPort<pIn, ddr = pIn+1, port = pIn+2>
static Unit get()        // read PIN register
static void put(Unit v)  // write PORT register (whole port)
static void on(Unit m)   // PORT |= m
static void off(Unit m)  // PORT &= ~m
static void dir(Unit m)  // DDR = m  (1 = output)
```

Predefined ports (type aliases, no state):

| Namespace     | Ports              |
|---------------|--------------------|
| `mega`        | PortB/C/D          |
| `mega2560`    | PortB/C/D/E/F/G/H/J/K/L |
| `mega1284`    | PortA/B/C/D        |
| `chip`        | macro-selected alias of above |

---

### `avrTC.h` — Timers (TC0 / TC1 / TC2 / TC3 / TC4 / TC5)

```cpp
// mega::TC0<Fn=nullptr>, TC1<Fn>, TC2<Fn>
// mega2560 adds TC3/TC4/TC5
// Fn = compile-time ISR fn (NTTP), nullptr = runtime-only

static void act()                        // call from ISR
static void attach(void(*f)())           // runtime fn binding
static float play(float hz, int duty=50) // set freq+duty, return actual Hz
static void  on(uint16_t ocr, byte cs, byte duty=50)
static void  off()
static void  intA()                      // enable compare-A interrupt
static void  setWaveMode(byte n)
static void  setOutMode_A(byte n)
static void  setOutMode_B(byte n)
static void  setClockSource(byte n)
```

ISR pattern:
```cpp
void myTick() { ... }
using MyTC1 = chip::TC1<myTick>;          // compile-time binding
ISR(TIMER1_COMPA_vect) { MyTC1::act(); }
MyTC1::intA();
```

---

### `avrPcInt.h` — Pin Change Interrupts

```cpp
// mega::PcInt0<Fn=nullptr>, PcInt1<Fn>, PcInt2<Fn>

static void act()               // call from ISR
static void attach(void(*f)())  // runtime fn binding
static void enable(Unit mask)   // set PCMSK bits + PCIE
static void disable(Unit mask)  // clear PCMSK bits; clears PCIE if empty
```

ISR pattern:
```cpp
ISR(PCINT0_vect) { chip::PcInt0<myHandler>::act(); }
chip::PcInt0<myHandler>::enable(1<<5);
```

---

### `watchdog.h`

```cpp
WatchdogClassic::reset()        // wdr instruction
WatchdogClassic::config(uint8_t x)  // WDTCSR unlock + write (ATmega)
WatchdogCcp::config(uint8_t x)      // CCP unlock + write (ATtiny/XMega)
```

### `flashMem.h` — SPM self-programming

```cpp
// FlashMem<Size, PageSize>::Part
static void    erase(uint32_t addr)
static void    fill(uint32_t addr, uint16_t word)
static void    write(uint32_t addr)
static uint8_t read(uint32_t addr)
static uint16_t readWord(uint32_t addr)
```

### `atmega.h` — chip descriptors

Chip type members (all `constexpr`):

```cpp
ATmega88 / ATmega168 / ATmega328 / ATmega1280 / ATmega2560 / ATmega1284P
  ::Watchdog          // WatchdogClassic
  ::Flash             // FlashMem<size, pageSize>
  ::Ram_              // Ram<start, size>
  ::Eeprom_           // Eeprom<size, pageSize>
```

---

## STM32 — `chips/stm32/`

### `stm32Port.h` — GPIO ports

```cpp
// STM32Port<BASE, RCC_ENR_ADDR, RCC_EN_BIT>
static void clockEnable()
static Unit get()             // IDR (lower 16 bits)
static void set(Unit val)     // ODR
static void high(Unit mask)   // BSRR atomic set   (bits 0-15)
static void low(Unit mask)    // BSRR atomic reset  (bits 16-31)
static void mode(uint8_t pin, PinMode m)
static void pull(uint8_t pin, PinPull p)
static void speed(uint8_t pin, PinSpeed s)
static void otype(uint8_t pin, PinType t)
static void altFunc(uint8_t pin, uint8_t af)
```

Enums:
```cpp
PinMode  { Input=0, Output=1, Alt=2, Analog=3 }
PinPull  { None=0, Up=1, Down=2 }
PinSpeed { Low=0, Medium=1, High=2, VeryHigh=3 }
PinType  { PushPull=0, OpenDrain=1 }
```

Predefined ports:

| Namespace | Bus     | RCC register  | Ports         |
|-----------|---------|---------------|---------------|
| `f4`      | AHB1    | 0x40023830    | PortA..I      |
| `l4`/`g4` | AHB2    | 0x4002104C    | PortA..H      |
| `h7`      | AHB4    | 0x580244E0    | PortA..K      |
| `chip`    | macro-selected alias (define `STM32H7`, `STM32L4`, etc.) |

No ST HAL or CMSIS headers required.

---

## Chip alias selection

Define the target macro **before** including chip headers:

```cpp
#define STM32L4          // → chip = l4
#define STM32H7          // → chip = h7
// default             // → chip = f4

// AVR: detected from __AVR_ATmegaXXX__ predefined macros
```

---

## Dependencies

- [HAPI](https://github.com/InternetOfPins/HAPI)
- [OneBit](https://github.com/InternetOfPins/OneBit)
- [OnePin](https://github.com/InternetOfPins/OnePin)

## License

MIT — see [LICENSE](LICENSE).
