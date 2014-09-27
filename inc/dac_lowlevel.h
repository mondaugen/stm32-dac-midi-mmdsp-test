#ifndef DAC_LOWLEVEL_H
#define DAC_LOWLEVEL_H 

#include "stm32f4xx.h"

#define DAC_DMA_BUF_SIZE    64
#define DAC_SAMPLE_RATE     (22500.0) 
#define DAC_TIM_PERIOD      2000 

void DAC_Setup(void);
#define DAC_INTERRUPT() \
{ \
    extern uint16_t *curDMAData; \
    extern uint16_t dacDMAData[]; \
    if (DMA_GetITStatus(DMA1_Stream5, DMA_IT_TCIF5)) { \
        DMA_ClearITPendingBit(DMA1_Stream5, DMA_IT_TCIF5); \
        curDMAData = dacDMAData + DAC_DMA_BUF_SIZE; \
    } \
    if (DMA_GetITStatus(DMA1_Stream5, DMA_IT_HTIF5)) { \
        DMA_ClearITPendingBit(DMA1_Stream5, DMA_IT_HTIF5); \
        curDMAData = dacDMAData; \
    } \
    NVIC_ClearPendingIRQ(DMA1_Stream0_IRQn); \
}

#endif /* DAC_LOWLEVEL_H */
