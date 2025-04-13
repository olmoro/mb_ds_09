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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sdkconfig.h"

// static const char *logTAG = "BOARD";

gpio_num_t _rgb_red_gpio = static_cast<gpio_num_t>(RGB_RED_GPIO);
gpio_num_t _rgb_green_gpio = static_cast<gpio_num_t>(RGB_GREEN_GPIO);
gpio_num_t _rgb_blue_gpio = static_cast<gpio_num_t>(RGB_BLUE_GPIO);

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
