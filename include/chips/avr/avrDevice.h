#pragma once
#include <chips/avr/avrSysClock.h>
#include <chips/avr/avrPort.h>
#include <chips/avr/avrUart.h>
// TWI/SPI not available on ATtiny family
#if !defined(__AVR_ATtiny13__) && !defined(__AVR_ATtiny45__) && !defined(__AVR_ATtiny85__)
  #include <chips/avr/avrTwi.h>
  #include <chips/avr/avrSpi.h>
  #include <chips/avr/avrPcIntC.h>
#endif

namespace hw::avr {

  namespace detail {
    struct IsAVRPort {
      template<typename T, typename = void>
      struct Check : std::false_type {};
      template<typename T>
      struct Check<T, std::void_t<typename T::is_avr_port>> : T::is_avr_port {};
    };

    template<typename PC>
    using avr_port_t = decltype(onePin::detail::firstInChain<onePin::detail::IsPort>(
      typename PC::Types::Tail{}));

    template<typename PC>
    static constexpr bool is_avr_peripheral =
      IsAVRPort::template Check<avr_port_t<PC>>::value;
  }

  // avr/io.h defines AVR=1 — undefine before using it as a struct name.
  #undef AVR
  struct AVR {
    AVR() = delete;

    template<typename MaskDesc, typename Port>
    using OutPin    = hapi::APIOf<onePin::AvrOutPin, onePin::Out, oneBit::Mask<MaskDesc>, Port>;

    template<typename MaskDesc, typename Port>
    using InvOutPin = hapi::APIOf<onePin::AvrOutPin, onePin::Out, oneBit::Inverted<>, oneBit::Mask<MaskDesc>, Port>;

    template<typename MaskDesc, typename Port>
    using InPin  = hapi::APIOf<onePin::AvrInPin,  onePin::In,  oneBit::Mask<MaskDesc>, Port>;

    template<typename MaskDesc, typename Port>
    using IOPin  = hapi::APIOf<onePin::AvrIOPin,              oneBit::Mask<MaskDesc>, Port>;

    template<typename Boot, typename... Peripherals>
    struct Board : onePin::Device<Boot, Peripherals...> {
      Board() = delete;
      static_assert((detail::is_avr_peripheral<Peripherals> && ...),
        "AVR::Board: all peripherals must use AVR ports");

      static void begin() {
        onePin::Device<Boot, Peripherals...>::begin();
#ifdef __AVR__
        sei();
#endif
      }
    };
  };

  // ── Interrupt source aliases (OnChange/OnRise/OnFall) ────────────────
  // Maps chip::OnChange<> to platform-specific implementation
  // (ATmega uses PcIntC, ATtiny uses platform-specific handlers)
#if !defined(__AVR_ATtiny13__) && !defined(__AVR_ATtiny45__) && !defined(__AVR_ATtiny85__)
  namespace interrupt_sources {
    template<uint8_t Pin0, uint8_t Pin1 = 0xFF, uint8_t Pin2 = 0xFF>
    using OnChange = PcIntC<Pin0, Pin1, Pin2>;

    template<uint8_t Pin0, uint8_t Pin1 = 0xFF, uint8_t Pin2 = 0xFF>
    using OnRise = PcIntC<Pin0, Pin1, Pin2>;

    template<uint8_t Pin0, uint8_t Pin1 = 0xFF, uint8_t Pin2 = 0xFF>
    using OnFall = PcIntC<Pin0, Pin1, Pin2>;
  }
#endif

} // hw::avr

// Platform-agnostic alias: chip::OnChange<> resolves to AVR implementation
// and chip::SysTick0<> resolves per chip family
#if !defined(__AVR_ATtiny13__) && !defined(__AVR_ATtiny45__) && !defined(__AVR_ATtiny85__)
namespace chip {
  template<uint8_t Pin0, uint8_t Pin1 = 0xFF, uint8_t Pin2 = 0xFF>
  using OnChange = hw::avr::interrupt_sources::OnChange<Pin0, Pin1, Pin2>;

  template<uint8_t Pin0, uint8_t Pin1 = 0xFF, uint8_t Pin2 = 0xFF>
  using OnRise = hw::avr::interrupt_sources::OnRise<Pin0, Pin1, Pin2>;

  template<uint8_t Pin0, uint8_t Pin1 = 0xFF, uint8_t Pin2 = 0xFF>
  using OnFall = hw::avr::interrupt_sources::OnFall<Pin0, Pin1, Pin2>;
}
#endif

// Chip-family namespace resolution for SysTick0
#ifdef __AVR_ATtiny85__
  namespace chip {
    template<uint32_t CpuHz = 8000000UL> using SysTick0 = hw::avr::tiny85::SysTick0<CpuHz>;
  }
#elif defined(__AVR_ATtiny45__)
  namespace chip {
    template<uint32_t CpuHz = 8000000UL> using SysTick0 = hw::avr::tiny45::SysTick0<CpuHz>;
  }
#elif defined(__AVR_ATtiny13__)
  namespace chip {
    template<uint32_t CpuHz = 9600000UL> using SysTick0 = hw::avr::tiny13::SysTick0<CpuHz>;
  }
#else
  namespace chip {
    template<uint32_t CpuHz = 16000000UL> using SysTick0 = hw::avr::mega::SysTick0<CpuHz>;
    template<uint32_t CpuHz = 16000000UL> using SysTick2 = hw::avr::mega::SysTick2<CpuHz>;
  }
#endif
