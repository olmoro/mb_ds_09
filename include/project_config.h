/*
   ---------------------------------------------------------------------------------
                            Файл конфигурации проекта
   ---------------------------------------------------------------------------------
*/

#pragma once

#include <stdint.h>
#include "esp_task.h"

#include <time.h>
#include <stdio.h>
// #include <stdint.h>
#include <stdbool.h>
#include "esp_bit_defs.h"
// ---------------------------------------------------------------------------------
//                                  Версии
// ---------------------------------------------------------------------------------
#define APP_VERSION "MB_DS_09 20250420.16"
// 202500420.16:  add: дестаффинг c одним буфером           RAM:  3.5%  Flash: 13.3%
// 202500419.15:  add:                               RAM:  3.5%  Flash: 13.2%
// 202500417.14:  add: дестаффинг         OK                RAM:  3.5%  Flash: 13.2%
// 202500416.12:  add: reply from SP      OK                RAM:  3.5%  Flash: 13.2%
// molbus -> 01 10 00 02 00 0A 14 01 00 86 1F 1D 33 33 32 02 09 30 30 30 09 30 30 33 0C 03 00 59 2D
// sp     -> 10 01 00 86 10 1F 1D 33 33 32 10 02 09 30 30 30 09 30 30 33 0C 10 03 42 16
// reply  <- FF FF 10 01 86 00 10 1F 03 33 33 32 10 02 09 30 09 30 30 33 0C 09 32 30 36 30 31 30 30 30 30 35 09 20 0C 10 03 32 61


#define TEST_MODBUS

// ---------------------------------------------------------------------------------
//                                  GPIO
// ---------------------------------------------------------------------------------
// Плата SPN.55
// Светодиоды
#define RGB_RED_GPIO 4   // Красный, катод на GND (7mA)
#define RGB_GREEN_GPIO 2 // Зелёный, катод на GND (5mA)
#define RGB_BLUE_GPIO 27 // Синий,   катод на GND (4mA)

// Входы
#define CONFIG_GPIO_IR 19 // Вход ИК датчика

// UART1
#define CONFIG_MB_UART_RXD 25
#define CONFIG_MB_UART_TXD 26
#define CONFIG_MB_UART_RTS 33

// UART2
#define CONFIG_SP_UART_RXD 21
#define CONFIG_SP_UART_TXD 23
#define CONFIG_SP_UART_RTS 22

#define CONFIG_UART_DTS 32 // Не используется

// ---------------------------------------------------------------------------------
//                                    Общие
// ---------------------------------------------------------------------------------
#define BUF_SIZE (512) // размер буфера
#define MAX_PDU_LENGTH 180

// Структура для передаваемого пакета
typedef struct
{
    uint8_t *data;
    uint16_t length;
} pdu_packet_t;

// Структура для передаваемого пакета
typedef struct 
{
  uint8_t *data;
  size_t length;
} msg_packet_t;

// ---------------------------------------------------------------------------------
//                                    MODBUS
// ---------------------------------------------------------------------------------

// #define PRB // Тестовый. Их принятого фрейма исключается вся служебная информация

#define MB_PORT_NUM UART_NUM_1
#define MB_BAUD_RATE 9600
#define SLAVE_ADDRESS 0x01
#define MB_QUEUE_SIZE 10
#define MB_FRAME_TIMEOUT_MS 4 // 3.5 символа при 19200 бод


// #define QUEUE_LENGTH 10        // Максимальное количество элементов в очереди
// #define ITEM_SIZE sizeof(modbus_packet_t)

// Test
// #define CONFIG_STAFF_TASK_STACK_SIZE 1024 * 4
// #define CONFIG_STAFF_TASK_PRIORITY CONFIG_SLAVE_TASK_PRIORITY - 1 // 9


// /* Используются две очереди FreeRTOS для двусторонней коммуникации */
// QueueHandle_t modbus_queue;
// QueueHandle_t processing_queue;

// /* Используются две очереди FreeRTOS для двусторонней коммуникации */
// typedef struct
// {
//     QueueHandle_t modbus_queue;    // От Modbus к SPnet
//     QueueHandle_t process_queue; // От SPnet к Modbus
// } TaskQueues;

// ---------------------------------------------------------------------------------
//                                    SP
// ---------------------------------------------------------------------------------

#define SP_PORT_NUM UART_NUM_2
#define SP_BAUD_RATE 9600

#define SP_ADDRESS 0x00
#define SP_QUEUE_SIZE 10
#define SP_FRAME_TIMEOUT_MS 4 // 3.5 символа при 19200 бод

// Константы протокола
#define SOH 0x01        // Байт начала заголовка
#define ISI 0x1F        // Байт указателя кода функции
#define STX 0x02        // Байт начала тела сообщения
#define ETX 0x03        // Байт конца тела сообщения
#define DLE 0x10        // Байт символа-префикса
#define CRC_INIT 0x1021 // Битовая маска полинома
// #define FRAME 128       //

// ---------------------------------------------------------------------------------
//                                    Задачи (предварительно)
// ---------------------------------------------------------------------------------

// Размер стеков
#define CONFIG_MB_TASK_STACK_SIZE 1024 * 4
#define CONFIG_PR_TASK_STACK_SIZE 1024 * 4

// Приоритеты
#define CONFIG_MB_RECEIVE_TASK_PRIORITY 5 // modbus_receive_task
#define CONFIG_PROCESSING_TASK_PRIORITY 4 // processing_task
