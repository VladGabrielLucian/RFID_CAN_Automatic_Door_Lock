/*
 * rc522_module_driver.c
 *
 *  Created on: Apr 18, 2026
 *      Author: TestUser
 */

#include "rc522_module_header.h"

extern SPI_HandleTypeDef hspi1;

#define VersionReg 0x37
#define CommandReg 0x01
#define TxControlReg 0x14

#define TModeReg 0x2A
#define TPrescalerReg 0x2B
#define TxASKReg 0x15
#define TReloadRegH 0x2C
#define TReloadRegL 0x2D

#define ComIrqReg 0x04
#define ErrorReg 0x06
#define ModeReg 0x11
#define ControlReg 0x0C

#define FIFODataReg 0x09
#define FIFOLevelReg 0x0A
#define BitFramingReg 0x0D

#define PICC_REQIDL 0x26
#define PCD_TRANSCEIVE 0x0C

#define VERSION_1_0 0x91
#define VERSION_2_0 0x92

#define CS_PORT GPIOA
#define CS_PIN GPIO_PIN_12

#define RST_PORT GPIOB
#define RST_PIN GPIO_PIN_12

#define ComIEnReg 0x02

#define CS_ENABLE() HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET)
#define CS_DISABLE() HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET)


uint8_t MFRC522_ReadRegister(uint8_t addr){
	uint8_t txData[2];
	uint8_t rxData[2];

	txData[0] = ((addr << 1) & 0x7E) | 0x80;
	txData[1] = 0x00;

	CS_ENABLE();
	HAL_SPI_TransmitReceive(&hspi1, txData, rxData, 2, HAL_MAX_DELAY);
	CS_DISABLE();

	return rxData[1];
}

void MFRC522_WriteRegister(uint8_t addr, uint8_t data){
	uint8_t txData[2];

	txData[0] = (addr<<1) & 0x7E;
	txData[1] = data;

	CS_ENABLE();
	HAL_SPI_Transmit(&hspi1, txData, 2, HAL_MAX_DELAY);
	CS_DISABLE();
}

void MFRC522_AntennaON(){
	uint8_t crt_ant_state = MFRC522_ReadRegister(TxControlReg);
	crt_ant_state |= 0x03;
	MFRC522_WriteRegister(TxControlReg, crt_ant_state);
}

void MFRC522_Init(){
	HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_RESET);
	HAL_Delay(50);
	HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_SET);
	HAL_Delay(50);

	CS_DISABLE();

	MFRC522_WriteRegister(CommandReg, 0x0F);

	HAL_Delay(50);

	MFRC522_WriteRegister(TModeReg, 0x8D);
	MFRC522_WriteRegister(TPrescalerReg, 0x3E);

	MFRC522_WriteRegister(TReloadRegL, 30);
	MFRC522_WriteRegister(TReloadRegH, 0);

	MFRC522_WriteRegister(TxASKReg, 0x40);
	MFRC522_WriteRegister(ModeReg, 0x3D);

	MFRC522_AntennaON();
}

void MFRC522_Transmit(uint8_t *data, uint8_t length){
	MFRC522_WriteRegister(CommandReg, 0x00);
	MFRC522_WriteRegister(FIFOLevelReg, 0x80);

	for(uint8_t i = 0; i < length; i++){
		MFRC522_WriteRegister(FIFODataReg, data[i]);
	}

	MFRC522_WriteRegister(CommandReg, 0x0C);

	uint8_t current_framing = MFRC522_ReadRegister(BitFramingReg);
	MFRC522_WriteRegister(BitFramingReg, current_framing | 0x80);
}

uint8_t MFRC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint16_t *backLen){
	uint8_t status = 2;
	uint8_t irqEn = 0x77;
	uint8_t waitIRq = 0x30;
	uint8_t n;

	MFRC522_WriteRegister(ComIEnReg, irqEn | 0x80);
	uint8_t current_irq = MFRC522_ReadRegister(ComIrqReg);
	MFRC522_WriteRegister(ComIrqReg, current_irq & (~0x80));
	MFRC522_WriteRegister(FIFOLevelReg, 0x80);

	MFRC522_WriteRegister(CommandReg, 0x00); // Trecere forțată în Idle

	for (uint8_t i = 0; i < sendLen; i++){
		MFRC522_WriteRegister(FIFODataReg, sendData[i]);
	}

	MFRC522_WriteRegister(CommandReg, command);
	if(command == PCD_TRANSCEIVE){
		uint8_t current_framing = MFRC522_ReadRegister(BitFramingReg);
		MFRC522_WriteRegister(BitFramingReg, current_framing | 0x80);
	}

	uint16_t limit = 2000;
	do {
		n = MFRC522_ReadRegister(ComIrqReg);
		limit--;
	} while((limit != 0) && !(n & 0x01) && !(n & waitIRq));

	uint8_t current_framing = MFRC522_ReadRegister(BitFramingReg);
	MFRC522_WriteRegister(BitFramingReg, current_framing & 0x7F);

	if(limit != 0){
		uint8_t errorRegValue = MFRC522_ReadRegister(ErrorReg);
		if(!(errorRegValue & 0x1B)){
			status = 0;
			if(command == PCD_TRANSCEIVE){
				n = MFRC522_ReadRegister(FIFOLevelReg);
				uint8_t lastBits = MFRC522_ReadRegister(ControlReg) & 0x07;

				if (lastBits) *backLen = (n - 1) * 8 + lastBits;
				else *backLen = n * 8;

				if (n == 0) n = 1;
				if (n > 16) n = 16;

				for (uint8_t i = 0; i < n; i++){
					backData[i] = MFRC522_ReadRegister(FIFODataReg);
				}
			}
		}
	}
	return status;
}

uint8_t MFRC522_Request(uint8_t reqMode, uint8_t *tagType){
	MFRC522_WriteRegister(BitFramingReg, 0x07);

	uint8_t reqBuffer[1];
	uint8_t status;
	uint16_t backBits;

	reqBuffer[0] = reqMode;

	status = MFRC522_ToCard(PCD_TRANSCEIVE, reqBuffer, 1, tagType, &backBits);

	if((status != 0) || (backBits != 0x10)){
		status = 2;
	}
	return status;
}

uint8_t MFRC522_Anticoll(uint8_t *UID){
	uint8_t reqBuffer[2];
	uint8_t status;
	uint16_t backBits;
	uint8_t checksum = 0x00;

	MFRC522_WriteRegister(BitFramingReg, 0x00);

	reqBuffer[0] = 0x93;
	reqBuffer[1] = 0x20;

	status = MFRC522_ToCard(PCD_TRANSCEIVE, reqBuffer, 2, UID, &backBits);

	if(status == 0){
		for(uint8_t i = 0; i < 4; i++)
			checksum ^= UID[i];
	}

	if(checksum != UID[4]){
		status = 2;
	}
	return status;
}
