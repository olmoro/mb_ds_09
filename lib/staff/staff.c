/*
 - заголовок    | SOH | DAD | SAD | ISI | FNC | DataHead |
 или безадресный заголовок  | SOH | ISI | FNC | DataHead |
 - данные                                                | STX | DataSet | ETX |
 - контрольная информация                                                      | CRC1 | CRC2 |
 */

#include "staff.h"
#include "project_config.h"
#include <stdint.h>
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



static const char *TAG = "STAFF";

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

esp_err_t deStaffProcess(const uint8_t *src, uint8_t *dest, size_t src_size, size_t dest_size, size_t *dest_len) 
{
    // Проверка входных параметров
    // if (src == NULL || dest == NULL || dest_len == NULL) 
    // {
    //     return ESP_ERR_INVALID_ARG;
    // }
    // if (src_size != BUF_SIZE || dest_size != BUF_SIZE) 
    // {
    //     return ESP_ERR_INVALID_SIZE;
    // }

    size_t i = 0;
    size_t j = 0;

    while (i < src_size) 
    {
        // Проверяем текущий байт на  DLE и наличие следующего байта
        if (src[i] == DLE && (i + 1) < src_size) 
        {
            uint8_t next_byte = src[i + 1];
            
            // Проверяем следующий байт на совпадение с целевыми значениями
            if (next_byte == SOH || next_byte == STX || next_byte == ETX || next_byte == ISI) 
            {
                // // Пропускаем  DLE, копируем следующий байт
                // if (j >= dest_size) 
                // {
                //     *dest_len = j;
                //     return ESP_ERR_INVALID_SIZE;
                // }
                dest[j++] = next_byte;
                i += 2;
            } 
            else 
            {
                // Сохраняем текущий байт DLE
                if (j >= dest_size) 
                {
                    *dest_len = j;
                    return ESP_ERR_INVALID_SIZE;
                }
                dest[j++] = src[i++];
            }
        } 
        else 
        {
            // Копируем текущий байт
            if (j >= dest_size) 
            {
                *dest_len = j;
                return ESP_ERR_INVALID_SIZE;
            }
            dest[j++] = src[i++];
        }
    }

    *dest_len = j;
    return ESP_OK;
}





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

// ================================== deStuff ==================================

// #include <stdint.h>
// #include "esp_err.h"

// esp_err_t deStaffProcess(const uint8_t *src, uint8_t *dest, size_t src_size, size_t dest_size, size_t *dest_len) 
// {
//     // Проверка входных параметров
//     if (src == NULL || dest == NULL || dest_len == NULL) 
//     {
//         return ESP_ERR_INVALID_ARG;
//     }
//     if (src_size != BUF_SIZE || dest_size != BUF_SIZE) 
//     {
//         return ESP_ERR_INVALID_SIZE;
//     }

//     size_t i = 0;
//     size_t j = 0;

//     while (i < src_size) 
//     {
//         // Проверяем текущий байт на  DLE и наличие следующего байта
//         if (src[i] ==  DLE && (i + 1) < src_size) 
//         {
//             uint8_t next_byte = src[i + 1];
            
//             // Проверяем следующий байт на совпадение с целевыми значениями
//             if (next_byte == SOH || next_byte == STX || next_byte == ETX || next_byte == ISI) 
//             {
//                 // Пропускаем  DLE, копируем следующий байт
//                 if (j >= dest_size) 
//                 {
//                     *dest_len = j;
//                     return ESP_ERR_INVALID_SIZE;
//                 }
//                 dest[j++] = next_byte;
//                 i += 2;
//             } 
//             else 
//             {
//                 // Сохраняем текущий байт  DLE
//                 if (j >= dest_size) 
//                 {
//                     *dest_len = j;
//                     return ESP_ERR_INVALID_SIZE;
//                 }
//                 dest[j++] = src[i++];
//             }
//         } 
//         else 
//         {
//             // Копируем текущий байт
//             if (j >= dest_size) 
//             {
//                 *dest_len = j;
//                 return ESP_ERR_INVALID_SIZE;
//             }
//             dest[j++] = src[i++];
//         }
//     }

//     *dest_len = j;
//     return ESP_OK;
// }





// // Пример использования:

// void app() 
// {
//     uint8_t input[BUF_SIZE] = {/* данные */};
//     uint8_t output[BUF_SIZE];
//     size_t output_len;
    
//     esp_err_t ret = deStaffProcess(input, output, sizeof(input), sizeof(output), &output_len);
    
//     if (ret == ESP_OK) {
//         ESP_LOGI("TAG", "deStaff Processed %d bytes", output_len);
//     } else {
//         ESP_LOGE("TAG", "deStaff Error: %s", esp_err_to_name(ret));
//     }
// }

