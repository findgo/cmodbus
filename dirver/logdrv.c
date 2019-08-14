
#include "stdint.h"
#include "stdlib.h"
#include <stdio.h>

#include "mbcpu.h"
#include "log.h"
#include "stm32f10x.h"
#include "stm32f10x_it.h"

#define USART_LOG_BANDRATE  115200
#define LOG_TX_MAX_SIZE     255

// for usart 3
#define USART_LOG                    USART3
#define USART_LOG_IRQ                    USART3_IRQn

#define USART_LOG_TX_PORT                GPIOB
#define USART_LOG_TX_PIN                GPIO_Pin_10

#define USART_LOG_RX_PORT                GPIOB
#define USART_LOG_RX_PIN                GPIO_Pin_11

#define USART_LOG_GPIO_PeriphClock_EN()  do{ RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE); }while(0)
#define USART_LOG_PeriphClock_EN()        do{ RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);}while(0)

typedef struct {
    uint8_t *ptxbuf;
    uint16_t txHead;
    uint16_t txTail;
    uint16_t txcount;
    uint16_t txsize;
} logcfg_t;

static uint16_t Serial_WriteByte(logcfg_t *cfg, uint8_t dat);

static uint16_t SerialTxPop(logcfg_t *cfg, uint8_t *dat);

static uint16_t SerialTxPut(logcfg_t *cfg, uint8_t dat);

static uint8_t logTxBuf[LOG_TX_MAX_SIZE];
static logcfg_t logcfg = {&logTxBuf[0], 0, 0, 0, LOG_TX_MAX_SIZE}; // serial����ʵ��

void logInit(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    USART_ClockInitTypeDef USART_ClockInitStructure;

    USART_LOG_GPIO_PeriphClock_EN();
    USART_LOG_PeriphClock_EN();

    //PB10 -- USAR1 TX, PB11 -- USART1 RX
    GPIO_InitStructure.GPIO_Pin = USART_LOG_TX_PIN;//TX AF mode
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(USART_LOG_TX_PORT, &GPIO_InitStructure);

//    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
//    GPIO_InitStructure.GPIO_Pin   = USART_LOG_RX_PIN;//RX AF mode
//    GPIO_Init(USART_LOG_RX_PORT, &GPIO_InitStructure);


    NVIC_InitStructure.NVIC_IRQChannel = USART_LOG_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ClockStructInit(&USART_ClockInitStructure);
    USART_ClockInit(USART_LOG, &USART_ClockInitStructure);

    USART_InitStructure.USART_BaudRate = USART_LOG_BANDRATE;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_HardwareFlowControl = DISABLE;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;// 8 bit
    USART_Init(USART_LOG, &USART_InitStructure);

    USART_ClearFlag(USART_LOG, USART_FLAG_TXE | USART_IT_RXNE | USART_FLAG_TC);
    USART_ITConfig(USART_LOG, USART_IT_TXE, DISABLE);
    USART_ITConfig(USART_LOG, USART_IT_TC, DISABLE);
    USART_ITConfig(USART_LOG, USART_IT_RXNE, DISABLE);

    USART_Cmd(USART_LOG, ENABLE);//enable USART3

    mo_log_set_max_logger_level(LOG_LEVEL_DEBUG);
}

/* 重定向fputc 到输出，单片机一般为串口*/
int fputc(int ch, FILE *f) {
    /* e.g. write a character to the USART */
    (void) Serial_WriteByte(&logcfg, ch);

    return ch;
}

//���ͻ����������ֽ���
#define SERIAL_TX_IDLE_AVAIL(ptr)   (ptr->txsize - ptr->txcount)
//���ͻ�������Ч����
#define SERIAL_TX_VALID_AVAIL(ptr)  (ptr->txcount)
//���ͻ������Ƿ��пɶ�����
#define IS_SERIAL_TX_VALID(ptr) (ptr->txcount > 0)

static uint16_t Serial_WriteByte(logcfg_t *cfg, uint8_t dat) {
    uint16_t count;

    ENTER_CRITICAL_SECTION();
    count = SerialTxPut(cfg, dat);

    //���÷�������ж� has some bug
    if (IS_SERIAL_TX_VALID(cfg)) {
        USART_ITConfig(USART_LOG, USART_IT_TXE, ENABLE);  // enable tx ie
    }

    EXIT_CRITICAL_SECTION();

    return count;
}

static uint16_t SerialTxPop(logcfg_t *cfg, uint8_t *dat) {
    if (IS_SERIAL_TX_VALID(cfg)) {//是否有有效数据
        *dat = cfg->ptxbuf[cfg->txHead];
        cfg->txcount--;
        if (++cfg->txHead >= cfg->txsize) {
            cfg->txHead = 0;
        }
        return 1;
    }

    return 0;
}

static uint16_t SerialTxPut(logcfg_t *cfg, uint8_t dat) {
    if (SERIAL_TX_IDLE_AVAIL(cfg) < 1)
        return 0;

    cfg->ptxbuf[cfg->txTail] = dat;
    cfg->txcount++;
    if (++cfg->txTail >= cfg->txsize) {
        cfg->txTail = 0;
    }

    return 1;
}

void LOG_TXE_Isr_callback(void) {
    uint8_t temp;

    ENTER_CRITICAL_SECTION();

    if (SerialTxPop(&logcfg, &temp)) {
        USART_SendData(USART_LOG, temp);
    } else {
        USART_ITConfig(USART_LOG, USART_IT_TXE, DISABLE);
    }
    EXIT_CRITICAL_SECTION();
}

void USART3_IRQHandler(void) {
    if (USART_GetITStatus(USART3, USART_IT_TXE) != RESET) {
        LOG_TXE_Isr_callback();
        USART_ClearITPendingBit(USART3, USART_IT_TXE);
    }

    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }

    if (USART_GetITStatus(USART3, USART_IT_TC) != RESET) {
        USART_ClearITPendingBit(USART3, USART_IT_TC);
    }
    NVIC_ClearPendingIRQ(USART3_IRQn);
}

