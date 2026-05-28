/*
 * main_functions.h
 *
 *  Created on: May 28, 2026
 *      Author: TestUser
 */

#ifndef SRC_MAIN_FUNCTIONS_H_
#define SRC_MAIN_FUNCTIONS_H_

#include "stm32f1xx_hal.h"

#define CAN_ID_READER_UID 		0x100
#define CAN_ID_READER_STATUS	0x105
#define CAN_ID_CENTRAL_CMD		0x200

#define LOCK_PORT	GPIOC
#define	LOCK_PIN	GPIO_PIN_13

#endif /* SRC_MAIN_FUNCTIONS_H_ */
