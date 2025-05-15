/*
 * PollingRoutine.c
 *
 *  Created on: Oct 24, 2023
 *      Author: karl.yamashita
 *
 *
 *      Template for projects.
 *
 *      The object of this PollingRoutine.c/h files is to not have to write code in main.c which already has a lot of generated code.
 *      It is cumbersome having to scroll through all the generated code for your own code and having to find a USER CODE section so your code is not erased when CubeMX re-generates code.
 *      
 *      Direction: Call PollingInit before the main while loop. Call PollingRoutine from within the main while loop
 * 
 *      Example;
        // USER CODE BEGIN WHILE
        PollingInit();
        while (1)
        {
            PollingRoutine();
            // USER CODE END WHILE

            // USER CODE BEGIN 3
        }
        // USER CODE END 3

 */


#include "main.h"


extern FDCAN_HandleTypeDef hfdcan1;
extern UART_HandleTypeDef huart2;

extern TimerCallbackStruct timerCallback;


#define SEND_AMOUNT 5 // the amount of messages to send per for loop.


FDCAN_Data_t fdcan1_msg =
{
	.fdcan = &hfdcan1,
	.rxQueueSize = FDCAN_RX_QUEUE_SIZE,
	.txQueueSize = FDCAN_TX_QUEUE_SIZE
};

UART_DMA_QueueStruct uart2_msg =
{
	.huart = &huart2,
	.rx.queueSize = UART_DMA_QUEUE_SIZE,
	.tx.queueSize = UART_DMA_QUEUE_SIZE
};

void PollingInit(void)
{
	TimerCallbackRegisterOnly(&timerCallback, LED_Toggle);
	TimerCallbackTimerStart(&timerCallback, LED_Toggle, 500, TIMER_REPEAT);

	TimerCallbackRegisterOnly(&timerCallback, FDCAN1_TxMessage);
	TimerCallbackTimerStart(&timerCallback, FDCAN1_TxMessage, 1000, TIMER_REPEAT);

	FDCAN_Filter_Init();
	FDCAN_Config(&fdcan1_msg);

	UART_DMA_NotifyUser(&uart2_msg, "STM32 Ready", strlen("STM32 Ready"), true);
}

void PollingRoutine(void)
{
	TimerCallbackCheck(&timerCallback);

	FDCAN1_Parse(&fdcan1_msg);
}

void FDCAN1_Parse(FDCAN_Data_t *msg)
{
	char str[UART_DMA_DATA_SIZE] = {0};
	char str2[8] = {0};
	int i;

	if(FDCAN_RxMsgRdy(msg))
	{
		if(msg->msgToParse->rxHeader.IdType == FDCAN_STANDARD_ID)
		{
			if(msg->msgToParse->rxHeader.Identifier == 0x321)
			{
				sprintf(str, "DDM ID=0x%lX, dlc = %ld, ", msg->msgToParse->rxHeader.Identifier, msg->msgToParse->rxHeader.DataLength);
			}
			else if(msg->msgToParse->rxHeader.Identifier == 0x101)
			{
				sprintf(str, "ONS ID=0x%lX, dlc = %ld, ", msg->msgToParse->rxHeader.Identifier, msg->msgToParse->rxHeader.DataLength);
			}
			else if(msg->msgToParse->rxHeader.Identifier == 0x2AC)
			{
				sprintf(str, "IPC ID=0x%lX, dlc = %ld, ", msg->msgToParse->rxHeader.Identifier, msg->msgToParse->rxHeader.DataLength);
			}
			else if(msg->msgToParse->rxHeader.Identifier == 0x200)
			{
				sprintf(str, "We should not see this message as ID 0x200 does not match filter(s)");
			}
			else
			{
				return;
			}
		}
		else // EXTENDED ID
		{
			if(msg->msgToParse->rxHeader.Identifier == 0x10044080)
			{
				sprintf(str, "BCM ID=0x%lX, dlc = %ld, ", msg->msgToParse->rxHeader.Identifier, msg->msgToParse->rxHeader.DataLength);
			}
			else
			{
				return;
			}
		}
		// get data
		for(i = 0; i < msg->msgToParse->rxHeader.DataLength; i++)
		{
			sprintf(str2, "0x%X ", msg->msgToParse->rxData.data[i]);
			strcat(str, str2);
		}

		UART_DMA_NotifyUser(&uart2_msg, str, strlen(str), true);
	}
}

