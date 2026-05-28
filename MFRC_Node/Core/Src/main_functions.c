/*
 * main_functions.c
 *
 *  Created on: May 12, 2026
 *      Author: TestUser
 */

#include "main_functions.h"
#include "main.h"
#include "rc522_module_header.h"
extern CAN_HandleTypeDef hcan;

static uint32_t last_keepalive_tick = 0;
static uint8_t card_type[2];
static uint8_t card_uid[5];
static uint8_t rfid_status;

void CAN_Init(void){

	CAN_FilterTypeDef canfilterconfig;

	canfilterconfig.FilterActivation = ENABLE;
	canfilterconfig.FilterBank = 0; 	//which filter bank to use from the assigned ones
	canfilterconfig.FilterFIFOAssignment = CAN_RX_FIFO0;
	canfilterconfig.FilterIdHigh = 0x100<<5;
	canfilterconfig.FilterIdLow = 0;
	canfilterconfig.FilterMaskIdHigh = 0x7FF<<5;	//define which ID bits to be compared (universal mask)
	canfilterconfig.FilterMaskIdLow = 0x0000;
	canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK;
	canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;
	canfilterconfig.SlaveStartFilterBank = 14;

	HAL_CAN_ConfigFilter(&hcan, &canfilterconfig);
	HAL_CAN_Start(&hcan);
	HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
}

void CAN_Network_Start(void){
	if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK){
		Error_Handler();
	}
}

uint8_t CAN_Transmit_Message(uint32_t stdId, uint8_t *pData, uint8_t length){
	CAN_TxHeaderTypeDef TxHeader;
	uint8_t TxData[8] = {0};
	uint32_t TxMailbox;

	TxHeader.StdId = stdId;
	TxHeader.RTR = CAN_RTR_DATA;
	TxHeader.IDE = CAN_ID_STD;
	TxHeader.DLC = length;
	TxHeader.TransmitGlobalTime = DISABLE;

	for(uint8_t i = 0; i < length; i++){
		TxData[i] = pData[i];
	}

	if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) == HAL_OK){
		return 0;
	}
	return 1;
}

void Reader_Manage_KeepAlive(uint8_t rfid_version){
	uint8_t status_byte;

	if((HAL_GetTick()-last_keepalive_tick) >= 10000){
		if(rfid_version == 0x82 || rfid_version == 0x92){
			status_byte = 0x00;
		}else{
			status_byte = 0xFF;
		}
		CAN_Transmit_Message(CAN_ID_READER_STATUS, &status_byte, 1);
		last_keepalive_tick = HAL_GetTick();
	}

}

void Reader_RFID(void){
	rfid_status = MFRC522_Request(PICC_REQIDL, card_type);
	if(rfid_status == 0){
		rfid_status = MFRC522_Anticoll(card_uid);
		if(rfid_status == 0){
			CAN_Transmit_Message(CAN_ID_READER_UID, card_uid, 5);
			HAL_Delay(1500);
		}
	}
}
/**
 * @brief: function which separates the response_types
 * @param response_type: decision variable
 */
void Reader_Notification_Feedback(uint8_t response_type){
	switch(response_type){
	case 0x01:
		//3 short beeps
		HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_SET);
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
		HAL_Delay(80);
		HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
		HAL_Delay(80);

		HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_SET);
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
		HAL_Delay(80);
		HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
		HAL_Delay(80);

		HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_SET);
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
		HAL_Delay(80);
		HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
		HAL_Delay(80);

		//2 long beeps
		HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_SET);
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
		HAL_Delay(300);
		HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
		HAL_Delay(300);

		HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_SET);
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
		HAL_Delay(300);
		HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
		break;

	case 0x02:
		HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN, GPIO_PIN_SET);
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
		HAL_Delay(1000);
		HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
	break;

	default:
		HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
	}
}

/**
 * @brief Automatically executed callback when a CAN message received in FIFO0
 * @param hcan: pointer to CAN struct
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypedDef *hcan){
	CAN_RxHeaderTypeDef RxHeader;
	uint8_t RxData[8];

	if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK){
		if(RxHeader,StdId == CAN_ID_CENTRAL_CMD){
			Reader_Notification_Feedback(RxData[0]);
		}
	}
}
