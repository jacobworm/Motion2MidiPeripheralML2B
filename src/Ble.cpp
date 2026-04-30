#include "Ble.h"

namespace {
const char *BLE_DEVICE_NAME = "Thinkerbell";
const char *SERVICE_UUID = "12345678-1234-1234-1234-1234567890ab";
const char *CHAR_UUID = "abcd1234-5678-1234-5678-abcdef123456";

BleCharacteristic gesturesCharacteristic("gestures",
                                         BleCharacteristicProperty::NOTIFY,
                                         CHAR_UUID, SERVICE_UUID);
} // namespace

int initBle(void) {
  BLE.on();
  BLE.setDeviceName(BLE_DEVICE_NAME);
  BLE.addCharacteristic(gesturesCharacteristic);
  BLE.advertise();
  return 0;
}
void sendGestureBluetooth(uint8_t payload) {
  gesturesCharacteristic.setValue(&payload, 1);
}