/*
 * Description: Transmit ID 0x321, 0x2AC and 0x101 ten times with data[0] incrementing from 0 and data[1] = 0xAA, 0xBB and so on.
 *
 * 				This demonstrates how we can add many messages to a queue and transmit them all without missing a beat.
 *
 * 				However, when not using a queue, if all the transmit buffers are full, then you won't be able to send the message.
 * 				You can check to see if the transmit buffer is full but then what do you do with the message you couldn't send?
 * 				In the end, you still need to do something with the unsent message, so might as well put it a queue.
 *
 * 				Of course in the real world, you're only going to be sending messages periodically. So a small
 * 				queue size of maybe 4-6 should not be an issue.
 */
void FDCAN1_TxMessage(void)
{
	FDCAN_Tx_t txMsg = {0};
	int i;

	// send a non matching ID 0x200 to show the filter works
	txMsg.txHeader.Identifier = 0x200;
	txMsg.txHeader.DataLength = 2;
	txMsg.txHeader.IdType = FDCAN_STANDARD_ID;
	txMsg.txHeader.TxFrameType = FDCAN_DATA_FRAME;
	txMsg.txHeader.FDFormat = FDCAN_CLASSIC_CAN;
	txMsg.txHeader.TxEventFifoControl = FDCAN_TX_EVENT;
	txMsg.txHeader.MessageMarker = 0;
	txMsg.txData.data[0] = 0xAA;
	txMsg.txData.data[1] = 0xCC;
	FDCAN_Add_TxQueue(&fdcan1_msg, &txMsg); // add to queue

	// now we will send ID's that match the filters

	// send ID 0x321
	txMsg.txHeader.Identifier = 0x321;
	txMsg.txHeader.DataLength = 2;
	txMsg.txHeader.IdType = FDCAN_STANDARD_ID;
	txMsg.txHeader.TxFrameType = FDCAN_DATA_FRAME;
	txMsg.txHeader.FDFormat = FDCAN_CLASSIC_CAN;
	txMsg.txHeader.MessageMarker = 0;
	txMsg.txData.data[1] = 0xAA; // 0xAA
	for(i = 0; i < SEND_AMOUNT; i++)
	{
		txMsg.txData.data[0] = i;
		FDCAN_Add_TxQueue(&fdcan1_msg, &txMsg); // add to queue
	}

	// send ID 0x101
	txMsg.txHeader.Identifier = 0x101;
	txMsg.txHeader.DataLength = 2;
	txMsg.txHeader.IdType = FDCAN_STANDARD_ID;
	txMsg.txHeader.TxFrameType = FDCAN_DATA_FRAME;
	txMsg.txHeader.FDFormat = FDCAN_CLASSIC_CAN;
	txMsg.txHeader.MessageMarker = 0;
	txMsg.txData.data[1] = 0xBB; // 0xBB
	for(i = 0; i < SEND_AMOUNT; i++)
	{
		txMsg.txData.data[0] = i;
		FDCAN_Add_TxQueue(&fdcan1_msg, &txMsg); // add to queue
	}

	// send ID 0x2AC
	txMsg.txHeader.Identifier = 0x2AC;
	txMsg.txHeader.DataLength = 2;
	txMsg.txHeader.IdType = FDCAN_STANDARD_ID;
	txMsg.txHeader.TxFrameType = FDCAN_DATA_FRAME;
	txMsg.txHeader.FDFormat = FDCAN_CLASSIC_CAN;
	txMsg.txHeader.MessageMarker = 0;
	txMsg.txData.data[1] = 0xCC; // 0xCC
	for(i = 0; i < SEND_AMOUNT; i++)
	{
		txMsg.txData.data[0] = i;
		FDCAN_Add_TxQueue(&fdcan1_msg, &txMsg); // add to queue
	}

	// send ID 0x10044080
	txMsg.txHeader.Identifier = 0x10044080;
	txMsg.txHeader.DataLength = 2;
	txMsg.txHeader.IdType = FDCAN_EXTENDED_ID;
	txMsg.txHeader.TxFrameType = FDCAN_DATA_FRAME;
	txMsg.txHeader.FDFormat = FDCAN_CLASSIC_CAN;
	txMsg.txHeader.TxEventFifoControl =
	txMsg.txHeader.MessageMarker = 0;
	txMsg.txData.data[1] = 0xDD; // 0xCC
	for(i = 0; i < SEND_AMOUNT; i++)
	{
		txMsg.txData.data[0] = i;
		FDCAN_Add_TxQueue(&fdcan1_msg, &txMsg); // add to queue
	}
}

