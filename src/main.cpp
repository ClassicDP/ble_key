#include <Arduino.h>
#include "BLEClientWrapper.h"

// Определения UUID
#define LOCK_SERVICE_UUID        "550e8400-e29b-41d4-a716-446655440000"
#define LOCK_STATUS_UUID         "e7e478d3-8d98-40c8-8f8f-d1c94e441b21"
#define LOCK_CONTROL_UUID        "1b3e8c6a-f7d5-49c9-b2a0-50e5722dbb6f"
#define LOCK_NOTIFICATION_UUID   "3c29d97c-92a1-4a64-bc7c-f58bcaaf9af4"

BLEClientWrapper* bleClientWrapper;

void setup() {
    Serial.begin(115200);
    std::vector<BLEServiceDescriptor> serviceDescriptors = {
        {BLEUUID(LOCK_SERVICE_UUID), {
            {BLEUUID(LOCK_STATUS_UUID), [](BLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify) {
                Serial.print("Lock Status Notification: ");
                for (size_t i = 0; i < length; i++) {
                    Serial.print((char)data[i]);
                }
                Serial.println();
            }},
            {BLEUUID(LOCK_CONTROL_UUID), [](BLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify) {
                Serial.print("Lock Control Notification: ");
                for (size_t i = 0; i < length; i++) {
                    Serial.print((char)data[i]);
                }
                Serial.println();
            }},
            {BLEUUID(LOCK_NOTIFICATION_UUID), [](BLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify) {
                Serial.print("Lock Notification: ");
                for (size_t i = 0; i < length; i++) {
                    Serial.print((char)data[i]);
                }
                Serial.println();
            }}
        }}
    };

    bleClientWrapper = new BLEClientWrapper(serviceDescriptors);
    bleClientWrapper->startScan();
    bleClientWrapper->taskManager();

    // Запускаем сканирование BLE-устройств
 


}

void loop() {
    // Основной цикл loop остается пустым, так как всё управление перенесено в коллбеки BLEClientWrapper.
}
