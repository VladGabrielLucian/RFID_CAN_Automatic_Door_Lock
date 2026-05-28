/*
 * rc522_module_header.h
 *
 *  Created on: Apr 18, 2026
 *      Author: TestUser
 */

#ifndef SRC_RC522_MODULE_HEADER_H_
#define SRC_RC522_MODULE_HEADER_H_

#define VersionReg 0x37

#define PICC_REQIDL 0x26
#define PCD_TRANSCEIVE 0x0C

#include "stm32f1xx_hal.h"


uint8_t MFRC522_ReadRegister(uint8_t addr);
void MFRC522_WriteRegister(uint8_t addr, uint8_t data);
void MFRC522_AntennaON();
void MFRC522_Init();
void MFRC522_Transmit(uint8_t *data, uint8_t length);
uint8_t MFRC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint16_t *backLen);
uint8_t MFRC522_Request(uint8_t reqMode, uint8_t *tagType);
uint8_t MFRC522_Anticoll(uint8_t *UID);

#endif /* SRC_RC522_MODULE_HEADER_H_ */
