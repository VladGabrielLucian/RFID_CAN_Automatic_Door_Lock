/*
 * rc522.c
 *
 *  Created on: Apr 13, 2026
 *      Author: TestUser
 */

#include "rc522.h"

extern SPI_HandleTypeDef hspi1;

#define CS_PORT GPIOA
#define CS_PIN  GPIO_PIN_12
#define RST_PORT GPIOB
#define RST_PIN  GPIO_PIN_12

#define CS_ENABLE()  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET)
#define CS_DISABLE() HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET)

// --- FUNCTII DE BAZA (START) ---

void RC522_WriteRegister(uint8_t addr, uint8_t val) {
    uint8_t data[2];
    data[0] = (addr << 1) & 0x7E;
    data[1] = val;
    CS_ENABLE();
    HAL_SPI_Transmit(&hspi1, data, 2, HAL_MAX_DELAY);
    CS_DISABLE();
}

uint8_t RC522_ReadRegister(uint8_t addr) {
    uint8_t txData[2];
    uint8_t rxData[2];
    txData[0] = ((addr << 1) & 0x7E) | 0x80;
    txData[1] = 0x00;
    CS_ENABLE();
    HAL_SPI_TransmitReceive(&hspi1, txData, rxData, 2, HAL_MAX_DELAY);
    CS_DISABLE();
    return rxData[1];
}

void RC522_Init(void) {
    // Reset Hardware
    HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_SET);
    HAL_Delay(50);

    CS_DISABLE();

    // Soft Reset prin comanda 0x0F (recomandat)
    RC522_WriteRegister(0x01, 0x0F);

    // Configurare Timer și Antena
    RC522_WriteRegister(0x2A, 0x8D); // TModeReg
    RC522_WriteRegister(0x2B, 0x3E); // TPrescalerReg
    RC522_WriteRegister(0x2D, 30);   // TReloadRegL
    RC522_WriteRegister(0x2C, 0);    // TReloadRegH
    RC522_WriteRegister(0x15, 0x40); // TxASKReg
    RC522_WriteRegister(0x11, 0x3D); // ModeReg

    RC522_AntennaOn(); // Activeaza emisia RF
}

// --- FUNCTII DE BITMASK (LOGICA DE CONTROL) ---

void RC522_SetBitMask(uint8_t reg, uint8_t mask) {
    RC522_WriteRegister(reg, RC522_ReadRegister(reg) | mask);
}

void RC522_ClearBitMask(uint8_t reg, uint8_t mask) {
    RC522_WriteRegister(reg, RC522_ReadRegister(reg) & (~mask));
}

void RC522_AntennaOn(void) {
    uint8_t temp = RC522_ReadRegister(0x14); // TxControlReg
    if (!(temp & 0x03)) {
        RC522_SetBitMask(0x14, 0x03);
    }
}

// --- PROTOCOLUL RFID (COMUNICATIA CU CARTELA) ---

// Aceasta este functia "creier" care trimite date in aer si așteapta raspuns
uint8_t RC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint16_t *backLen) {
    uint8_t status = 2; // MI_ERR
    uint8_t irqEn = 0x77;
    uint8_t waitIRq = 0x30;
    uint8_t n;
    uint16_t i;

    RC522_WriteRegister(0x04, irqEn | 0x80); // ComIrqReg
    RC522_ClearBitMask(0x04, 0x80);
    RC522_SetBitMask(0x0A, 0x80);            // FIFOLevelReg (Flush FIFO)

    RC522_WriteRegister(0x01, 0x00);         // Idle

    for (i = 0; i < sendLen; i++) {
        RC522_WriteRegister(0x09, sendData[i]); // FIFODataReg
    }

    RC522_WriteRegister(0x01, command);
    if (command == 0x0C) { // PCD_TRANSCEIVE
        RC522_SetBitMask(0x0D, 0x80); // BitFramingReg (StartSend)
    }

    i = 2000;
    do {
        n = RC522_ReadRegister(0x04);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

    RC522_ClearBitMask(0x0D, 0x80);

    if (i != 0) {
        if (!(RC522_ReadRegister(0x06) & 0x1B)) { // ErrorReg
            status = 0; // MI_OK
            if (command == 0x0C) {
                n = RC522_ReadRegister(0x0A);
                uint8_t lastBits = RC522_ReadRegister(0x0C) & 0x07;
                if (lastBits) *backLen = (n - 1) * 8 + lastBits;
                else *backLen = n * 8;
                if (n == 0) n = 1;
                for (i = 0; i < n; i++) {
                    backData[i] = RC522_ReadRegister(0x09);
                }
            }
        }
    }
    return status;
}

uint8_t RC522_Request(uint8_t reqMode, uint8_t *TagType) {
    uint8_t status;
    uint16_t backBits;
    RC522_WriteRegister(0x0D, 0x07); // BitFramingReg
    TagType[0] = reqMode;
    status = RC522_ToCard(0x0C, TagType, 1, TagType, &backBits);
    if ((status != 0) || (backBits != 0x10)) status = 2;
    return status;
}

uint8_t RC522_Anticoll(uint8_t *serNum) {
    uint8_t status;
    uint8_t i;
    uint8_t serNumCheck = 0;
    uint16_t unLen;

    RC522_WriteRegister(0x0D, 0x00);
    serNum[0] = 0x93; // PICC_ANTICOLL
    serNum[1] = 0x20;
    status = RC522_ToCard(0x0C, serNum, 2, serNum, &unLen);

    if (status == 0) {
        for (i = 0; i < 4; i++) serNumCheck ^= serNum[i];
        if (serNumCheck != serNum[4]) status = 2;
    }
    return status;
}
