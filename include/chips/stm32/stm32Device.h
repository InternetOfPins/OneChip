#pragma once
#include <chips/stm32/stm32SysClock.h>
#if defined(STM32F1xx)
  #include <chips/stm32/stm32F1Port.h>
#else
  #include <chips/stm32/stm32Port.h>
#endif

namespace hw::stm32 {

  namespace detail {
    struct IsSTM32Port {
      template<typename T, typename = void>
      struct Check : std::false_type {};
      template<typename T>
      struct Check<T, std::void_t<typename T::is_stm32_port>> : T::is_stm32_port {};
    };

    template<typename PC>
    using stm32_port_t = decltype(onePin::detail::firstInChain<onePin::detail::IsPort>(
      typename PC::Types::Tail{}));

    template<typename PC>
    static constexpr bool is_stm32_peripheral =
      IsSTM32Port::template Check<stm32_port_t<PC>>::value;
  }

  struct STM32 {
    STM32() = delete;

    template<typename MaskDesc, typename Port>
    using OutPin    = hapi::APIOf<onePin::Stm32OutPin, onePin::Out, oneBit::Mask<MaskDesc>, Port>;

    template<typename MaskDesc, typename Port>
    using InvOutPin = hapi::APIOf<onePin::Stm32OutPin, onePin::Out, oneBit::Inverted<>, oneBit::Mask<MaskDesc>, Port>;

    template<typename MaskDesc, typename Port>
    using InPin  = hapi::APIOf<onePin::Stm32InPin,  onePin::In,  oneBit::Mask<MaskDesc>, Port>;

    template<typename MaskDesc, typename Port>
    using IOPin  = hapi::APIOf<onePin::Stm32IOPin,              oneBit::Mask<MaskDesc>, Port>;

    template<typename Boot, typename... Peripherals>
    struct Board : onePin::Device<Boot, Peripherals...> {
      Board() = delete;
      static_assert((detail::is_stm32_peripheral<Peripherals> && ...),
        "STM32::Board: all peripherals must use STM32 ports");
    };
  };

} // hw::stm32
