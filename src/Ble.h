#ifndef BLE_H
#define BLE_H

#include "particle.h"

int initBle(void);
void sendGestureBluetooth(uint8_t payload);

#endif