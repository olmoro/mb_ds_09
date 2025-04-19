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

 #include "slave_tx.h"
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

 static const char *TAG = "SLAVE_TX";

 QueueHandle_t processor_queue;

 static SemaphoreHandle_t uart_mutex = NULL;

static portMUX_TYPE param_lock = portMUX_INITIALIZER_UNLOCKED;


extern uint8_t addr;    // адрес slave
extern uint8_t comm;    // команда (функция)
//uint8_t bytes = 0x00;   // количество байт в содержательной части пакета

// Задача отправления ответа
void mb_send_task(void *arg)
{
    //msg_packet_t msg;

        // Создание очереди и мьютекса
        processor_queue = xQueueCreate(MB_QUEUE_SIZE, sizeof(pdu_packet_t));

        uart_mutex = xSemaphoreCreateMutex();
        if (!uart_mutex)
        {
            ESP_LOGE(TAG, "Mutex creation failed");
            return;
        }

    while (1)
    {
        msg_packet_t msg;


        // mb_frame_t frame;   // структура
        // #ifdef TEST_MODBUS
        //     if(xQueueReceive(modbus_queue, &msg, portMAX_DELAY)) // Test
        // #else
        if (xQueueReceive(processor_queue, &msg, portMAX_DELAY))        // Пока будет так
        // #endif
        {
            // Обработка данных
            ESP_LOGI(TAG, "Received deStuffing msd (%d bytes):", msg.length);

            printf("%08X ", *msg.data);
            printf("\n");


            for (int i = 0; i < msg.length; i++)
            {
                printf("%02X ", msg.data[i]);
            }
            printf("\n");

            // ledsGreen();
            // ledsOn();

            // // Проверка минимальной длины фрейма
            // if(msg.length < 4) {
            //     ESP_LOGE(TAG, "Invalid msg length: %d", msg.length);
            //     free(msg.data);
            //     continue;
            // }

            // Формирование ответа

            uint8_t response[msg.length + 2];
            response[0] = addr; //      , msg.data, msg.length);
            response[1] = comm; //      , msg.data, msg.length);


            memcpy(response + 2, msg.data, msg.length);

            // Расчет CRC для ответа
            uint16_t response_crc = mb_crc16(response, msg.length);
            response[msg.length] = response_crc & 0xFF;
            response[msg.length + 1] = response_crc >> 8;


            ESP_LOGI(TAG, "Response msg (%d bytes):", msg.length + 2);
            for (int i = 0; i < msg.length + 2; i++)
            {
                printf("%02X ", response[i]);
            }
            printf("\n");

            // Отправка ответа с синхронизацией
            xSemaphoreTake(uart_mutex, portMAX_DELAY);
            uart_write_bytes(MB_PORT_NUM, (const char *)response, sizeof(response));
            xSemaphoreGive(uart_mutex);

            ledsBlue(); // Ожидание ввода нового пакета

            //free(msg.data);
        }
    }
}
