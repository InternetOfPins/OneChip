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

  // Platform-agnostic alias: chip::OnChange<> (etc.) resolves to the AVR
  // implementation above via the REAL hw::avr::chip namespace alias
  // (avrChipAlias.h — chip = mega/mega2560/mega1284/tiny.../..., the single
  // source of truth for chip-family resolution). Must land inside these
  // concrete per-family namespaces, not a separately-declared `namespace
  // chip { ... }` here: that used to open a SECOND, independent namespace
  // at GLOBAL scope (this whole file sits outside hw::avr from here on,
  // historically) sharing the bare name "chip" with the real
  // hw::avr::chip alias — two distinct entities, same simple name,
  // different scopes. Any unqualified chip::X lookup (chip::PortB,
  // chip::SysTick0, ...) anywhere in a TU that also does `using namespace
  // hw::avr;` became genuinely ambiguous between the two — found on real
  // hardware (Arduino Uno) as "reference to 'chip' is ambiguous" on a
  // completely unrelated chip::PortB use in avrPort.h. Reopening the real
  // per-family namespaces directly (same style avrSysClock.h already uses
  // for SysTick0's own mega2560/mega1284 forwarding, just below) means
  // chip::OnChange resolves through the one real alias, with no second
  // namespace to collide with.
  namespace mega       { using namespace interrupt_sources; }
  namespace mega2560   { using namespace interrupt_sources; }
  namespace mega1284   { using namespace interrupt_sources; }
#endif

} // hw::avr

// chip::SysTick0<>/SysTick2<> need no forwarding here at all — they already
// resolve through the real hw::avr::chip alias (avrChipAlias.h), since
// every per-family namespace (mega/mega2560/mega1284/tiny85/tiny45/tiny13)
// already defines its own SysTick0/SysTick2 directly (avrSysClock.h). A
// duplicate #if cascade re-deriving "which family" here, to forward into a
// second, separately-declared `namespace chip { ... }`, used to sit at this
// exact spot — dead code (the alias already did this job), and the actual
// source of the chip::X ambiguity described above.
