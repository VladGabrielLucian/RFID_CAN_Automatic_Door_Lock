/*
 * main_functions.h
 *
 *  Created on: May 12, 2026
 *      Author: TestUser
 */

#ifndef SRC_MAIN_FUNCTIONS_H_
#define SRC_MAIN_FUNCTIONS_H_

#define CAN_ID_READER_UID 		0x100
#define CAN_ID_READER_STATUS	0x105
#define CAN_ID_CENTRAL_CMD		0x200

#define GREEN_LED_PORT GPIOC
#define GREEN_LED_PIN GPIO_PIN_13

#define RED_LED_PORT GPIOA
#define RED_LED_PIN GPIO_PIN_0

#define BUZZER_PORT GPIOA
#define BUZZER_PIN GPIO_PIN_1

#include "main.h"
#include "stm32f1xx_hal.h"

void CAN_Init(void);
uint8_t CAN_Transmit_Message(uint32_t stdId, uint8_t *pData, uint8_t length);
void Reader_RFID(void);
void Reader_Manage_KeepAlive(void);
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);

void Reader_Feedback(uint8_t command);
void OK_UID_Tone(uint32_t duration);
void BAD_UID_Tone(uint32_t duration);
void ALARM(void);

#endif /* SRC_MAIN_FUNCTIONS_H_ */
