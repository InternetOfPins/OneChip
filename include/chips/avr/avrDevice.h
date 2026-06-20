#pragma once
#include <chips/avr/avrSysClock.h>
#include <chips/avr/avrPort.h>

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

} // hw::avr
