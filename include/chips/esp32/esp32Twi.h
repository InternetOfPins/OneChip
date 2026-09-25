#pragma once
#include <stdint.h>
#include <oneBus/busAPI.h>
#include <oneBus/twiMaster.h>

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
    inline static oneBus::TwiCause _cause = oneBus::TwiCause::None;

    static void begin() { Wire.begin(SDA, SCL, SclHz); }

    // Wire buffers the transaction: the address is only put on the wire, and acknowledged, in end_write().
    // endTransmission(): 0 ok, 1 too long, 2 NACK (ESP_FAIL: address or data, the core cannot tell), 4 other, 5 timeout.
    static bool begin_write(uint8_t addr) { Wire.beginTransmission(addr); return true; }
    static void write_byte(uint8_t b)     { Wire.write(b); }
    static bool end_write() {
      const uint8_t e = Wire.endTransmission();
      _cause = oneBus::twiCauseFromWire(e);
      return e == 0;
    }

    static bool send(uint8_t addr, uint8_t data) {
      Wire.beginTransmission(addr); Wire.write(data); return end_write();
    }
    static bool send(uint8_t addr, const uint8_t* data, uint8_t len) {
      Wire.beginTransmission(addr);
      while (len--) Wire.write(*data++);
      return end_write();
    }

    // ── Read streaming ─────────────────────────────────────────────────
    // requestFrom() returns 0 on any failure and does not say why (log_e only).
    [[nodiscard]] static uint8_t request_from(uint8_t addr, uint8_t n) {
      const uint8_t got = Wire.requestFrom(static_cast<uint8_t>(addr), static_cast<uint8_t>(n));
      _cause = got ? oneBus::TwiCause::None : oneBus::TwiCause::Unknown;
      return got;
    }

    [[nodiscard]] static uint8_t read_byte() { return Wire.read(); }

    [[nodiscard]] static bool available() { return Wire.available() > 0; }

    static oneBus::TwiCause cause() { return _cause; }

    // Presence. A read whose address byte is not acknowledged takes the legacy driver at least a second to give up,
    // whatever timeout it is given (measured, Arduino-ESP32 2.0.6), so a read-probe would cost 24 s per scan: unless
    // ONEBUS_ESP32_READ_PROBE is defined, both kinds are the write-probe. With it, the read-probe goes through the HAL's
    // i2cRead, and that timeout, with both lines idle, is "nobody there"; with a line held low it is the bus.
    static bool busIdle() { return digitalRead(SDA) && digitalRead(SCL); }
    static bool probe(uint8_t addr, oneBus::ProbeKind kind) {
#ifndef ONEBUS_ESP32_READ_PROBE
      (void)kind;
      Wire.beginTransmission(addr);
      return end_write();
#else
      if (kind == oneBus::ProbeKind::Write) {
        Wire.beginTransmission(addr);
        return end_write();
      }
      uint8_t b; size_t got = 0;
      const esp_err_t e = i2cRead(0, addr, &b, 1, Wire.getTimeOut(), &got);
      _cause = e == ESP_OK ? oneBus::TwiCause::None : e == ESP_FAIL ? oneBus::TwiCause::Nack
             : e == ESP_ERR_TIMEOUT ? (busIdle() ? oneBus::TwiCause::Nack : oneBus::TwiCause::Timeout)
             : oneBus::TwiCause::Unknown;
      return e == ESP_OK;
#endif
    }
  };

#else // ESP-IDF

  template<int SDA = 21, int SCL = 22, uint32_t SclHz = 400000UL,
           i2c_port_t Port = I2C_NUM_0>
  struct Esp32TwiMaster {
    inline static oneBus::TwiCause _cause = oneBus::TwiCause::None;

  private:
    inline static i2c_cmd_handle_t _cmd = nullptr;

    static void _start(uint8_t addr) {
      _cmd = i2c_cmd_link_create();
      i2c_master_start(_cmd);
      i2c_master_write_byte(_cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    }
    // the command link runs here: ESP_OK, ESP_FAIL (a byte was not acknowledged), ESP_ERR_TIMEOUT (bus busy)
    static bool _stop() {
      i2c_master_stop(_cmd);
      const esp_err_t e = i2c_master_cmd_begin(Port, _cmd, 10 / portTICK_PERIOD_MS);
      i2c_cmd_link_delete(_cmd);
      _cmd = nullptr;
      _cause = e == ESP_OK ? oneBus::TwiCause::None : e == ESP_FAIL ? oneBus::TwiCause::Nack
             : e == ESP_ERR_TIMEOUT ? oneBus::TwiCause::Timeout : oneBus::TwiCause::Unknown;
      return e == ESP_OK;
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

    // the command link is only executed in end_write(), where the acknowledgement is known
    static bool begin_write(uint8_t addr) { _start(addr); return true; }
    static void write_byte(uint8_t b)     { i2c_master_write_byte(_cmd, b, true); }
    static bool end_write()               { return _stop(); }

    static bool send(uint8_t addr, uint8_t data) {
      _start(addr); i2c_master_write_byte(_cmd, data, true); return _stop();
    }
    static bool send(uint8_t addr, const uint8_t* data, uint8_t len) {
      _start(addr); i2c_master_write(_cmd, data, len, true); return _stop();
    }

    static oneBus::TwiCause cause() { return _cause; }

    // Presence. As above: a NACKed read costs this driver a second, so both kinds are the write-probe unless
    // ONEBUS_ESP32_READ_PROBE is defined. This path has no read stream, so its read-probe queues SLA+R and one byte
    // read with NACK; a timeout with both lines idle is "nobody there".
    static bool busIdle() { return gpio_get_level(static_cast<gpio_num_t>(SDA)) && gpio_get_level(static_cast<gpio_num_t>(SCL)); }
    static bool probe(uint8_t addr, oneBus::ProbeKind kind) {
#ifndef ONEBUS_ESP32_READ_PROBE
      (void)kind;
      _start(addr); return _stop();
#else
      if (kind == oneBus::ProbeKind::Write) { _start(addr); return _stop(); }
      _cmd = i2c_cmd_link_create();
      i2c_master_start(_cmd);
      i2c_master_write_byte(_cmd, (addr << 1) | I2C_MASTER_READ, true);
      uint8_t b;
      i2c_master_read_byte(_cmd, &b, I2C_MASTER_NACK);
      const bool ok = _stop();
      if (!ok && _cause == oneBus::TwiCause::Timeout && busIdle()) _cause = oneBus::TwiCause::Nack;
      return ok;
#endif
    }
  };

#endif // ARDUINO

  namespace esp32 {
    template<int SDA = 21, int SCL = 22, uint32_t SclHz = 400000UL>
    using TwiMaster = Esp32TwiMaster<SDA, SCL, SclHz>;
  }

} // hw::esp32
