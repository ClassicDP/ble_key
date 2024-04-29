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
    void setDevice(BLEAdvertisedDevice *foundDevice);
    std::vector<BLEUUID> getServiceUUIDList();

private:
private:
    BLEScan *bleScan_ = nullptr;
    BLEClient *pClient_ = nullptr;
    std::vector<BLEServiceDescriptor> serviceDescriptors_;
    std::string lastDeviceAddress_;
    BLERemoteService *remoteService_ = nullptr;
    BLERemoteCharacteristic *characteristic1_ = nullptr;
    BLERemoteCharacteristic *characteristic2_ = nullptr;
    static void notifyCallback(BLERemoteCharacteristic *characteristic, uint8_t *data, size_t length, bool isNotify);
    bool shouldReconnect_ = false;
    BLEAdvertisedDevice *pDevice_ = nullptr;
};
