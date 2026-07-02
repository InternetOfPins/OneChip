#pragma once
#include <chips/stm32/stm32SysClock.h>
#include <chips/stm32/stm32Uart.h>
#include <chips/stm32/stm32Twi.h>
#include <chips/stm32/stm32Spi.h>
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

    template<typename T, typename = void>
    struct has_millis_fn : std::false_type {};
    template<typename T>
    struct has_millis_fn<T, std::void_t<decltype(T::millis())>> : std::true_type {};

    template<typename... Ts>
    static constexpr bool any_has_clock = (has_millis_fn<Ts>::value || ...);
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
    struct Board;

    // Auto-inject chip::SysClk<> when Boot has no clock component. (Named SysClk, not
    // SysTick — collides with ARM CMSIS's #define SysTick, see stm32SysClock.h.)
    template<typename... BootItems, typename... Peripherals>
    struct Board<onePin::Boot<BootItems...>, Peripherals...>
      : std::conditional_t<
          detail::any_has_clock<BootItems...>,
          onePin::Device<onePin::Boot<BootItems...>,               Peripherals...>,
          onePin::Device<onePin::Boot<chip::SysClk<>, BootItems...>, Peripherals...>
        >
    {
      Board() = delete;
      static_assert((detail::is_stm32_peripheral<Peripherals> && ...),
        "STM32::Board: all peripherals must use STM32 ports");
    };
  };

} // hw::stm32
