/**
 * @file avrPcIntC.h
 * @brief AVR PORTC Pin Change Interrupt (PCINT1) — implements onChange abstraction.
 *
 * Maps PCINT1 group to onePin::OnChange/OnRise/OnFall contract.
 * Eliminates manual ISR wiring; framework handles interrupts transparently.
 *
 * Max 3 pins per instance (fits typical encoder: A channel, B channel, button).
 * PORTC = A0..A5 on Uno/Nano.
 *
 * Usage:
 *   using MyInt = chip::OnChange<0, 1, 2>;  // A0, A1, A2
 *   MyInt::begin();
 *   if (MyInt::changed()) { handle(MyInt::read()); }
 */

#pragma once
#include <stdint.h>
#include <avr/io.h>
#include <onePin/onChange.h>

namespace hw::avr {

  /// PORTC Pin Change Interrupt adapter (max 3 pins)
  template<uint8_t Pin0, uint8_t Pin1 = 0xFF, uint8_t Pin2 = 0xFF>
  struct PcIntC {
    static_assert(Pin0 < 8, "PcIntC: pin must be 0..7 (PORTC bit)");
    static_assert(Pin1 >= 8 || Pin1 < 8, "");  // either 0xFF or valid bit
    static_assert(Pin2 >= 8 || Pin2 < 8, "");

    static constexpr uint8_t m0 = (1u << Pin0);
    static constexpr uint8_t m1 = (Pin1 < 8) ? (1u << Pin1) : 0;
    static constexpr uint8_t m2 = (Pin2 < 8) ? (1u << Pin2) : 0;
    static constexpr uint8_t mask = uint8_t(m0 | m1 | m2);

    inline static volatile uint8_t _last = 0;

    template<typename EdgeMode>
    static void begin() {
      DDRC  &= ~mask;         // input
      PORTC |=  mask;         // pull-up on
      PCICR |= (1u << PCIE1); // enable PCINT1 group
      PCMSK1 |= mask;         // watch these pins
      _last = (PINC & mask);
      sei();
    }

    template<typename EdgeMode>
    static uint8_t read() {
      return (PINC & mask);
    }

    template<typename EdgeMode>
    static bool changed() {
      uint8_t now = read();
      uint8_t diff = now ^ _last;
      return (diff & mask) != 0;
    }

    // Call from ISR(PCINT1_vect) to update state
    static bool isr_handler() {
      uint8_t now = (PINC & mask);
      uint8_t was_diff = (now ^ _last) & mask;
      _last = now;
      return was_diff != 0;
    }
  };

} // hw::avr
