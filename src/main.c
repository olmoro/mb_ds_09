#include <string.h>
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "project_config.h"
#include "board.h"
#include "slave_rx.h"
#include "slave_tx.h"
#include "processor.h"
#include "processor_tx.h"

static const char *TAG = "MODBUS_MAIN";

void app_main()
{
    // Инициализация периферии
    boardInit();
    uart_mb_init();
    uart_sp_init();

    /* Проверка RGB светодиода */
    ledsBlue();

    // Создание задач
    BaseType_t slave_rx_task_handle = xTaskCreate(slave_rx_task, "slave_rx", 4096, NULL, 5, NULL);
    if (!slave_rx_task_handle)
    {
        ESP_LOGE(TAG, "Failed to create Slave RX task");
        return;
    }
    ESP_LOGI(TAG, "Slave RX task created successfully");

    // BaseType_t slave_tx_task_handle = xTaskCreate(slave_tx_task, "slave_tx", 4096, NULL, 6, NULL);
    // if (!slave_tx_task_handle)
    // {
    //     ESP_LOGE(TAG, "Failed to create Slave TX task");
    //     return;
    // }
    // ESP_LOGI(TAG, "Slave TX task created successfully");

    BaseType_t processor_rx_task_handle = xTaskCreate(processor_rx_task, "processor_r[]", 4096, NULL, 7, NULL);
    if (!processor_rx_task_handle)
    {
        ESP_LOGE(TAG, "Failed to create Processor task");
        return;
    }
    ESP_LOGI(TAG, "Processor task created successfully");

    BaseType_t processor_tx_task_handle = xTaskCreate(processor_tx_task, "processor_tx", 4096, NULL, 8, NULL);
    if (!processor_tx_task_handle)
    {
        ESP_LOGE(TAG, "Failed to create Processor TX task");
        return;
    }
    ESP_LOGI(TAG, "Processor TX task created successfully");
}
