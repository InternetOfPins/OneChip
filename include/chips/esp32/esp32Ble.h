#pragma once
#include <stdint.h>
#include <string.h>

// ESP32 BLE GATT server (Arduino core's BLEDevice/BLEServer/BLECharacteristic).
// One service, N characteristics declared at compile time via oneBus::Characteristic<Id,Uuid>.
// Satisfies oneBus::BleAPI: char_write/char_read/char_written/connected.
//
// Device: a named struct with `static constexpr const char* name` and `serviceUuid`
// (see feedback_named_struct_boundary — avoids NTTP string-literal template params).
//
// Usage:
//   struct MyDevice { static constexpr const char* name = "MicroRTC";
//                      static constexpr const char* serviceUuid = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"; };
//   using Ble = hapi::APIOf<oneBus::BleAPI,
//     hw::esp32::Esp32BleCore<MyDevice, oneBus::Characteristic<0, oneBus::Uuid16<0x2A19>>>>;
//   Ble::begin();
//   Ble::char_write(0, data, len);

#ifdef ARDUINO
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <hapi/hapi.h>
#include <oneBus/busAPI.h>
#include <oneBus/uuid.h>

namespace hw::esp32 {

  template<typename UuidTag> struct ToBleUuid;

  // Overlays V into bytes[2..3] of Base (Bluetooth's 16-bit-in-128-bit convention —
  // holds for the SIG base and vendor bases like Nordic UART's alike), so a plain
  // Uuid16<V> (Base defaults to StdBase) still yields the familiar short-form UUID.
  template<uint16_t V, typename Base>
  struct ToBleUuid<oneBus::Uuid16<V, Base>> {
    static BLEUUID get() {
      uint8_t bytes[16];
      memcpy(bytes, Base::bytes, 16);
      bytes[2] = uint8_t(V >> 8);
      bytes[3] = uint8_t(V & 0xFF);
      return BLEUUID(bytes, 16, true);  // bytes are in string/MSB-first order — must say so
    }
  };

  template<uint8_t... B>
  struct ToBleUuid<oneBus::Uuid128<B...>> {
    static BLEUUID get() {
      uint8_t bytes[16] = {B...};
      return BLEUUID(bytes, 16, true);  // bytes are in string/MSB-first order — must say so
    }
  };

  // Marks flags[id] on a peer write; id/flags captured per-instance (not per-type)
  // since BleAPI addresses characteristics by a runtime id.
  class Esp32BleWriteCb : public BLECharacteristicCallbacks {
    uint16_t _id;
    volatile bool* _flags;
  public:
    Esp32BleWriteCb(uint16_t id, volatile bool* flags) : _id(id), _flags(flags) {}
    void onWrite(BLECharacteristic*) override { _flags[_id] = true; }
  };

  class Esp32BleServerCb : public BLEServerCallbacks {
  public:
    static inline volatile bool connected = false;
    void onConnect(BLEServer*) override { connected = true; }
    void onDisconnect(BLEServer* srv) override { connected = false; srv->startAdvertising(); }
  };

  template<typename Device, typename... Chars>
  struct Esp32BleCore {
    template<typename O>
    struct Part : O {
      using Base = O;

    private:
      static constexpr uint16_t N = sizeof...(Chars);
      static inline BLECharacteristic* _table[N]   = {};
      static inline volatile bool      _written[N] = {};

      template<typename C, typename... Rest>
      static void _reg(BLEService* svc) {
        static_assert(C::id < N, "Characteristic id must be < characteristic count");
        auto* ch = svc->createCharacteristic(
          ToBleUuid<typename C::uuid>::get(),
          BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
            | BLECharacteristic::PROPERTY_NOTIFY);
        ch->addDescriptor(new BLE2902());
        ch->setCallbacks(new Esp32BleWriteCb(C::id, _written));
        _table[C::id] = ch;
        if constexpr (sizeof...(Rest) > 0) _reg<Rest...>(svc);
      }

    public:
      static void begin() {
        BLEDevice::init(Device::name);
        BLEServer* srv = BLEDevice::createServer();
        srv->setCallbacks(new Esp32BleServerCb());
        BLEService* svc = srv->createService(Device::serviceUuid);
        if constexpr (N > 0) _reg<Chars...>(svc);
        svc->start();
        BLEAdvertising* adv = BLEDevice::getAdvertising();
        adv->addServiceUUID(Device::serviceUuid);  // else the service UUID never appears in the advertisement
        adv->setScanResponse(true);
        adv->start();
        Base::begin();
      }

      static void char_write(uint16_t id, const uint8_t* data, uint8_t len) {
        _table[id]->setValue(const_cast<uint8_t*>(data), len);
        if (Esp32BleServerCb::connected) _table[id]->notify();
      }
      static uint8_t char_read(uint16_t id, uint8_t* data, uint8_t maxLen) {
        std::string v = _table[id]->getValue();
        uint8_t n = v.size() < maxLen ? (uint8_t)v.size() : maxLen;
        memcpy(data, v.data(), n);
        _written[id] = false;
        return n;
      }
      static bool char_written(uint16_t id) { return _written[id]; }
      static bool connected()               { return Esp32BleServerCb::connected; }
    };
  };

  namespace esp32 {
    template<typename Device, typename... Chars>
    using Ble = hapi::APIOf<oneBus::BleAPI, Esp32BleCore<Device, Chars...>>;
  }

} // hw::esp32
#endif // ARDUINO
