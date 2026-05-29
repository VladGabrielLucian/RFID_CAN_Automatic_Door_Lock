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
extern TIM_HandleTypeDef htim2;

static uint32_t last_keepalive_tick = 0;
static uint8_t card_type[2];
static uint8_t card_uid[5];
static uint8_t rfid_status;
volatile uint8_t uid_response_flag = 0;

/**
 * @brief: 	CAN filter configuration. This function sets up the main CAN filter parameters and the IDs
 * 		   	to look for on the bus.
 */
void CAN_Init(void){

	CAN_FilterTypeDef canfilterconfig;

	canfilterconfig.FilterActivation = ENABLE;
	canfilterconfig.FilterBank = 0; 	//which filter bank to use from the assigned ones
	canfilterconfig.FilterFIFOAssignment = CAN_RX_FIFO0;

	//Filter configuration only for Central Node with ID 0x200
	canfilterconfig.FilterIdHigh = (CAN_ID_CENTRAL_CMD << 5);	//the ID is mapped into a 32 bit filter, therefore it needs to be shifted
	canfilterconfig.FilterIdLow = 0x0000;

	canfilterconfig.FilterMaskIdHigh = (0x7FF << 5);	//define which ID bits to be compared (all the 11 bits of the ID + IDE bit)
	canfilterconfig.FilterMaskIdLow = 0x0004;			//standard package
	canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK;
	canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;
	canfilterconfig.SlaveStartFilterBank = 14;

	HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

	HAL_CAN_ConfigFilter(&hcan, &canfilterconfig);
	HAL_CAN_Start(&hcan);
	HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
}

/**
 * @brief: 			function that packages and transmits a standard CAN message frame
 * @param stdId:	Standard Identifier (11-bit ID)
 * @param pData:	pointer to the data payload buffer
 * @param length:	length of the message
 * @retval:			0 on success, 1 if transmission failed
 */
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

/**
 * @brief:	function that evaluates connectivity with the MFRC module by using a 10-seconds non-blocking timer interval
 * 			transmits on CAN 0x00 (OK) or 0xFF (Fault), depending on the integrity of the data read from VersionReg
 */
void Reader_Manage_KeepAlive(void){
	if((HAL_GetTick() - last_keepalive_tick) >= 10000){
		uint8_t status_byte = 0xFF;

		uint8_t version = MFRC522_ReadRegister(VersionReg);

		if(version == 0x82 || version == 0x92){
				status_byte = 0x00;
			}
		CAN_Transmit_Message(CAN_ID_READER_STATUS, &status_byte, 1);
		last_keepalive_tick = HAL_GetTick();
	}
}

/**
 * @breif:	function that executes sequential polling of the MFRC522 IC for active RF field tokens
 * 			also extracts the 5-byte unique UID array and sends it to the Central Node via CAN Bus
 */
void Reader_RFID(void){
	rfid_status = MFRC522_Request(PICC_REQIDL, card_type);
	if(rfid_status == 0){
		rfid_status = MFRC522_Anticoll(card_uid);
		if(rfid_status == 0){
			CAN_Transmit_Message(CAN_ID_READER_UID, card_uid, 5);
			/*	delay in order to avoid overlapping reads of the same card	 */
			HAL_Delay(1500);
		}
	}
}

/**
 * @brief:	 	Automatically executed callback when a CAN message received in FIFO0
 * @param hcan:	pointer to CAN struct \
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan){
	CAN_RxHeaderTypeDef RxHeader;
	uint8_t RxData[8];

	if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK){
		if(RxHeader.StdId == CAN_ID_CENTRAL_CMD){
			uid_response_flag = RxData[0];
		}
	}
}
/**
 * @breif:			controls the feedback logic based on the received CAN message
 * @param command:	command code received on the CAN Bus from the Central Node
 */
void Reader_Feedback(uint8_t command){
	switch(command){
/*	access granted	 */
	case 0x01:
		for (uint8_t i = 0; i<3; i++){
			OK_UID_Tone(80);
			HAL_Delay(50);
		}
		OK_UID_Tone(1200);
	break;
/*	access restricted	 */
	case 0x02:
		BAD_UID_Tone(1600);
	break;
/*	MFRC522 Malfunction	 */
	case 0x03:
		ALARM();
	break;
	}

	uid_response_flag = 0;
}

/**
 * @breif:			function used for visual and audio feedback for correct UID card scanned
 * @param duation:	specifies the ON duration of the feedback (in ms)
 */
void OK_UID_Tone(uint32_t duration){
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
	HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_SET);
	HAL_Delay(duration);
	HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
	HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_RESET);
}

/**
 * @brief:			function used for visual and audio feedback for incorrect UID card scanned
 * @param duration:	specifies the ON duration of the feedback (in ms)
 */
void BAD_UID_Tone(uint32_t duration){
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
	HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN, GPIO_PIN_SET);
	HAL_Delay(duration);
	HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
	HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN, GPIO_PIN_RESET);
}
/**
 * @brief:	function used for visual and audio output in case of complete failure of the MFRC522 module
 */
void ALARM(void){
	for (uint8_t i = 0; i < 20 ; i++){
		HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
		HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN, GPIO_PIN_SET);
		HAL_Delay(200);
		HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
		HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN, GPIO_PIN_RESET);
		HAL_Delay(120);
	}
}
