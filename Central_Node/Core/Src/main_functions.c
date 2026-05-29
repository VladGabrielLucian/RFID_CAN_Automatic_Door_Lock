/*
 * main_functions.c
 *
 *  Created on: May 28, 2026
 *      Author: TestUser
 */


#include "main_functions.h"

extern CAN_HandleTypeDef hcan;

/**
 * @brief:	CAN filter configuration. This function sets up the main CAN filter parameters
 */
void CAN_Init(void){
	CAN_FilterTypeDef canFilterConfig;

	canFilterConfig.SlaveStartFilterBank = 14;

	canFilterConfig.FilterActivation = ENABLE;
	canFilterConfig.FilterBank = 0;
	canFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;

	/*	Filter based on the ID 0x100	*/
	canFilterConfig.FilterIdHigh = (0x100 << 5);
	canFilterConfig.FilterIdLow = 0x000;

	/* filter mask only for IDs 0x100 and 0x105 from the MFRC Node	*/
	canFilterConfig.FilterMaskIdHigh = (0x7FA << 5);
	canFilterConfig.FilterMaskIdLow = 0x0004;

	canFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	canFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;

	HAL_CAN_ConfigFilter(&hcan, &canFilterConfig);

	HAL_CAN_Start(&hcan);
}

/**
 * @brief:	polls the CAN receive FIFO, decodes received messages on the CAN Bus and manages the peripheral output states
 */
void Central_Process_Messages(void){
	  CAN_TxHeaderTypeDef TxHeader;
	  uint8_t TxData[1];
	  uint32_t TxMailbox;
	  /* header configuration for response packages sent to the MFRC Node	*/
	  TxHeader.StdId = 0x200;
	  TxHeader.RTR = CAN_RTR_DATA;
	  TxHeader.IDE = CAN_ID_STD;
	  TxHeader.DLC = 1;
	  TxHeader.TransmitGlobalTime = DISABLE;

	  CAN_RxHeaderTypeDef RxHeader;
	  uint8_t RxData[8];

	  /*	Checking if there are any messages in the FIFO0 buffer	*/
	  if(HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0)>0){
		  if(HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK){
			  switch (RxHeader.StdId){
			  	  case 0x100:							// access granted code
			  		  if(RxHeader.DLC == 5 &&			//validating UID
			  				  RxData[0] == 161 &&
							  RxData[1] == 19 &&
							  RxData[2] == 50 &&
							  RxData[3] == 7 &&
							  RxData[4] == 135){
			  			  TxData[0] = 0x01;
			  			  HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox);

			  			  HAL_Delay(1500);
			  			  HAL_GPIO_WritePin(LOCK_PORT, LOCK_PIN, GPIO_PIN_RESET);
			  			  HAL_Delay(4500);
			  			  HAL_GPIO_WritePin(LOCK_PORT, LOCK_PIN, GPIO_PIN_SET);

			  			  /*	emptying residual packages received in FIFO0	*/
			  			  while(HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) > 0){
			  				  HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &RxHeader, RxData);
			  			  }
			  		  }else{
			  			  TxData[0] = 0x02;		// access denied code
			  			  HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox);
			  		  }
			  	  break;

			  	  case 0x105:
			  		  if(RxData[0] == 0xFF){
			  			  TxData[0] = 0x03;		// alarm code
			  			  HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox);
			  		  }
			  	  break;

			  	  default:
			  		break;
			  }
		  }
	  }
}
