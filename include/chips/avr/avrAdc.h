/**
 * @file avrAdc.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief AVR ADC HAPI component — successive-approximation analog-to-digital converter.
 *        Single fixed peripheral (unlike AVRPort, not parameterized by address/port):
 *        uses avr-libc's own register names directly (ADMUX/ADCSRA/ADC/...), same
 *        convention watchdog.h already uses for other single-instance peripherals.
 */

#pragma once
#include <hapi/hapi.h>

#ifdef __AVR__
  #include <avr/io.h>
  #include <stdint.h>

namespace hw {
namespace avr {

  // Clock prescaler — pick so F_CPU/Div lands in the ADC's 50-200kHz sweet spot
  // (datasheet "ADC Characteristics"): 16MHz->Div128 (125kHz), 8MHz->Div64 (125kHz),
  // 1MHz->Div8 (125kHz). Faster prescalers trade conversion accuracy for speed.
  enum class AdcDiv:uint8_t {Div2=1,Div4=2,Div8=3,Div16=4,Div32=5,Div64=6,Div128=7};

  // AVCC reference, right-adjusted (ADLAR=0), blocking one-shot conversion.
  // read(channel): channel 0-7 select ADC0-7 (ATmega328P); chips with MUX5 (e.g.
  // ATmega2560/1284, channels 8-15) toggle it via ADCSRB when bit 0x10 is set.
  /// @brief successive-approximation ADC; begin() configures it, read(ch) blocks for one sample
  template<AdcDiv Div=AdcDiv::Div128>
  struct AVRAdc {
    template<typename O>
    struct Part : O {
      using Base=O;
      static void begin() {
        ADMUX  = _BV(REFS0);
        ADCSRA = uint8_t(_BV(ADEN) | uint8_t(Div));
        Base::begin();
      }
      static uint16_t read(uint8_t channel) {
        ADMUX = uint8_t((ADMUX & 0xF0) | (channel & 0x0F));
        #if defined(ADCSRB) && defined(MUX5)
          if(channel & 0x10) ADCSRB |= _BV(MUX5);
          else ADCSRB &= uint8_t(~_BV(MUX5));
        #endif
        ADCSRA |= _BV(ADSC);
        while(ADCSRA & _BV(ADSC)) {}
        return ADC;  // avr-libc combines ADCL+ADCH into one 16-bit read
      }
    };
  };

}} // hw::avr
#endif // __AVR__
