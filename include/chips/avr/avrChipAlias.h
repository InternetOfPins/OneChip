#pragma once

// ============================================================
// hw::avr::chip:: — single source of truth for chip-family resolution.
//
// avrPort.h, avrTC.h and avrPcInt.h each used to carry their own
// independent copy of this #if chain, guarded by a shared
// HW_AVR_CHIP_ALIAS_DEFINED macro so only one copy would actually run
// per translation unit — whichever of those three files happened to be
// included first. When one copy fell out of sync with the others (a
// missing ATtiny branch, or a branch aliasing to a namespace that didn't
// exist), the result wasn't a compiler error: the mismatched branch was
// simply never reached, and the TU silently got whichever *other* file's
// (wrong) answer instead. Undefined chip resolution must fail to compile,
// not silently resolve to a different chip's addresses — hence one file,
// included by all three, so there is exactly one #if chain to keep in
// sync and no possibility of a second copy masking a gap in the first.
//
// Add a new chip family here once; every consumer (avrPort.h's PortB,
// avrTC.h's TC0/TC1/.., avrPcInt.h's PcInt0/1/2) sees it immediately.
// ============================================================

namespace hw { namespace avr {
  namespace mega {}
  namespace mega2560 {}
  namespace mega1284 {}
  namespace tiny85 {}
  namespace tiny45 {}
  namespace tiny13 {}
}}

#if defined(__AVR_ATmega640__)  || defined(__AVR_ATmega1280__) || \
    defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || \
    defined(__AVR_ATmega2561__)
  namespace hw { namespace avr { namespace chip = mega2560; }}
#elif defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__)
  namespace hw { namespace avr { namespace chip = mega1284; }}
#elif defined(__AVR_ATtiny85__)
  namespace hw { namespace avr { namespace chip = tiny85; }}
#elif defined(__AVR_ATtiny45__)
  namespace hw { namespace avr { namespace chip = tiny45; }}
#elif defined(__AVR_ATtiny13__)
  namespace hw { namespace avr { namespace chip = tiny13; }}
#else
  namespace hw { namespace avr { namespace chip = mega; }}
#endif
