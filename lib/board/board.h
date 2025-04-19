/*
  Файл   board.h
  Проект
  pcb: spn.55

  OTA: "Я реализовал вариант обновления с сервера TLS. Используется шифрование 
  и проверка допустимости версий. Мне это подходит больше."
*/

#ifndef _BOARD_H_
#define _BOARD_H_

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void boardInit();
    void uart_mb_init();
    void uart_sp_init();

    void ledsOn();
    void ledsRed();
    void ledsGreen();
    void ledsBlue();
    void ledsOff();

    void ledRedToggle();
    void ledGreenToggle();
    void ledBlueToggle();

    uint16_t mb_crc16(const uint8_t *buffer, size_t length);
    uint16_t CRCode(const uint8_t *msg, size_t len);


#ifdef __cplusplus
}
#endif

#endif // !_BOARD_H_
