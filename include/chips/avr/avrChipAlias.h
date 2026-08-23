#pragma once

// ============================================================
// hw::avr::chip:: — single source of truth for chip-family resolution.
//
// avrPort.h, avrTC.h and avrPcInt.h all include this single file rather
// than each carrying its own independent copy of this #if chain: a
// second, independently-maintained copy could fall out of sync (e.g. a
// missing ATtiny branch) without causing a compiler error — the
// mismatched branch would simply go unreached, and the TU would
// silently resolve through whichever other copy's #if chain ran first.
// Undefined chip resolution must fail to compile, not silently resolve
// to a different chip's addresses — hence one file, included by all
// three, so there is exactly one #if chain to keep in sync.
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
#elif defined(__AVR__)
  namespace hw { namespace avr { namespace chip = mega; }}
#endif
