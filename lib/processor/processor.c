/*
   **Благие пожелания от DeepSeek`а:**

1. **Разделение задач**:
   - Чтение UART с детектированием фреймов по таймауту
   - Обработка фреймов с проверкой CRC
   - Формирование и отправка ответов

2. **Синхронизация**:
   - Мьютекс для защиты операций записи в UART
   - Очередь FreeRTOS для передачи фреймов между задачами

3. **Проверки ошибок**:
   - Контроль переполнения буфера
   - Проверка CRC входящих фреймов
   - Обработка ошибок выделения памяти
   - Защита от переполнения очереди

4. **Безопасность памяти**:
   - Динамическое выделение памяти для каждого фрейма
   - Гарантированное освобождение памяти

5. **Соответствие протоколу**:
   - Правильный расчет времени таймаута (3.5 символа)
   - Корректная реализация CRC16 для МАГИСТРАЛЬНОГО ПРОТОКОЛА СПСеть.
            см. Руководство программиста РАЖГ.00276-33
*/


#include "processor_tx.h"
#include "board.h"
#include "staff.h"
#include "project_config.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>                 // for standard int types definition
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"

// // Структура для передачи фреймов между задачами
// typedef struct
// {
//     uint8_t *data;
//     size_t length;
// } mb_frame_t;

/* Пример сообщения:
   Заголовок:       DLE SOH DAD SAD DLE ISI FNC DataDLEHead 
   Тело сообщения:  DLE STX DataDLESet DLE ETX CRC1 CRC2
*/
static const char *TAG = "PROC_RX";

extern QueueHandle_t modbus_queue;


// Задача обработки полученного по модбасу фрейма (без MB_CRC)
void processor_rx_task(void *arg) 
{
    //pdu_packet_t pdu;

    while(1) 
    {
        pdu_packet_t pdu;

        uint8_t temp_buf[240];
        uint8_t sp_err = 0x00;  // Ошибок приёма пакета по sp нет

        // uint8_t src[] = {0x00, 0x01, 0x02, 0x01};
        // size_t src_len = sizeof(src);
        // uint8_t dest[10]; // Буфер с запасом
        // size_t dest_len;


        // Прием сообщения от modbus_receive_task (реализованы не все проверки)
        if(xQueueReceive(modbus_queue, &pdu, portMAX_DELAY)) 
        {
            // Обработка данных (зарезервировать 2 байта для CRC)
            ESP_LOGI(TAG, "Received PDU (%d bytes):", pdu.length);
            uint8_t src[pdu.length];
            uint8_t dest[pdu.length * 2];
            size_t dest_len;

            for(int i = 0; i < pdu.length; i++) 
            {
                printf("%02X ", pdu.data[i]);
                src[i] = pdu.data[i];
            }
            printf("\n");
           
            if(staffProcess(src, dest, pdu.length, sizeof(dest), &dest_len)) 
            {
                // Успешная обработка
                ESP_LOGI(TAG, "staffProcess data length: %d", dest_len);
                // Далее можно использовать данные в dest
            } 
            else 
            {
                ESP_LOGE(TAG, "Destination buffer overflow!");
            }

            // Формирование ответа для UART2 (SP)
            uint8_t response[dest_len + 2];     // +2 байта для контрольной суммы
            memcpy(response, dest, dest_len);

            // Расчет CRC для ответа (функция откорректирована в части типов данных)
            uint16_t response_crc = CRCode(response + 2, dest_len - 2);
            response[dest_len + 1] = response_crc & 0xFF;
            response[dest_len] = response_crc >> 8;

            // ESP_LOGI(TAG, "Response for send to SP (%d bytes):", dest_len + 2);
            // for(int i = 0; i < dest_len + 2; i++) 
            dest_len += 2;
            ESP_LOGI(TAG, "Response for send to SP (%d bytes):", dest_len);
            for(int i = 0; i < dest_len; i++) 
            {
                printf("%02X ", response[i]);
            }
            printf("\n");

            // Отправка ответа //с синхронизацией
            //    xSemaphoreTake(uart_mutex, portMAX_DELAY);
            uart_write_bytes(SP_PORT_NUM, (const char *)response, sizeof(response));
            //    xSemaphoreGive(uart_mutex);  

        }
        //free(msg.data); // Освобождение памяти

    }
}
