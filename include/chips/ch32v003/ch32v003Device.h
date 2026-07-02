#pragma once
#include <chips/ch32v003/ch32v003SysClock.h>
#include <chips/ch32v003/ch32v003Port.h>

extern "C" {
  #include "ch32v00x.h"
}

namespace hw::ch32v003 {

  namespace detail {
    struct IsCh32v003Port {
      template<typename T, typename = void>
      struct Check : std::false_type {};
      template<typename T>
      struct Check<T, std::void_t<typename T::is_ch32v003_port>> : T::is_ch32v003_port {};
    };

    template<typename PC>
    using ch32v003_port_t = decltype(onePin::detail::firstInChain<onePin::detail::IsPort>(
      typename PC::Types::Tail{}));

    template<typename PC>
    static constexpr bool is_ch32v003_peripheral =
      IsCh32v003Port::template Check<ch32v003_port_t<PC>>::value;
  }

  /// @brief CH32V003 chip descriptor: port aliases + Board wrapper (SystemCoreClockUpdate on begin)
  struct Ch32v003 {
    Ch32v003() = delete;

    template<typename MaskDesc, typename Port>
    using OutPin = hapi::APIOf<onePin::OutPin<Unit>, onePin::Out, oneBit::Mask<MaskDesc>, Port>;

    template<typename MaskDesc, typename Port>
    using InPin  = hapi::APIOf<onePin::InPin<Unit>,  onePin::In,  oneBit::Mask<MaskDesc>, Port>;

    template<typename MaskDesc, typename Port>
    using IOPin  = hapi::APIOf<onePin::IOPin<Unit>,               oneBit::Mask<MaskDesc>, Port>;

    template<typename Boot, typename... Peripherals>
    struct Board : onePin::Device<Boot, Peripherals...> {
      Board() = delete;
      static_assert((detail::is_ch32v003_peripheral<Peripherals> && ...),
        "Ch32v003::Board: all peripherals must use Ch32v003Port");

      static void begin() {
        SystemCoreClockUpdate();
        onePin::Device<Boot, Peripherals...>::begin();
      }
    };
  };

} // hw::ch32v003
