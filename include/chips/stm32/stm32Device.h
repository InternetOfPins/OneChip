#pragma once
#include <onePin/onePin.h>
#include <chips/stm32/stm32Port.h>

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

  template<typename Boot, typename... Peripherals>
  struct STM32 : onePin::Device<Boot, Peripherals...> {
    STM32() = delete;
    static_assert((detail::is_stm32_peripheral<Peripherals> && ...),
      "STM32: all peripherals must use STM32 ports");
  };

} // hw::stm32
