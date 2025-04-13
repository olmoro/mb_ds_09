/*


*/

#include "staff.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

/* Для структурирования сообщений используются управляющие символы */
#define DLE 0x10 // символ-префикс
#define SOH 0x01 // начало заголовка
#define ISI 0x1F // указатель кода функции FNC
#define STX 0x02 // начало тела сообщения
#define ETX 0x03 // конец тела сообщения

/* Использованы следующие обозначения:
DAD  байт адреса приёмника,
SAD  байт адреса источника,
FNC  байт кода функции,
CRC1, CRC2  циклические контрольные коды
*/

/*
 - заголовок    | SOH | DAD | SAD | ISI | FNC | DataHead |
 или безадресный заголовок  | SOH | ISI | FNC | DataHead |
 - данные                                                | STX | DataSet | ETX |
 - контрольная информация                                                      | CRC1 | CRC2 |
 */

static const char *TAG = "STAFF";

/*
 */
bool staffProcess(const uint8_t *src, uint8_t *dest, size_t src_len, size_t dest_max_len, size_t *dest_actual_len)
{
    size_t j = 0;
    for (size_t i = 0; i < src_len; i++)
    {
        // Определяем, сколько байт нужно записать
        size_t required = (src[i] == SOH || src[i] == ISI || src[i] == STX || src[i] == ETX) ? 2 : 1;

        // Проверяем, достаточно ли места в целевом буфере
        if (j + required > dest_max_len)
        {
            *dest_actual_len = j; // Текущая длина на момент ошибки
            return false;
        }

        // Вставляем байты согласно условию
        switch (src[i])
        {

        case SOH:
            dest[j++] = DLE;
            dest[j++] = SOH;
            break;
        case ISI:
            dest[j++] = DLE;
            dest[j++] = ISI;
            break;
        case STX:
            dest[j++] = DLE;
            dest[j++] = STX;
            break;
        case ETX:
            dest[j++] = DLE;
            dest[j++] = ETX;
            break;
        default:
            dest[j++] = src[i];
        }
    }
    *dest_actual_len = j;

    ESP_LOGI(TAG, "Staffing (%d bytes):", j);
    for (int i = 0; i < j; i++)
    {
        printf("%02X ", dest[i]);
    }
    printf("\n");

    return true;
}

// bool deStaffProcess(const uint8_t *src, uint8_t *dest, size_t src_len, size_t dest_max_len, size_t *dest_actual_len)
// {

//     return true;
// }

/* Пример использования
void app()
{
    uint8_t src[] = {0x00, 0x01, 0x02, 0x01};
    size_t src_len = sizeof(src);
    uint8_t dest[10]; // Буфер с запасом
    size_t dest_len;

    if (staffProcess(src, dest, src_len, sizeof(dest), &dest_len)) {
        // Успешная обработка
        ESP_LOGI("TAG", "Processed data length: %d", dest_len);

        // Далее можно использовать данные в dest
    }
    else
    {
        ESP_LOGE("TAG", "Destination buffer overflow!");
    }
}
*/
