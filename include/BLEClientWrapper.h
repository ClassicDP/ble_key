#pragma once

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string>

// Объявляем тип для функции обратного вызова уведомлений
using NotificationCallback = std::function<void(BLERemoteCharacteristic *, uint8_t *, size_t, bool)>;

// Структура для хранения описания сервиса и его характеристик
struct BLEServiceDescriptor
{
    BLEUUID serviceUUID;
    std::vector<std::pair<BLEUUID, NotificationCallback>> characteristics;
};

class BLEClientWrapper
{
public:
    ~BLEClientWrapper();
    BLEScanResults startScan();
    bool connectToServer();
    void disconnectFromServer();
    void tryReconnect();
    bool isConnected() const;
    void taskManager();
    BLEClientWrapper(const std::vector<BLEServiceDescriptor> &descriptors);
    void setFoundDevice(BLEAdvertisedDevice *foundDevice);
    std::vector<BLEUUID> getServiceUUIDList();

private:
private:
    BLEScan *bleScan_;
    BLEClient *client_;
    std::vector<BLEServiceDescriptor> serviceDescriptors_;
    std::string lastDeviceAddress_;
    BLERemoteService *remoteService_;
    BLERemoteCharacteristic *characteristic1_;
    BLERemoteCharacteristic *characteristic2_;
    static void notifyCallback(BLERemoteCharacteristic *characteristic, uint8_t *data, size_t length, bool isNotify);
    bool shouldReconnect_ = false;
    BLEAdvertisedDevice *foundDevice_;
};
