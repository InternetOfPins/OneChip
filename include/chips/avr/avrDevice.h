#pragma once
#include <chips/avr/avrSysClock.h>
#include <chips/avr/avrPort.h>
#include <chips/avr/avrUart.h>
#include <chips/avr/avrTwi.h>
#include <chips/avr/avrSpi.h>
#include <chips/avr/avrPcIntC.h>

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
  // Maps chip::OnChange<> to platform-specific PcIntC implementation
  namespace interrupt_sources {
    template<uint8_t Pin0, uint8_t Pin1 = 0xFF, uint8_t Pin2 = 0xFF>
    using OnChange = PcIntC<Pin0, Pin1, Pin2>;

    template<uint8_t Pin0, uint8_t Pin1 = 0xFF, uint8_t Pin2 = 0xFF>
    using OnRise = PcIntC<Pin0, Pin1, Pin2>;

    template<uint8_t Pin0, uint8_t Pin1 = 0xFF, uint8_t Pin2 = 0xFF>
    using OnFall = PcIntC<Pin0, Pin1, Pin2>;
  }

} // hw::avr

// Platform-agnostic alias: chip::OnChange<> resolves to AVR implementation
namespace chip {
  template<uint8_t Pin0, uint8_t Pin1 = 0xFF, uint8_t Pin2 = 0xFF>
  using OnChange = hw::avr::interrupt_sources::OnChange<Pin0, Pin1, Pin2>;

  template<uint8_t Pin0, uint8_t Pin1 = 0xFF, uint8_t Pin2 = 0xFF>
  using OnRise = hw::avr::interrupt_sources::OnRise<Pin0, Pin1, Pin2>;

  template<uint8_t Pin0, uint8_t Pin1 = 0xFF, uint8_t Pin2 = 0xFF>
  using OnFall = hw::avr::interrupt_sources::OnFall<Pin0, Pin1, Pin2>;
}
