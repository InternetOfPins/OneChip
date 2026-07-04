#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// nRF52 BLE GATT server (Adafruit's Bluefruit52Lib — bluefruit.h/BLEService/
// BLECharacteristic). One service, N characteristics declared at compile time via
// oneBus::Characteristic<Id,Uuid>. Satisfies oneBus::BleAPI: char_write/char_read/
// char_written/connected.
//
// Device: a named struct with `static constexpr const char* name` and `serviceUuid`
// (see feedback_named_struct_boundary — avoids NTTP string-literal template params).
//
// Usage:
//   struct MyDevice { static constexpr const char* name = "MicroRTC";
//                      static constexpr const char* serviceUuid = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"; };
//   using Ble = hapi::APIOf<oneBus::BleAPI,
//     hw::nrf52::Nrf52BleCore<MyDevice, oneBus::Characteristic<0, oneBus::Uuid16<0x0002>>>>;
//   Ble::begin();
//   Ble::char_write(0, data, len);

#ifdef ARDUINO
#include <bluefruit.h>
#include <hapi/hapi.h>
#include <oneBus/busAPI.h>
#include <oneBus/uuid.h>

namespace hw::nrf52 {

  template<typename UuidTag> struct ToBleUuid;

  // Bluefruit's BLEUuid(uint8_t[16]) constructor expects Nordic's internal
  // little-endian (reversed-from-string) byte order — see BLEUuid.cpp's own
  // toString() ("uuid is little endian") for confirmation. Rather than hand-reverse
  // (exactly the kind of byte-order mistake that bit the ESP32 core, see
  // project_bt_menu_output memory), format a plain hex string instead and use
  // BLEUuid(const char*), which the library parses itself (parse_str2uuid128) —
  // unambiguous, no reversal to get wrong.
  template<uint16_t V, typename Base>
  struct ToBleUuid<oneBus::Uuid16<V, Base>> {
    static BLEUuid get() {
      static char buf[37];
      snprintf(buf, sizeof buf,
        "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        Base::bytes[0], Base::bytes[1], uint8_t(V >> 8), uint8_t(V & 0xFF),
        Base::bytes[4], Base::bytes[5], Base::bytes[6], Base::bytes[7],
        Base::bytes[8], Base::bytes[9], Base::bytes[10], Base::bytes[11],
        Base::bytes[12], Base::bytes[13], Base::bytes[14], Base::bytes[15]);
      return BLEUuid(buf);
    }
  };

  template<uint8_t... B>
  struct ToBleUuid<oneBus::Uuid128<B...>> {
    static BLEUuid get() {
      static const uint8_t raw[16] = {B...};
      static char buf[37];
      snprintf(buf, sizeof buf,
        "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        raw[0],raw[1],raw[2],raw[3], raw[4],raw[5], raw[6],raw[7],
        raw[8],raw[9], raw[10],raw[11],raw[12],raw[13],raw[14],raw[15]);
      return BLEUuid(buf);
    }
  };

  template<typename Device, typename... Chars>
  struct Nrf52BleCore {
    template<typename O>
    struct Part : O {
      using Base = O;

    private:
      static constexpr uint16_t N = sizeof...(Chars);
      static inline BLECharacteristic* _table[N]   = {};
      static inline volatile bool      _written[N] = {};

      // Bluefruit's write callback has no user-context slot — resolve id by
      // matching the characteristic pointer against the table.
      static void _onWrite(uint16_t, BLECharacteristic* chr, uint8_t*, uint16_t) {
        for (uint16_t i = 0; i < N; i++) if (_table[i] == chr) { _written[i] = true; break; }
      }

      template<typename C, typename... Rest>
      static void _reg() {
        static_assert(C::id < N, "Characteristic id must be < characteristic count");
        auto* ch = new BLECharacteristic(ToBleUuid<typename C::uuid>::get());
        ch->setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_NOTIFY);
        ch->setPermission(SECMODE_OPEN, SECMODE_OPEN);
        ch->setMaxLen(20);
        ch->setWriteCallback(_onWrite);
        ch->begin();
        _table[C::id] = ch;
        if constexpr (sizeof...(Rest) > 0) _reg<Rest...>();
      }

    public:
      static void begin() {
        Bluefruit.begin();
        Bluefruit.setName(Device::name);

        static BLEService svc(BLEUuid(Device::serviceUuid));
        svc.begin();
        if constexpr (N > 0) _reg<Chars...>();

        Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
        Bluefruit.Advertising.addTxPower();
        Bluefruit.Advertising.addService(svc);
        Bluefruit.Advertising.addName();
        Bluefruit.Advertising.restartOnDisconnect(true);
        Bluefruit.Advertising.setInterval(32, 244);
        Bluefruit.Advertising.setFastTimeout(30);
        Bluefruit.Advertising.start(0);
        Base::begin();
      }

      static void char_write(uint16_t id, const uint8_t* data, uint8_t len) {
        _table[id]->write(data, len);
        if (Bluefruit.Periph.connected()) _table[id]->notify(data, len);
      }
      static uint8_t char_read(uint16_t id, uint8_t* data, uint8_t maxLen) {
        uint16_t n = _table[id]->read(data, maxLen);
        _written[id] = false;
        return (uint8_t)n;
      }
      static bool char_written(uint16_t id) { return _written[id]; }
      static bool connected()               { return Bluefruit.Periph.connected() > 0; }
    };
  };

  namespace nrf52 {
    template<typename Device, typename... Chars>
    using Ble = hapi::APIOf<oneBus::BleAPI, Nrf52BleCore<Device, Chars...>>;
  }

} // hw::nrf52
#endif // ARDUINO
