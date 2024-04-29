#include "BLEClientWrapper.h"
#include <Arduino.h>
#include <type_traits>

template <typename T>
void freePtr(T *&ptr)
{
    Serial.print("deleting: ");
    Serial.println(typeid(T).name());
    if (ptr!= nullptr)
    {
        delete ptr;
        ptr = nullptr;
    }
}
// Коллбек для обработки найденных устройств
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
public:
    MyAdvertisedDeviceCallbacks(BLEClientWrapper *bleClient) : bleClient(bleClient) {}
    void onResult(BLEAdvertisedDevice advertisedDevice) override
    {
        Serial.print("Device found: ");
        Serial.println(advertisedDevice.toString().c_str());
        auto serviceUUIDList = bleClient->getServiceUUIDList();
        // Используем алгоритм std::find_if для поиска хотя бы одного совпадающего UUID сервиса
        auto found = std::find_if(
            serviceUUIDList.begin(), serviceUUIDList.end(),
            [&advertisedDevice](const BLEUUID &uuid)
            {
                return advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(uuid);
            });

        if (found != serviceUUIDList.end())
        {
            BLEDevice::getScan()->stop(); // Останавливаем сканирование
            // Не останавливаем сканирование, чтобы продолжить обнаружение других устройств
            int rssi = advertisedDevice.getRSSI();
            std::string rssi_str = std::to_string(rssi); // Конвертируем RSSI в строку
            Serial.println(("Found device with matching service UUID. RSSI: " + rssi_str).c_str());
            bleClient->setDevice(new BLEAdvertisedDevice((advertisedDevice)));
            // Действия после нахождения устройства...
        }
    }
    BLEClientWrapper *bleClient;
};

BLEClientWrapper::BLEClientWrapper(const std::vector<BLEServiceDescriptor> &descriptors)
    : serviceDescriptors_(descriptors)
{
    pClient_ = nullptr;
    pDevice_ = nullptr;
    BLEDevice::init("");
    Serial.println("Starting Arduino BLE Client application...");
    bleScan_ = BLEDevice::getScan();
    bleScan_->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(this), false);
    bleScan_->setInterval(1349);
    bleScan_->setWindow(449);
    bleScan_->setActiveScan(true);
    bleScan_->start(1, true);
}

BLEScanResults BLEClientWrapper::startScan()
{
    BLEScanResults results;
    do
    {
        results = bleScan_->start(1, false); // Сканирование
        if (results.getCount() == 0)
        {
            Serial.println("No devices found. Restarting scan...");
        }
    } while (results.getCount() == 0 || !connectToServer());
    

    return results;
}

void BLEClientWrapper::setDevice(BLEAdvertisedDevice *device)
{
    pDevice_ = device;
}

std::vector<BLEUUID> BLEClientWrapper::getServiceUUIDList()
{
    std::vector<BLEUUID> list;
    for (auto &d : serviceDescriptors_)
    {
        list.push_back(d.serviceUUID);
    }
    return list;
}

BLEClientWrapper::~BLEClientWrapper()
{
    disconnectFromServer();
}

bool BLEClientWrapper::connectToServer()
{
    Serial.println("Connecting to server...");
    disconnectFromServer(); // Отключаемся от предыдущего сервера, если это необходимо
    free(pClient_);
    pClient_ = BLEDevice::createClient(); // Создаем новый клиент BLE

    while (!pDevice_)
    {
        Serial.println("Waiting for device");
        return false;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    Serial.println("Connected");

    if (!pClient_->connect(pDevice_)) // Попытка подключения к устройству
    {
        Serial.println("Failed to connect to server.");
        return false;
    }

    Serial.println("Subscribing to services...");
    Serial.println(std::to_string(pClient_->getRssi()).c_str());

    // Перебираем все описания сервисов и подключаемся к характеристикам
    for (const auto &serviceDesc : serviceDescriptors_)
    {
        auto remoteService = pClient_->getService(serviceDesc.serviceUUID); // Получаем сервис по UUID
        if (remoteService)                                                  // Проверяем, что сервис доступен
        {
            for (const auto &charDesc : serviceDesc.characteristics)
            {
                auto characteristic = remoteService->getCharacteristic(charDesc.first); // Получаем характеристику по UUID
                if (characteristic)
                {
                    if (characteristic->canNotify()) // Проверяем, может ли характеристика отправлять уведомления
                    {
                        characteristic->registerForNotify(charDesc.second); // Регистрируем коллбек для уведомлений
                    }
                    if (characteristic->canIndicate()) // Для характеристик, поддерживающих индикации
                    {
                        characteristic->registerForNotify(charDesc.second);
                    }
                }
                else
                {
                    BLEUUID tempUuid = charDesc.first;
                    Serial.println("Characteristic not found: " + String(tempUuid.toString().c_str()));
                }
            }
        }
        else
        {
            BLEUUID tempUuid = serviceDesc.serviceUUID;
            Serial.println("Service not found: " + String(tempUuid.toString().c_str()));
        }
        Serial.println("Reconnection complete");
    }

    lastDeviceAddress_ = pDevice_->getAddress().toString(); // Сохраняем адрес устройства для возможного переподключения

    return true;
}

void BLEClientWrapper::disconnectFromServer()
{
    if (pClient_)
    {
        pClient_->disconnect();
        delete pClient_;
        pClient_ = nullptr;
    }
}

void BLEClientWrapper::tryReconnect()
{
    free(pDevice_);
    startScan();
    // if (!lastDeviceAddress_.empty() && client_)
    // {
    //     BLEAddress addr(lastDeviceAddress_);
    //     if (client_->connect(addr))
    //     {
    //         Serial.println("Reconnected to BLE device.");
    //         // Тут ваш код для повторной установки сервисов и характеристик
    //     }
    //     else
    //     {
    //         Serial.println("Failed to reconnect to BLE device.");
    //     }
    // }
}

bool BLEClientWrapper::isConnected() const
{
    return pClient_ && pClient_->isConnected();
}

void BLEClientWrapper::taskManager()
{
    while (1)
    {
        if (!isConnected())
        {
            tryReconnect();
        }
        Serial.println(std::to_string(pClient_->getRssi()).c_str());
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
