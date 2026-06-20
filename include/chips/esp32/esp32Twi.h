#pragma once
#include <stdint.h>

// ESP32 I2C master.
// #ifdef ARDUINO: wraps Arduino Wire (TwoWire).
// otherwise: uses ESP-IDF legacy i2c command-link API.
//
// SDA / SCL: GPIO numbers (default = ESP32 standard I2C pins 21/22).
// SclHz: bus clock, default 400 kHz (ESP32 can go to 800 kHz).

#ifdef ARDUINO
  #include <Wire.h>
#else
  #include <driver/i2c.h>
#endif

namespace hw::esp32 {

#ifdef ARDUINO

  template<int SDA = 21, int SCL = 22, uint32_t SclHz = 400000UL>
  struct Esp32TwiMaster {
    static void begin() { Wire.begin(SDA, SCL, SclHz); }

    static void begin_write(uint8_t addr) { Wire.beginTransmission(addr); }
    static void write_byte(uint8_t b)     { Wire.write(b); }
    static void end_write()               { Wire.endTransmission(); }

    static void send(uint8_t addr, uint8_t data) {
      Wire.beginTransmission(addr); Wire.write(data); Wire.endTransmission();
    }
    static void send(uint8_t addr, const uint8_t* data, uint8_t len) {
      Wire.beginTransmission(addr);
      while (len--) Wire.write(*data++);
      Wire.endTransmission();
    }
  };

#else // ESP-IDF

  template<int SDA = 21, int SCL = 22, uint32_t SclHz = 400000UL,
           i2c_port_t Port = I2C_NUM_0>
  struct Esp32TwiMaster {
  private:
    inline static i2c_cmd_handle_t _cmd = nullptr;

    static void _start(uint8_t addr) {
      _cmd = i2c_cmd_link_create();
      i2c_master_start(_cmd);
      i2c_master_write_byte(_cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    }
    static void _stop() {
      i2c_master_stop(_cmd);
      i2c_master_cmd_begin(Port, _cmd, 10 / portTICK_PERIOD_MS);
      i2c_cmd_link_delete(_cmd);
      _cmd = nullptr;
    }

  public:
    static void begin() {
      i2c_config_t cfg{};
      cfg.mode             = I2C_MODE_MASTER;
      cfg.sda_io_num       = SDA;
      cfg.scl_io_num       = SCL;
      cfg.sda_pullup_en    = GPIO_PULLUP_ENABLE;
      cfg.scl_pullup_en    = GPIO_PULLUP_ENABLE;
      cfg.master.clk_speed = SclHz;
      i2c_param_config(Port, &cfg);
      i2c_driver_install(Port, I2C_MODE_MASTER, 0, 0, 0);
    }

    static void begin_write(uint8_t addr) { _start(addr); }
    static void write_byte(uint8_t b)     { i2c_master_write_byte(_cmd, b, true); }
    static void end_write()               { _stop(); }

    static void send(uint8_t addr, uint8_t data) {
      _start(addr); i2c_master_write_byte(_cmd, data, true); _stop();
    }
    static void send(uint8_t addr, const uint8_t* data, uint8_t len) {
      _start(addr); i2c_master_write(_cmd, data, len, true); _stop();
    }
  };

#endif // ARDUINO

  namespace esp32 {
    template<int SDA = 21, int SCL = 22, uint32_t SclHz = 400000UL>
    using TwiMaster = Esp32TwiMaster<SDA, SCL, SclHz>;
  }

} // hw::esp32
