/**
 * @file linuxTwi.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief Native TwiMaster implementations for development and testing.
 *
 *   hw::native::VirtualTwi   — in-memory bus; captures transactions for assertions.
 *                               Available on all platforms (no OS includes).
 *
 *   hw::native::LinuxTwi<N>  — real I2C over /dev/i2c-N via ioctl.
 *                               Linux only; useful on Raspberry Pi or for
 *                               hardware-in-the-loop testing on a dev machine.
 *
 * Both satisfy oneBus::is_twi_master.
 *
 * VirtualTwi example:
 *   using Display = I2cOled<hw::native::VirtualTwi>;
 *   Display::begin();
 *   Display::clear();
 *   assert(hw::native::VirtualTwi::_addr == 0x3C);
 *   assert(hw::native::VirtualTwi::_buf[0] == 0x00);  // command-stream control byte
 *
 * LinuxTwi example (Linux only):
 *   using Display = I2cOled<hw::native::LinuxTwi<1>>;
 *   Display::begin();   // opens /dev/i2c-1
 *   Display::clear();
 */

#pragma once
#include <cstdint>

namespace hw::native {

  // ── VirtualTwi ──────────────────────────────────────────────────────────────
  // Pure in-memory I2C bus. No OS includes.
  // Test code preloads _rxBuf/_rxLen for reads; inspects _addr/_buf/_len after writes.
  struct VirtualTwi {
    inline static uint8_t _addr  = 0;
    inline static uint8_t _buf[256];
    inline static uint8_t _len   = 0;
    inline static uint8_t _rxBuf[256];
    inline static uint8_t _rxLen = 0;
    inline static uint8_t _rxPos = 0;

    static void reset() { _addr = 0; _len = 0; _rxLen = 0; _rxPos = 0; }

    static void    begin()                               {}
    static void    begin_write(uint8_t addr)             { _addr = addr; _len = 0; }
    static void    write_byte(uint8_t b)                 { _buf[_len++] = b; }
    static void    end_write()                           {}
    static uint8_t request_from(uint8_t, uint8_t n)     { _rxPos = 0; return n; }
    static uint8_t read_byte() {
      return _rxPos < _rxLen ? _rxBuf[_rxPos++] : uint8_t(0);
    }
  };

} // hw::native

// ── LinuxTwi — Linux /dev/i2c-N ─────────────────────────────────────────────
#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cstdio>

namespace hw::native {

  // LinuxTwi<N> — I2C master over /dev/i2c-N.
  // Write bytes are buffered and flushed as a single ::write() in end_write().
  // Read bytes are read into _rxBuf in request_from() and consumed by read_byte().
  template<int BusN = 1>
  struct LinuxTwi {
    inline static int     _fd     = -1;
    inline static uint8_t _buf[256];
    inline static int     _bufLen = 0;
    inline static uint8_t _rxBuf[256];
    inline static int     _rxPos  = 0;

    static void begin() {
      char path[16];
      std::snprintf(path, sizeof(path), "/dev/i2c-%d", BusN);
      _fd = open(path, O_RDWR);
    }

    static void begin_write(uint8_t addr) {
      ioctl(_fd, I2C_SLAVE, static_cast<long>(addr));
      _bufLen = 0;
    }

    static void write_byte(uint8_t b) { _buf[_bufLen++] = b; }

    static void end_write() {
      if (_bufLen > 0) ::write(_fd, _buf, _bufLen);
      _bufLen = 0;
    }

    static uint8_t request_from(uint8_t addr, uint8_t n) {
      ioctl(_fd, I2C_SLAVE, static_cast<long>(addr));
      int got = ::read(_fd, _rxBuf, n);
      _rxPos = 0;
      return static_cast<uint8_t>(got > 0 ? got : 0);
    }

    static uint8_t read_byte() {
      return _rxBuf[_rxPos++];
    }
  };

} // hw::native
#endif // __linux__
