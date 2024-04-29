#include <Arduino.h>
#include "BLEClientWrapper.h"

// Определения UUID
#define LOCK_SERVICE_UUID "550e8400-e29b-41d4-a716-446655440000"
#define LOCK_STATUS_UUID "e7e478d3-8d98-40c8-8f8f-d1c94e441b21"
#define LOCK_CONTROL_UUID "1b3e8c6a-f7d5-49c9-b2a0-50e5722dbb6f"
#define LOCK_NOTIFICATION_UUID "3c29d97c-92a1-4a64-bc7c-f58bcaaf9af4"

BLEClientWrapper *bleClientWrapper;
#include "esp_heap_caps.h"

void logHeapStats()
{
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_8BIT);

    Serial.print("Total free bytes: ");
    Serial.println(info.total_free_bytes);
    Serial.print("Total allocated bytes: ");
    Serial.println(info.total_allocated_bytes);
    Serial.print("Largest free block: ");
    Serial.println(info.largest_free_block);
    Serial.print("Minimum free bytes: ");
    Serial.println(info.minimum_free_bytes);
    Serial.print("Allocated blocks: ");
    Serial.println(info.allocated_blocks);
    Serial.print("Free blocks: ");
    Serial.println(info.free_blocks);
}

// Функция, которая исполняется в задаче
void logMemoryTask(void *parameter)
{
    while (1)
    {
        logHeapStats();

        // Добавьте другие метрики, если это необходимо
        vTaskDelay(100000 / portTICK_PERIOD_MS); // Задержка на 1 секунду
    }
}

void setup()
{
    Serial.begin(115200);
    std::vector<BLEServiceDescriptor> serviceDescriptors = {
        {BLEUUID(LOCK_SERVICE_UUID), {{BLEUUID(LOCK_STATUS_UUID), [](BLERemoteCharacteristic *characteristic, uint8_t *data, size_t length, bool isNotify)
                                       {
                                           Serial.print("Lock Status Notification: ");
                                           for (size_t i = 0; i < length; i++)
                                           {
                                               Serial.print((char)data[i]);
                                           }
                                           Serial.println();
                                       }},
                                      {BLEUUID(LOCK_CONTROL_UUID), [](BLERemoteCharacteristic *characteristic, uint8_t *data, size_t length, bool isNotify)
                                       {
                                           Serial.print("Lock Control Notification: ");
                                           for (size_t i = 0; i < length; i++)
                                           {
                                               Serial.print((char)data[i]);
                                           }
                                           Serial.println();
                                       }},
                                      {BLEUUID(LOCK_NOTIFICATION_UUID), [](BLERemoteCharacteristic *characteristic, uint8_t *data, size_t length, bool isNotify)
                                       {
                                              Serial.print("Lock Notification: ");
                                              for (size_t i = 0; i < length; i++)
                                              {
                                                  Serial.print((char)data[i]);
                                              }
                                              Serial.println();
                                       }}}}};

    // Создаем задачу FreeRTOS для логирования памяти
    xTaskCreate(
        logMemoryTask,      // Функция, которая исполняется в задаче
        "Log Memory Stats", // Название задачи
        2048,               // Размер стека задачи
        NULL,               // Параметры передаваемые в задачу
        1,                  // Приоритет задачи
        NULL                // Указатель на задачу
    );

    bleClientWrapper = new BLEClientWrapper(serviceDescriptors);
    bleClientWrapper->startScan();
    bleClientWrapper->taskManager();
}

unsigned long previousMillis = 0; // Переменная для хранения времени последнего вызова
const long interval = 1000;       // Интервал между вызовами logHeapStats() в миллисекундах

void loop()
{
    // Основной цикл loop остается пустым
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Уменьшаем загрузку CPU
}
