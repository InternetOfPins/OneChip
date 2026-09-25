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
#include <oneBus/busAPI.h>

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
    // presence: every address answers unless absent() says otherwise
    inline static bool _absent[128] = {};
    inline static bool _ack = true;
    inline static oneBus::TwiCause _cause = oneBus::TwiCause::None;

    static void reset() {
      _addr = 0; _len = 0; _rxLen = 0; _rxPos = 0; _ack = true; _cause = oneBus::TwiCause::None;
      for (auto& a : _absent) a = false;
    }
    static void absent(uint8_t addr, bool v = true) { _absent[addr & 0x7F] = v; }

    static void    begin()                               {}
    static bool    begin_write(uint8_t addr) {
      _addr = addr; _len = 0; _ack = !_absent[addr & 0x7F];
      _cause = _ack ? oneBus::TwiCause::None : oneBus::TwiCause::Nack;
      return _ack;
    }
    static void    write_byte(uint8_t b)                 { if (_ack) _buf[_len++] = b; }
    static bool    end_write()                           { return _ack; }
    [[nodiscard]] static uint8_t request_from(uint8_t addr, uint8_t n) {
      _rxPos = 0;
      if (_absent[addr & 0x7F]) { _cause = oneBus::TwiCause::Nack; return 0; }
      _cause = oneBus::TwiCause::None;
      return n;
    }
    [[nodiscard]] static uint8_t read_byte() {
      return _rxPos < _rxLen ? _rxBuf[_rxPos++] : uint8_t(0);
    }
    static oneBus::TwiCause cause() { return _cause; }
  };

} // hw::native

// ── LinuxTwi — Linux /dev/i2c-N ─────────────────────────────────────────────
#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <cerrno>
#include <cstdio>

namespace hw::native {

  // LinuxTwi<N> — I2C master over /dev/i2c-N.
  // Write bytes are buffered and flushed as a single ::write() in end_write(); a write of no bytes is an SMBus
  // quick write (what i2cdetect does), so probing needs no register write.
  // Read bytes are read into _rxBuf in request_from() and consumed by read_byte().
  // errno -> cause, from the kernel's i2c fault-codes: ENXIO address not acknowledged, ETIMEDOUT, EAGAIN lost
  // arbitration, EBUSY bus busy; anything else is Unknown.
  template<int BusN = 1>
  struct LinuxTwi {
    inline static int     _fd     = -1;
    inline static uint8_t _buf[256];
    inline static int     _bufLen = 0;
    inline static uint8_t _rxBuf[256];
    inline static int     _rxPos  = 0;
    inline static bool    _ok     = false;
    inline static oneBus::TwiCause _cause = oneBus::TwiCause::None;

    static constexpr oneBus::TwiCause causeFromErrno(int e) {
      return e == ENXIO ? oneBus::TwiCause::Nack : e == ETIMEDOUT ? oneBus::TwiCause::Timeout
           : e == EAGAIN ? oneBus::TwiCause::ArbLost : e == EBUSY ? oneBus::TwiCause::BusError
           : oneBus::TwiCause::Unknown;
    }

    static void begin() {
      char path[16];
      std::snprintf(path, sizeof(path), "/dev/i2c-%d", BusN);
      _fd = open(path, O_RDWR);
    }

    static bool select(uint8_t addr) {
      _cause = oneBus::TwiCause::None;
      if (ioctl(_fd, I2C_SLAVE, static_cast<long>(addr)) < 0) { _cause = causeFromErrno(errno); return false; }
      return true;
    }

    static bool begin_write(uint8_t addr) {
      _bufLen = 0;
      _ok = select(addr);
      return _ok;                        // the address itself is only put on the wire when the transaction runs
    }

    static void write_byte(uint8_t b) { if (_bufLen < int(sizeof _buf)) _buf[_bufLen++] = b; }

    static bool end_write() {
      if (_ok) {
        if (_bufLen > 0) {
          const ssize_t w = ::write(_fd, _buf, _bufLen);
          if (w != _bufLen) { _ok = false; _cause = w < 0 ? causeFromErrno(errno) : oneBus::TwiCause::Unknown; }
        } else {
          i2c_smbus_ioctl_data q{};
          q.read_write = I2C_SMBUS_WRITE; q.command = 0; q.size = I2C_SMBUS_QUICK; q.data = nullptr;
          if (ioctl(_fd, I2C_SMBUS, &q) < 0) { _ok = false; _cause = causeFromErrno(errno); }
        }
      }
      _bufLen = 0;
      return _ok;
    }

    [[nodiscard]] static uint8_t request_from(uint8_t addr, uint8_t n) {
      _rxPos = 0;
      if (!select(addr)) return 0;
      const int got = ::read(_fd, _rxBuf, n);
      if (got <= 0) { _cause = got < 0 ? causeFromErrno(errno) : oneBus::TwiCause::Unknown; return 0; }
      return static_cast<uint8_t>(got);
    }

    [[nodiscard]] static uint8_t read_byte() {
      return _rxBuf[_rxPos++];
    }

    static oneBus::TwiCause cause() { return _cause; }

  };

} // hw::native
#endif // __linux__
