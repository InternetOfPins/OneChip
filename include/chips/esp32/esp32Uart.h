/**
 * @file esp32Uart.h
 * @brief ESP32 UART hardware core — register-level Base:: primitives for oneBus::Uart<>.
 *
 * #ifdef ARDUINO: wraps one of ESP32's 3 built-in HardwareSerial ports (Serial/Serial1/
 * Serial2) — framework=arduino is this project's primary ESP32 target.
 * otherwise: ESP-IDF uart driver, same shape.
 *
 * N selects which hardware UART (0/1/2). RxPin/TxPin: GPIO numbers; -1 (Arduino only)
 * means "use that port's default pins" — ESP-IDF's uart_set_pin() has no such default,
 * so the ESP-IDF branch requires real pin numbers.
 */

#pragma once
#include <stdint.h>

#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <driver/uart.h>
#endif

#include <hapi/hapi.h>
#include <oneBus/uart.h>

namespace hw::esp32 {

  template<int N = 0, int RxPin = -1, int TxPin = -1>
  struct Esp32UsartCore {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

#ifdef ARDUINO
      static HardwareSerial& port() {
        if constexpr (N == 0) return Serial;
        else if constexpr (N == 1) return Serial1;
        else return Serial2;
      }

      static bool available() { return port().available() > 0; }
      static void putch(uint8_t c) { port().write(c); }
      static uint8_t getch() { return (uint8_t)port().read(); }
      static void uart_init(uint32_t baud) { port().begin(baud, SERIAL_8N1, RxPin, TxPin); }
#else
      static constexpr uart_port_t uart_num = (uart_port_t)N;

      static bool available() {
        size_t n = 0;
        uart_get_buffered_data_len(uart_num, &n);
        return n > 0;
      }
      static void putch(uint8_t c) { uart_write_bytes(uart_num, (const char*)&c, 1); }
      static uint8_t getch() {
        uint8_t c = 0;
        uart_read_bytes(uart_num, &c, 1, portMAX_DELAY);
        return c;
      }
      static void uart_init(uint32_t baud) {
        uart_config_t cfg = {};
        cfg.baud_rate = (int)baud;
        cfg.data_bits = UART_DATA_8_BITS;
        cfg.parity    = UART_PARITY_DISABLE;
        cfg.stop_bits = UART_STOP_BITS_1;
        cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
        uart_param_config(uart_num, &cfg);
        uart_set_pin(uart_num, TxPin, RxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        uart_driver_install(uart_num, 256, 0, 0, nullptr, 0);
      }
#endif

      static void begin() { Base::begin(); }
    };
  };

  // ============================================================
  // Board-specific namespace aliases for oneBus UART API
  // Nested in namespace esp32 to match esp32Device.h's convention
  // ============================================================
  namespace esp32 {

    template<uint32_t BaudRate>
    using Serial0 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>, Esp32UsartCore<0>>;
    template<uint32_t BaudRate>
    using Serial1 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>, Esp32UsartCore<1>>;
    template<uint32_t BaudRate>
    using Serial2 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>, Esp32UsartCore<2>>;

  } // hw::esp32::esp32

} // hw::esp32
