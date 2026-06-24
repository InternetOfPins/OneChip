/**
 * @file clock.h
 * @brief Platform-switched millisecond clock — IOP replacement for TinyTimeUtils.
 *
 * Provides four things in namespace hw::
 *   millis()      — wall time in ms (uint32_t, wraps ~49 days)
 *   delay_ms(N)   — blocking N-ms delay
 *   Period<N>     — non-blocking periodic trigger (true every N ms)
 *   Timeout<N>    — one-shot trigger (fires once N ms after construction / reset())
 *
 * Platform dispatch:
 *   __AVR__   — util/delay.h (_delay_ms loop) + ::millis() from framework or SysTick
 *   ARDUINO   — ::millis() / ::delay() from the active Arduino framework
 *   else      — std::chrono / std::this_thread (native / PC)
 *
 * In IOP mode (no Arduino framework), the user provides millis() via their Board
 * SysTick chain. Override hw::millis() before including this header if needed.
 */

#pragma once
#include <stdint.h>

// ── Platform millis() + delay_ms() ───────────────────────────────────────────

#if defined(__AVR__)
  #include <util/delay.h>
  #ifndef IOP
    extern "C" unsigned long millis();
    namespace hw { inline uint32_t millis() { return (uint32_t)::millis(); } }
  #endif
  namespace hw { inline void delay_ms(uint32_t ms) { while (ms--) _delay_ms(1); } }

#elif defined(ARDUINO)
  extern "C" unsigned long millis();
  extern "C" void delay(unsigned long);
  namespace hw {
    inline uint32_t millis()        { return (uint32_t)::millis(); }
    inline void     delay_ms(uint32_t ms) { ::delay(ms); }
  }

#else
  #include <chrono>
  #include <thread>
  namespace hw {
    inline uint32_t millis() {
      return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    inline void delay_ms(uint32_t ms) {
      std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
  }
#endif

// ── hw::Period<N> — non-blocking period ──────────────────────────────────────
// Returns true every N ms. First call returns true immediately.
// when() returns the timestamp of the next expected fire (for idle sleep math).

namespace hw {
  template<uint32_t Ms>
  struct Period {
    uint32_t last = 0;
    bool operator()()   {
      uint32_t now = millis();
      if (now - last < Ms) return false;
      last = now;
      return true;
    }
    explicit operator bool() { return operator()(); }
    void     reset()         { last = millis(); }
    uint32_t when()    const { return last + Ms; }
  };
}

// ── hw::Timeout<N> — one-shot N-ms latch ─────────────────────────────────────
// Constructed armed: fires (returns true) once N ms after construction or reset().
// Stays true after firing until reset().

namespace hw {
  template<uint32_t Ms>
  struct Timeout {
    uint32_t _end;
    bool     _fired;
    Timeout() : _end(millis() + Ms), _fired(false) {}
    void reset() { _end = millis() + Ms; _fired = false; }
    operator bool() { return _fired ? true : (_fired = (millis() >= _end)); }
    uint32_t when() const { return _end; }
  };
}
