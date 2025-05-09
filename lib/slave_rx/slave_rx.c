/*
 *      Ключевые проверки и особенности:
 *   1.  Валидация входных параметров
 *   2.  Проверка успешности создания очереди
 *   3.  Обработка таймаута при чтении
 *   4.  Проверка целостности данных в сообщении
 *   5.  Контроль размера буфера получателя
 *   6.  Безопасное копирование данных
 *   7.  Возврат разных кодов ошибок для диагностики - не реализовано
 *   8.  Использование структуры с размером данных для защиты от переполнения - не реализовано
 *
 *       Индикация:
 *               Blue        - ожидание ввода нового пакета modbus;
 *               Red         - чужой адрес, ошибка CRC, ошибка выделения памяти, очередь переполнена или
 *                             возвращён пакет с установленным битом ошибки;
 *               Green       - пакет отправлен в очередь на обработку;
 */

#include "slave_rx.h"
#include "board.h"
#include "processor.h"
#include "project_config.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h> // for standard int types definition
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"

static const char *TAG = "SLAVE_RX";

QueueHandle_t modbus_queue;


static SemaphoreHandle_t uart_mutex = NULL;

// static portMUX_TYPE param_lock = portMUX_INITIALIZER_UNLOCKED;


uint8_t addr = 0x00;    // адрес slave
uint8_t comm = 0x00;    // команда (функция)
//uint8_t bytes = 0x00;   // количество байт в содержательной части пакета


