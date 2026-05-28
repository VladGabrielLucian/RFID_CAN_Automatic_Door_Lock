#ifndef RC522_H
#define RC522_H

#include "stm32f1xx_hal.h"

// Comenzi PCD (Proximity Coupling Device - Cititor)
#define PCD_IDLE              0x00
#define PCD_AUTHENT           0x0E
#define PCD_RECEIVE           0x08
#define PCD_TRANSMIT          0x04
#define PCD_TRANSCEIVE        0x0C
#define PCD_RESETPHASE        0x0F
#define PCD_CALCCRC           0x03

// Comenzi PICC (Proximity Integrated Circuit Card - Cartela)
#define PICC_REQIDL           0x26
#define PICC_REQALL           0x52
#define PICC_ANTICOLL         0x93
#define PICC_SELEC_TAG        0x93
#define PICC_AUTHENT1A        0x60
#define PICC_AUTHENT1B        0x61
#define PICC_READ             0x30
#define PICC_WRITE            0xA0

// Regiștri MFRC522
#define CommandReg            0x01
#define ComIrqReg             0x04
#define ErrorReg              0x06
#define FIFODataReg           0x09
#define FIFOLevelReg          0x0A
#define ControlReg            0x0C
#define BitFramingReg         0x0D
#define TxControlReg          0x14
#define VersionReg            0x37

// Status
#define MI_OK                 0
#define MI_ERR                2

void RC522_Init(void);
void RC522_WriteRegister(uint8_t addr, uint8_t val);
uint8_t RC522_ReadRegister(uint8_t addr);
void RC522_SetBitMask(uint8_t reg, uint8_t mask);
void RC522_ClearBitMask(uint8_t reg, uint8_t mask);
void RC522_AntennaOn(void);
uint8_t RC522_Request(uint8_t reqMode, uint8_t *TagType);
uint8_t RC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint16_t *backLen);
uint8_t RC522_Anticoll(uint8_t *serNum);
#endif
