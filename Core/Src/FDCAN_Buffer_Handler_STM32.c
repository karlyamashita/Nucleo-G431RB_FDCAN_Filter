/*
 * FDCAN_Buffer_Handler_STM32.c
 *
 *  Created on: Sep 23, 2024
 *      Author: karl.yamashita
 */

#include "main.h"

/*
 * Description: Initiate CAN filters.
 */
void FDCAN_Filter(FDCAN_Data_t *msg, FDCAN_FilterTypeDef *sFilterConfig)
{
	HAL_StatusTypeDef hal_status;

	hal_status = HAL_FDCAN_ConfigFilter(msg->fdcan, sFilterConfig);
	if(hal_status != HAL_OK)
	{
		Error_Handler();
	}
}

void FDCAN_Config(FDCAN_Data_t *msg)
{
	HAL_StatusTypeDef hal_status;

	if (HAL_FDCAN_ConfigGlobalFilter(msg->fdcan, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE) != HAL_OK)
	{
		Error_Handler();
	}

	hal_status = HAL_FDCAN_Start(msg->fdcan);
	if(hal_status != HAL_OK)
	{
		Error_Handler();
	}

	// only need FIFO0 received activate. Enable Transmit FIFO empty interrupt.
	hal_status = HAL_FDCAN_ActivateNotification(msg->fdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_TX_FIFO_EMPTY, 0);
	if(hal_status != HAL_OK)
	{
		Error_Handler();
	}
}

bool FDCAN_RxMsgRdy(FDCAN_Data_t *msg)
{
	if(msg->rx_ptr.cnt_Handle)
	{
		msg->msgToParse = &msg->rxQueue[msg->rx_ptr.index_OUT];
		RingBuff_Ptr_Output(&msg->rx_ptr, msg->rxQueueSize);

		return true;
	}

	return false;
}

void FDCAN_Add_TxQueue(FDCAN_Data_t *msg, FDCAN_Tx_t *tx)
{
	FDCAN_Tx_t *ptr;

	ptr = &msg->txQueue[msg->tx_ptr.index_IN];

	memcpy(&ptr->txHeader, &tx->txHeader, sizeof(tx->txHeader));
	memcpy(&ptr->txData, &tx->txData, sizeof(tx->txData));

	RingBuff_Ptr_Input(&msg->tx_ptr, msg->txQueueSize);

	FDCAN_Tx_Send(msg);
}

void FDCAN_Tx_Send(FDCAN_Data_t *msg)
{
	FDCAN_Tx_t *ptr;

	if(msg->tx_ptr.cnt_Handle)
	{
		ptr = &msg->txQueue[msg->tx_ptr.index_OUT];
		msg->msgToSend = ptr;

		if(HAL_FDCAN_AddMessageToTxFifoQ(msg->fdcan, &msg->msgToSend->txHeader, msg->msgToSend->txData.data) == HAL_OK)
		{
			RingBuff_Ptr_Output(&msg->tx_ptr, msg->txQueueSize);
		}
	}
}
