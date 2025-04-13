/*=====================================================================================
 * Description:
 *  
 *  
 *  
 *====================================================================================*/
#ifndef _PROCESSOR_H_
#define _PROCESSOR_H_



#ifdef __cplusplus
extern "C"
{
#endif

void uart_sp_init();

void frame_processor_task(void *arg);


#ifdef __cplusplus
}
#endif

#endif // _PROCESSOR_H_
