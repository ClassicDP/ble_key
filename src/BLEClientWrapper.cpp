#include "BLEClientWrapper.h"
#include <Arduino.h>

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
            Serial.println("Found device with matching service UUID.");
            bleClient->setFoundDevice(new BLEAdvertisedDevice((advertisedDevice)));
            // Действия после нахождения устройства...
        }
    }
    BLEClientWrapper *bleClient;
};

BLEClientWrapper::BLEClientWrapper(const std::vector<BLEServiceDescriptor> &descriptors)
    : serviceDescriptors_(descriptors)
{
    client_ = nullptr;
    foundDevice_ = nullptr;
    BLEDevice::init("");
    Serial.println("Starting Arduino BLE Client application...");
    bleScan_ = BLEDevice::getScan();
    bleScan_->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(this), false);
    bleScan_->setInterval(1349);
    bleScan_->setWindow(449);
    bleScan_->setActiveScan(true);
    bleScan_->start(1, true);
}

void BLEClientWrapper::findDevice()
{
    // Инициируем сканирование
    startScan();

    // Ждем завершения сканирования
    // Для примера используем delay, хотя в реальном приложении стоит использовать другие механизмы
    delay(1000); 

    // Проверяем, найдено ли устройство
    if (foundDevice_)
    {
        Serial.println("Device found. Connecting...");
        connectToServer();
    }
    else
    {
        Serial.println("No device found.");
    }
}

void BLEClientWrapper::setFoundDevice(BLEAdvertisedDevice *foundDevice)
{
    this->foundDevice_ = foundDevice;
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

BLEScanResults BLEClientWrapper::startScan()
{
    return bleScan_->start(1, false); // Сканирование на 5 секунд
}

bool BLEClientWrapper::connectToServer()
{
    disconnectFromServer();              // Отключаемся от предыдущего сервера, если это необходимо
    client_ = BLEDevice::createClient(); // Создаем новый клиент BLE

    if (!client_->connect(foundDevice_)) // Попытка подключения к устройству
    {
        Serial.println("Failed to connect to server.");
        return false;
    }

    // Перебираем все описания сервисов и подключаемся к характеристикам
    for (const auto &serviceDesc : serviceDescriptors_)
    {
        auto remoteService = client_->getService(serviceDesc.serviceUUID); // Получаем сервис по UUID
        if (remoteService)                                                 // Проверяем, что сервис доступен
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
    }

    lastDeviceAddress_ = foundDevice_->getAddress().toString(); // Сохраняем адрес устройства для возможного переподключения
    return true;
}

void BLEClientWrapper::disconnectFromServer()
{
    if (client_)
    {
        client_->disconnect();
        delete client_;
        client_ = nullptr;
    }
}

void BLEClientWrapper::tryReconnect()
{
    if (!lastDeviceAddress_.empty() && client_)
    {
        BLEAddress addr(lastDeviceAddress_);
        if (client_->connect(addr))
        {
            Serial.println("Reconnected to BLE device.");
            // Тут ваш код для повторной установки сервисов и характеристик
        }
        else
        {
            Serial.println("Failed to reconnect to BLE device.");
        }
    }
}

bool BLEClientWrapper::isConnected() const
{
    return client_ && client_->isConnected();
}

void BLEClientWrapper::taskManager()
{
    while (1)
    {
        if (!isConnected())
        {
            tryReconnect();
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Задержка на 1 секунду
    }
}
