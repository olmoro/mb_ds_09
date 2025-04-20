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

// Структура для передачи фреймов между задачами
// typedef struct
// {
//     uint8_t *data;
//     size_t length;
// } mb_frame_t;

/* Пример сообщения:
   Заголовок:       DLE SOH DAD SAD DLE ISI FNC DataDLEHead 
   Тело сообщения:  DLE STX DataDLESet DLE ETX CRC1 CRC2
*/
static const char *TAG = "PROC_TX";

QueueHandle_t processor_queue;

static SemaphoreHandle_t uart_mutex = NULL;



/* Задача получения фрейма от целевого прибора, проверки CRC, 
дестаффинга и отправки фрейма в очередь processor_queue. */
void processor_tx_task(void *arg) 
{
    // Создание очереди и мьютекса
    processor_queue = xQueueCreate(SP_QUEUE_SIZE, sizeof(msg_packet_t));

    uart_mutex = xSemaphoreCreateMutex();
    if (!uart_mutex)
    {
        ESP_LOGE(TAG, "Mutex creation failed");
        return;
    }

    uint8_t *frame_buffer = NULL;
    uint16_t frame_length = 0;
    uint32_t last_rx_time = 0;

    uint8_t *dest_buffer = NULL;
    size_t dest_length = 0;

    while(1)
    {
        uint8_t temp_buf[128];  // уточнить
        uint8_t sp_err = 0x00;  // Ошибок приёма пакета по sp нет
        uint16_t bytes = 0x00;   // количество байт в содержательной части пакета (сообщение об ошибке)

        int len = uart_read_bytes(SP_PORT_NUM, temp_buf, sizeof(temp_buf), pdMS_TO_TICKS(100));

        if (len > 0)
        {
            // Начало нового фрейма
            if (frame_buffer == NULL)
            {
                frame_buffer = malloc(MAX_PDU_LENGTH);
                frame_length = 0;
            }

            // Проверка переполнения буфера
            if (frame_length + len > MAX_PDU_LENGTH)
            {
                ESP_LOGE(TAG, "Buffer overflow! Discarding frame");
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;
            }

            memcpy(frame_buffer + frame_length, temp_buf, len);
            frame_length += len;
            last_rx_time = xTaskGetTickCount();

            ESP_LOGI(TAG, "Copy of temp_buf: %d", frame_length);    // FF FF .... 32 61 (39)
            for (int i = 0; i < frame_length; i++)
            {
                printf("%02X ", frame_buffer[i]);
            }
            printf("\n");

        }

        // Проверка завершения фрейма
        if (frame_buffer && (xTaskGetTickCount() - last_rx_time) > pdMS_TO_TICKS(MB_FRAME_TIMEOUT_MS))
        {
            // Минимальная длина фрейма: адрес + функция + CRC уточнить
            if (frame_length < 4)
            {
                ESP_LOGE(TAG, "Invalid frame length: %d", frame_length);
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;
            }

            // // Проверка адреса - уточнить
            // if (frame_buffer[???] != SP_ADDRESS)
            // {
            //     ESP_LOGW(TAG, "Address mismatch: 0x%02X", frame_buffer[0]);
            //     free(frame_buffer);
            //     frame_buffer = NULL;
            //     frame_length = 0;
            //     continue;
            // }

            // Контрольная сумма сохраняется для сравнения
            uint16_t received_crc = (frame_buffer[frame_length - 2] << 8) | frame_buffer[frame_length - 1];
            
            // в начале буфера два FF, они исключаются, как и 2 байта контрольной суммы
            frame_length -= 4;
            if (frame_buffer[0] && frame_buffer[1])
            {
                memcpy(frame_buffer, frame_buffer + 2, frame_length);       // FF FF убираются сдвигом, а за одно и CRC
            }

            ESP_LOGI(TAG, "CRC: %04X", received_crc);       // 3261

            ESP_LOGI(TAG, "frame bytes: %d", frame_length); // (35)
            for (int i = 0; i < frame_length; i++)          // 10 01 ... 10 03 32 61
            {
                printf("%02X ", frame_buffer[i]);
            }
            printf("\n");

            // Проверка CRC
            uint16_t calculated_crc = CRCode(frame_buffer + 2, frame_length - 2); // Исключая 10 01 в начале
        
            if (received_crc != calculated_crc)
            {
                ESP_LOGE(TAG, "CRC error: %04X vs %04X", received_crc, calculated_crc);
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;
            }

            ESP_LOGI(TAG, "CRC OK: %04X vs %04X", received_crc, calculated_crc);        // OK

           // Подготовка PDU пакета (вариант дестаффинга непосредственно в пакете сдвигом)
            msg_packet_t pdu =
           {
               .data = malloc(frame_length),
               .length = frame_length,      // + 2, если нужна CRC
           };

            memcpy(pdu.data, frame_buffer, frame_length);

           if (pdu.data == NULL)
           {
               ESP_LOGE(TAG, "Memory allocation failed!");
               free(frame_buffer);
               frame_buffer = NULL;
               frame_length = 0;
               continue;
           }

            // Попытаться произвести дестаффинг здесь
            /* @@@ esp_err_t deStaff(uint8_t *buffer, size_t *length); */
            esp_err_t ret = deStaff(pdu.data, &pdu.length);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "Destuffing failed!");
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;
            }
            
            ESP_LOGI(TAG, "deStaff Processed %d bytes", pdu.length);
            for (int i = 0; i < pdu.length; i++)
                printf("%02X ", pdu.data[i]);
            printf("\n");


//  ====================== OK ======================


            // // Отправка в очередь
            // if (xQueueSend(processor_queue, &pdu, 0) != pdTRUE)
            // {
            //     // if(xQueueSend(queues->modbus_queue, &pdu, 0) != pdTRUE) {     //if(xQueueSend(queues->modbus_queue, &pdu, pdMS_TO_TICKS(100)) != pdPASS) {
            //     ESP_LOGE(TAG, "Queue full! Dropping PDU");
            //     free(pdu.data);
            //     continue;
            // }

            free(pdu.data); // Освобождение памяти после обработки сообщения

            free(frame_buffer);
            frame_buffer = NULL;
            frame_length = 0;            
        } // Проверка завершения фрейма
    } // while(1)
} //processor_tx_task


    // //     msg_packet_t msg;
    // // //    uint8_t src[240]; // Уточнить максимальный размер пакета
    // // //    uint8_t dest[240]; // Уточнить максимальный размер пакета


    // //         // Обработка данных (зарезервировать 2 байта для CRC   ??   )
    // //         ESP_LOGI(TAG, "Received PDU (%d bytes):", &msg.length);
    // //         uint8_t src[msg.length];
    // //         uint8_t dest[msg.length];
    // //         size_t dest_len;

    // //        int len = uart_read_bytes(SP_PORT_NUM, src, sizeof(src), pdMS_TO_TICKS(100));


    //     // Проверка CRC
    //     uint16_t received_crc = (src[len - 2] << 8) | src[len - 1];
    //     uint16_t calculated_crc = CRCode(src + 4, len - 6); // Исключая FF FF 10 01 в начале и два байта CRC
    //     if (received_crc != calculated_crc)
    //     {
    //         ESP_LOGE(TAG, "len: %02X CRC error: %04X vs %04X", len, received_crc, calculated_crc);
    //         // С этим надо что-то делать ...
    //         //sp_err = 0x04;
    //         //free(pdu.data);
    //         continue;
    //     }
    //     else
    //     {
    //         ESP_LOGI(TAG, "CRC OK: %04X vs %04X", received_crc, calculated_crc);

    //         // Попытаться произвести дестаффинг здесь
    //         esp_err_t ret = deStaffProcess(src + 2, dest, len - 4, sizeof(dest), &dest_len);
    //         if (ret == ESP_OK)
    //         {
    //             ESP_LOGI(TAG, "deStaff Processed %d bytes", msg.length);
    //             for (int i = 0; i < msg.length; i++)
    //                 printf("%02X ", dest[i]);
    //             printf("\n");


    //         }
