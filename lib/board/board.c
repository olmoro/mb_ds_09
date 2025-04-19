/*
  Работа с аппаратными ресурсами платы (упрощённый вариант)
  pcb: spn.55
  22.03.2025
*/

#include "board.h"
#include "project_config.h"
#include <stdio.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sdkconfig.h"
#include "driver/uart.h"

static const char *TAG = "BOARD";

// gpio_num_t _rgb_red_gpio = static_cast<gpio_num_t>(RGB_RED_GPIO);
// gpio_num_t _rgb_green_gpio = static_cast<gpio_num_t>(RGB_GREEN_GPIO);
// gpio_num_t _rgb_blue_gpio = static_cast<gpio_num_t>(RGB_BLUE_GPIO);
gpio_num_t _rgb_red_gpio = RGB_RED_GPIO;
gpio_num_t _rgb_green_gpio = RGB_GREEN_GPIO;
gpio_num_t _rgb_blue_gpio = RGB_BLUE_GPIO;
void boardInit()
{
    /* Инициализация GPIO (push/pull output) */
    gpio_reset_pin(_rgb_red_gpio);
    gpio_reset_pin(_rgb_green_gpio);
    gpio_reset_pin(_rgb_blue_gpio);
    gpio_set_direction(_rgb_red_gpio, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_direction(_rgb_green_gpio, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_direction(_rgb_blue_gpio, GPIO_MODE_INPUT_OUTPUT);

    /* Установка начального состояния (выключено) */
    gpio_set_level(_rgb_red_gpio, 0);
    gpio_set_level(_rgb_green_gpio, 0);
    gpio_set_level(_rgb_blue_gpio, 0);
}

void uart_mb_init()
{
    /* Configure parameters of an UART driver, communication pins and install the driver */
    uart_config_t uart_mb_config = {
        .baud_rate = MB_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, // RTS для управления направлением DE/RE !!
        .rx_flow_ctrl_thresh = 122,
    };

    //     int intr_alloc_flags = 0;

    // #if CONFIG_UART_ISR_IN_IRAM
    //     intr_alloc_flags = ESP_INTR_FLAG_IRAM;
    // #endif

    ESP_ERROR_CHECK(uart_driver_install(MB_PORT_NUM, BUF_SIZE, BUF_SIZE, MB_QUEUE_SIZE, NULL, 0));
    ESP_ERROR_CHECK(uart_set_pin(MB_PORT_NUM, CONFIG_MB_UART_TXD, CONFIG_MB_UART_RXD, CONFIG_MB_UART_RTS, 32)); // IO32 свободен (трюк)
    ESP_ERROR_CHECK(uart_set_mode(MB_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX));                                   // activate RS485 half duplex in the driver
    ESP_ERROR_CHECK(uart_param_config(MB_PORT_NUM, &uart_mb_config));
    ESP_LOGI(TAG, "slave_uart initialized.");
}

void uart_sp_init()
{
    /* Configure parameters of an UART driver, communication pins and install the driver */
    uart_config_t uart_sp_config = 
    {
        .baud_rate = SP_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,  // RTS для управления направлением DE/RE !!
        .rx_flow_ctrl_thresh = 122,
    };

//     int intr_alloc_flags = 0;

// #if CONFIG_UART_ISR_IN_IRAM
//     intr_alloc_flags = ESP_INTR_FLAG_IRAM;
// #endif

    ESP_ERROR_CHECK(uart_driver_install(SP_PORT_NUM, BUF_SIZE, BUF_SIZE, SP_QUEUE_SIZE, NULL, 0));
    ESP_ERROR_CHECK(uart_set_pin(SP_PORT_NUM, CONFIG_SP_UART_TXD, CONFIG_SP_UART_RXD, CONFIG_SP_UART_RTS, 32));   // IO32 свободен (трюк)
    ESP_ERROR_CHECK(uart_set_mode(SP_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX)); // activate RS485 half duplex in the driver
    ESP_ERROR_CHECK(uart_param_config(SP_PORT_NUM, &uart_sp_config));  
    ESP_LOGI(TAG, "sp_uart initialized.");
}

void ledsOn()
{
    gpio_set_level(_rgb_red_gpio, 1);
    gpio_set_level(_rgb_green_gpio, 1);
    gpio_set_level(_rgb_blue_gpio, 1);
}
void ledsRed()
{
    gpio_set_level(_rgb_red_gpio, 1);
    gpio_set_level(_rgb_green_gpio, 0);
    gpio_set_level(_rgb_blue_gpio, 0);
}
void ledsGreen()
{
    gpio_set_level(_rgb_red_gpio, 0);
    gpio_set_level(_rgb_green_gpio, 1);
    gpio_set_level(_rgb_blue_gpio, 0);
}
void ledsBlue()
{
    gpio_set_level(_rgb_red_gpio, 0);
    gpio_set_level(_rgb_green_gpio, 0);
    gpio_set_level(_rgb_blue_gpio, 1);
}

void ledsOff()
{
    gpio_set_level(_rgb_red_gpio, 0);
    gpio_set_level(_rgb_green_gpio, 0);
    gpio_set_level(_rgb_blue_gpio, 0);
}

void ledRedToggle()
{
    if (gpio_get_level(_rgb_red_gpio))
        gpio_set_level(_rgb_red_gpio, 0);
    else
        gpio_set_level(_rgb_red_gpio, 1);
}

void ledGreenToggle()
{
    if (gpio_get_level(_rgb_green_gpio))
        gpio_set_level(_rgb_green_gpio, 0);
    else
        gpio_set_level(_rgb_green_gpio, 1);
}

void ledBlueToggle()
{
    if (gpio_get_level(_rgb_blue_gpio))
        gpio_set_level(_rgb_blue_gpio, 0);
    else
        gpio_set_level(_rgb_blue_gpio, 1);
}


uint16_t mb_crc16(const uint8_t *buffer, size_t length)
{
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < length; i++)
    {
        crc ^= (uint16_t)buffer[i];

        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

/* Контрольные коды насчитывается с байта, следующего за SOH, поскольку два первых байта DLE
    и SOH проверяются явно при выделении начала сообщения. Контрольные коды охватывают все байты,
    включая ETX и все стаффинг символы в этом промежутке.
*/
uint16_t CRCode(const uint8_t *msg, size_t len)
{
    int j, crc = 0;
    while (len-- > 0)
    {
        crc = crc ^ (int)*msg++ << 8;
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ CRC_INIT;    //0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

