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


#include "processor.h"
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
typedef struct
{
    uint8_t *data;
    size_t length;
} mb_frame_t;

/* Обмен может выполняться на скоростях (бит/с.): */
uint32_t bauds[10] = {300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};

/* Пример сообщения:
   Заголовок:       DLE SOH DAD SAD DLE ISI FNC DataDLEHead 
   Тело сообщения:  DLE STX DataDLESet DLE ETX CRC1 CRC2
*/
static const char *TAG = "PROCESSOR";

extern QueueHandle_t modbus_queue;
extern QueueHandle_t processor_queue;




// Задача обработки полученного по модбасу фрейма (без MB_CRC)
void frame_processor_task(void *arg) 
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

            // Расчет CRC для ответа (функия откорректирована в части типов данных)
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

// ===========================================================================================
            // Далее будет приём пакета от целевого прибора
        //    uint8_t temp_buf[128];
            uint8_t mb_err = 0x00; // Ошибок приёма пакета по sp нет
    
            int len = uart_read_bytes(SP_PORT_NUM, temp_buf, sizeof(temp_buf), pdMS_TO_TICKS(100));

            // Проверка CRC
            uint16_t received_crc = (temp_buf[len - 2] << 8) | temp_buf[len - 1];
            uint16_t calculated_crc = CRCode(temp_buf + 4, len - 6);              // Исключая FF FF 10 01 в начале и два байта CRC
            if (received_crc != calculated_crc)
            {
                ESP_LOGE(TAG, "len: %02X CRC error: %04X vs %04X", len, received_crc, calculated_crc);

                sp_err = 0x04;
            }
            else
            {
                ESP_LOGI(TAG, "CRC OK: %04X vs %04X", received_crc, calculated_crc);



                // сюда перенести 

            }


            // uint8_t dest[BUF_SIZE];
            // size_t dest_len;

    // @@@  esp_err_t deStaffProcess(const uint8_t *src, uint8_t *dest, size_t src_size, size_t dest_size, size_t *dest_len); 

            esp_err_t ret = deStaffProcess(temp_buf + 2, dest, len - 4, sizeof(dest), &dest_len);
            if (ret == ESP_OK)
            {
                ESP_LOGI(TAG, "deStaff Processed %d bytes", dest_len);
                for (int i = 0; i < dest_len; i++)
                    printf("%02X ", dest[i]);
                printf("\n");
    /* OK */
                // подготовка пакета для отправки в очередь
                // Подготовка PDU пакета


                msg_packet_t msg =
                {
                    .data = malloc(dest_len),       // + 2)
                    .length = dest_len,             // + 2,                      // Без CRC и + 2 байта на количество байт 
                };

                // msg.data[0] = dest_len >> 8;
                // msg.data[1] = dest_len & 0xFF;
                // memcpy(msg.data + 2, dest, msg.length);
                memcpy(msg.data, dest, sizeof(dest));      //          msg.length);

                ESP_LOGI(TAG, "deStaff msg %d bytes", dest_len);
                for (int i = 0; i < dest_len; i++)                //
                    printf("%02X ", msg.data[i]);
                printf("\n");

    /* OK */



                // Отправка в очередь
                // if(xQueueSend(queues->modbus_queue, &msg, 0) != pdTRUE)      // if(xQueueSend(queues->modbus_queue, &msg, pdMS_TO_TICKS(100)) != pdPASS) {

                if (xQueueSend(processor_queue, &msg, 0) != pdTRUE)
                {
                    ESP_LOGE(TAG, "Queue full! Dropping PDU");
                    free(msg.data);
                    continue;
                }


                free(msg.data);

            }
            else
            {
                ESP_LOGE(TAG, "deStaff Error: %s", esp_err_to_name(ret));
            }

            // dest
            // dest_len

 






            // memcpy(msg.data, temp_buf+2, len-2)
            // msg->l .lenght = len-2;

            // // Подготовка PDU пакета
            // msg_packet_t msg =
            //     {
            //         .length = len - 2, // Исключаем FF FF
            //         .data = malloc(temp_buf + 2)
            //     };


            // // #ifdef TEST_MODBUS
            //     // Отправка в очередь
            //     // if(xQueueSend(processor_queue, &msg, pdMS_TO_TICKS(100)) != pdPASS) 
            //     // {

            //         ledsOn();

            //     if (xQueueSend(processor_queue, &msg, 0) != pdTRUE)
            //     {
            // //     //     // if(xQueueSend(queues->modbus_queue, &msg, 0) != pdTRUE) {     //if(xQueueSend(queues->modbus_queue, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
            //         ESP_LOGE(TAG, "Queue full! Dropping PDU");
            //         //free(msg.data);
            //         continue;
            //      }
            // // #endif


            // далее staffing с выбором параметров обмена с целевым прибором
            // отправка фрейма по uart2
            // приём ответа, дестаффинг
            // отправка в очередь to_modbus_queue

//    ledsGreen();        

        }
        //free(msg.data); // Освобождение памяти

    }
}
