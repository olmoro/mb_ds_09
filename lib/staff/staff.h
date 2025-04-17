/*=====================================================================================
 * Description:
 *    ESP32
 *====================================================================================*/
#ifndef _STAFF_H_
#define _STAFF_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"


/* Параметры функции:
.
*/

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Побайтное копирование со стаффингом.
 *
 * @param src: исходный буфер.
 * @param dest: целевой буфер.
 * @param src_len: длина исходного буфера.
 * @param dest_max_len: максимально допустимая длина целевого буфера.
 * @param dest_actual_len: указатель для возврата фактической длины данных после обработки
 *
 * @return
 *        true: данные успешно обработаны и записаны.
 *        false: недостаточно места в целевом буфере.
 */
bool staffProcess(const uint8_t *src, uint8_t *dest, size_t src_len, size_t dest_max_len, size_t *dest_actual_len);
//bool deStaffProcess(const uint8_t *src, uint8_t *dest, size_t src_len, size_t dest_max_len, size_t *dest_actual_len);
esp_err_t deStaffProcess(const uint8_t *src, uint8_t *dest, size_t src_size, size_t dest_size, size_t *dest_len); 

#ifdef __cplusplus
}
#endif

#endif  // _STAFF_H_
