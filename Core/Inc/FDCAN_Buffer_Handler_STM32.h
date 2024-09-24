/*
 * FDCAN_Buffer_Handler_STM32.h
 *
 *  Created on: Sep 23, 2024
 *      Author: karl.yamashita
 */

#ifndef INC_FDCAN_BUFFER_HANDLER_STM32_H_
#define INC_FDCAN_BUFFER_HANDLER_STM32_H_


#define FDCAN_RX_QUEUE_SIZE 32
#define FDCAN_TX_QUEUE_SIZE 32

typedef struct
{
	uint8_t data[64];
}DataBuffer_t;

typedef struct
{
	FDCAN_RxHeaderTypeDef rxHeader;
	DataBuffer_t rxData;
}FDCAN_Rx_t;

typedef struct
{
	FDCAN_TxHeaderTypeDef txHeader;
	DataBuffer_t txData;
}FDCAN_Tx_t;

typedef struct
{
	FDCAN_HandleTypeDef *fdcan;

	FDCAN_Rx_t rxQueue[FDCAN_RX_QUEUE_SIZE];
	FDCAN_Rx_t *msgToParse;
	RING_BUFF_STRUCT rx_ptr;
	uint32_t rxQueueSize;

	FDCAN_Tx_t txQueue[FDCAN_TX_QUEUE_SIZE];
	FDCAN_Tx_t *msgToSend;
	RING_BUFF_STRUCT tx_ptr;
	uint32_t txQueueSize;
}FDCAN_Data_t;


void FDCAN_Filter(FDCAN_Data_t *msg, FDCAN_FilterTypeDef *sFilterConfig);
void FDCAN_Config(FDCAN_Data_t *msg);

bool FDCAN_RxMsgRdy(FDCAN_Data_t *msg);
void FDCAN_Add_TxQueue(FDCAN_Data_t *msg, FDCAN_Tx_t *tx);
void FDCAN_Tx_Send(FDCAN_Data_t *msg);

#endif /* INC_FDCAN_BUFFER_HANDLER_STM32_H_ */