/* Задача приёма пакета по Modbus, его проверка и отправка в очередь для стаффинга */
void slave_rx_task(void *arg)
{
    // Создание очереди и мьютекса
    modbus_queue = xQueueCreate(MB_QUEUE_SIZE, sizeof(pdu_packet_t));

    uart_mutex = xSemaphoreCreateMutex();
    if (!uart_mutex)
    {
        ESP_LOGE(TAG, "Mutex creation failed");
        return;
    }

    uint8_t *frame_buffer = NULL;
    uint16_t frame_length = 0;
    uint32_t last_rx_time = 0;

    while (1)
    {
        uint8_t temp_buf[128];  // уточнить
        uint8_t mb_err = 0x00;  // Ошибок приёма пакета по modbus нет
        uint16_t bytes = 0x00;   // количество байт в содержательной части пакета (сообщение об ошибке)

        int len = uart_read_bytes(MB_PORT_NUM, temp_buf, sizeof(temp_buf), pdMS_TO_TICKS(100));

        if (len > 0)
        {
            // Начало нового фрейма
            if (frame_buffer == NULL)
            {
                frame_buffer = malloc(MAX_PDU_LENGTH + 3);  // + лишнее
                frame_length = 0;
            }

            // Проверка переполнения буфера
            if (frame_length + len > MAX_PDU_LENGTH + 3)
            {
                ESP_LOGE(TAG, "Buffer overflow! Discarding frame");
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;       // mb_err = 0x04; // continue;
            }

            ESP_LOGI(TAG, "len (%d bytes):", len);

            memcpy(frame_buffer + frame_length, temp_buf, len);
            frame_length += len;
            last_rx_time = xTaskGetTickCount();

            ESP_LOGI(TAG, "frame_buffer (%d bytes):", frame_length);
            for (int i = 0; i < frame_length; i++)
            {
                printf("%02X ", frame_buffer[i]);
            }
            printf("\n");
        }

        // Проверка завершения фрейма
        if (frame_buffer && (xTaskGetTickCount() - last_rx_time) > pdMS_TO_TICKS(MB_FRAME_TIMEOUT_MS))
        {
            // Минимальная длина фрейма: адрес + функция + CRC
            if (frame_length < 4)
            {
                ESP_LOGE(TAG, "Invalid frame length: %d", frame_length);
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;       // mb_err = 0x04;
            }

            // Проверка адреса
            if (frame_buffer[0] != SLAVE_ADDRESS)
            {
                ESP_LOGW(TAG, "Address mismatch: 0x%02X", frame_buffer[0]);
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;       // mb_err = 0x04;
            }

            // Проверка CRC
            uint16_t received_crc = (frame_buffer[frame_length - 1] << 8) | frame_buffer[frame_length - 2];
            uint16_t calculated_crc = mb_crc16(frame_buffer, frame_length - 2);

            if (received_crc != calculated_crc)
            {
                ESP_LOGE(TAG, "CRC error: %04X vs %04X", received_crc, calculated_crc);
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;       // mb_err = 0x04;
            }

            ESP_LOGI(TAG,"CRC OK");

            // Подготовка PDU пакета
            pdu_packet_t pdu =
                {
                    .data = malloc(frame_length - 2),
                    .length = frame_length - 2, // Исключаем CRC
                };

            if (pdu.data == NULL)
            {
                ESP_LOGE(TAG, "Memory allocation failed!");
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;       // mb_err = 0x04;
            }

            switch (frame_buffer[1])
            {
            case 0x10:
                // сохранить адрес и команду для ответа
                addr = frame_buffer[0];
                comm = frame_buffer[1];
                if(frame_buffer[frame_length] == 0x00) 
                    bytes = frame_buffer[6] - 1 ;

                pdu.length = bytes;                           // содержательная часть пакета в байтах
                memmove(frame_buffer, frame_buffer + 7, pdu.length);    // сдвиг на 7 байтов
                break;
            
            // case 0x : // другая команда

            default:
                break;
            }


            memcpy(pdu.data, frame_buffer, pdu.length);
            pdu.length = bytes;                         // Это из-за терминала (он оперирует 16-битовым типом)

            ESP_LOGI(TAG, "pdu.length %d bytes:", pdu.length);      // OK
            for(int i = 0; i < pdu.length; i++ )
            {
                printf("%02X ", pdu.data[i]);
            }
            printf("\n");

            if (mb_err == 0x00)
            {
                // Отправка в очередь
                if (xQueueSend(modbus_queue, &pdu, 0) != pdTRUE)
                {
                    // if(xQueueSend(queues->modbus_queue, &pdu, 0) != pdTRUE) {     //if(xQueueSend(queues->modbus_queue, &pdu, pdMS_TO_TICKS(100)) != pdPASS) {
                    ESP_LOGE(TAG, "Queue full! Dropping PDU");
                    free(pdu.data);
                    continue;
                }
                
            }


        //     // else
        //     // {
        //         /* Если mb_err == true, то пакет принят с ошибкой, либо очередь на обработку пакета
        //             переполнена. Формируется ответ c установленным старшим битом в коде команды */
        //         uint8_t response[5] =
        //             {
        //                 addr,                   // Адрес
        //                 comm |= 0x80,           // Функция
        //                 pdu.data[2] = mb_err,   // Код ошибки
        //                 0x00, 0x00              // Место для CRC
        //             };
        //         uint8_t len = sizeof(response);

        //         /* Расчет CRC для ответа */
        //         uint16_t response_crc = mb_crc16(response, len - 2);
        //         response[3] = response_crc & 0xFF;
        //         response[4] = response_crc >> 8;

        //         ESP_LOGI(TAG, "Response (%d bytes):", len);
        //         for (int i = 0; i < len; i++)
        //         {
        //             printf("%02X ", response[i]);
        //         }
        //         printf("\n");

        //         /* Отправка ответа с синхронизацией */
        //         xSemaphoreTake(uart_mutex, portMAX_DELAY);
        //         uart_write_bytes(MB_PORT_NUM, (const char *)response, sizeof(response));
        //         xSemaphoreGive(uart_mutex);

        //         ledsBlue();     // Ожидание ввода нового пакета
        //         free(pdu.data); // Освобождение памяти после обработки сообщения
        // //    }

            free(frame_buffer);
            frame_buffer = NULL;
            frame_length = 0;
        }
    }
}