/*
 * Description: Configure filter(s) for ID 0x0321, 0x2AC, 0x101 and 0x10044080
 * 				Note: Std Filters Nbr = 28, Ext Filters Nbr = 8
 */
void FDCAN_Filter_Init()
{
	FDCAN_FilterTypeDef sFilterConfig;

	/* Configure Rx filter for ID 0x321*/
	sFilterConfig.IdType = FDCAN_STANDARD_ID;
	sFilterConfig.FilterIndex = 0; // increment index as more STD ID's are added
	sFilterConfig.FilterType = FDCAN_FILTER_MASK;
	sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	sFilterConfig.FilterID1 = 0x321;
	sFilterConfig.FilterID2 = 0x7FF;
	FDCAN_Filter(&fdcan1_msg, &sFilterConfig);

	/* Configure Rx filter for ID 0x101*/
	sFilterConfig.IdType = FDCAN_STANDARD_ID;
	sFilterConfig.FilterIndex = 1;
	sFilterConfig.FilterType = FDCAN_FILTER_MASK;
	sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	sFilterConfig.FilterID1 = 0x101;
	sFilterConfig.FilterID2 = 0x7FF;
	FDCAN_Filter(&fdcan1_msg, &sFilterConfig);

	/* Configure Rx filter for ID 0x2AC*/
	sFilterConfig.IdType = FDCAN_STANDARD_ID;
	sFilterConfig.FilterIndex = 2;
	sFilterConfig.FilterType = FDCAN_FILTER_MASK;
	sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	sFilterConfig.FilterID1 = 0x2AC;
	sFilterConfig.FilterID2 = 0x7FF;
	FDCAN_Filter(&fdcan1_msg, &sFilterConfig);

	/* Configure Rx filter for ID 0x101*/
	sFilterConfig.IdType = FDCAN_EXTENDED_ID;
	sFilterConfig.FilterIndex = 0; // first EXT ID, so we can start at zero
	sFilterConfig.FilterType = FDCAN_FILTER_MASK;
	sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	sFilterConfig.FilterID1 = 0x10044080;
	sFilterConfig.FilterID2 = 0x1FFFFFFF;
	FDCAN_Filter(&fdcan1_msg, &sFilterConfig);
}

void LED_Toggle(void)
{
	HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
}


// HAL callbacks

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if(huart == uart2_msg.huart)
	{
		RingBuff_Ptr_Input(&uart2_msg.rx.ptr, uart2_msg.rx.queueSize);
		UART_DMA_EnableRxInterrupt(&uart2_msg);
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == uart2_msg.huart)
	{
		uart2_msg.tx.txPending = false;
		UART_DMA_SendMessage(&uart2_msg);
	}
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
	FDCAN_Rx_t *ptr;

	if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
	{
		ptr = &fdcan1_msg.rxQueue[fdcan1_msg.rx_ptr.index_IN];

		/* Retrieve Rx messages from RX FIFO0 and save into queue */
		if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &ptr->rxHeader, ptr->rxData.data) == HAL_OK)
		{
			RingBuff_Ptr_Input(&fdcan1_msg.rx_ptr, fdcan1_msg.rxQueueSize);
		}
	}
}

void HAL_FDCAN_TxFifoEmptyCallback(FDCAN_HandleTypeDef *hfdcan)
{
	if(hfdcan == &hfdcan1)
	{
		FDCAN_Tx_Send(&fdcan1_msg); // send anymore pending messages in queue
	}
}
