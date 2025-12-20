/*
 * BMP280.h
 *
 *  Created on: Dec 15, 2025
 *      Author: david
 */

#ifndef INC_BMP280_H_
#define INC_BMP280_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void BMP280_I2CWrite(uint8_t reg, uint8_t data);
void BMP280_I2CRead(uint8_t reg);
void BMP280_I2CMultiRead(uint8_t startReg,uint8_t size, uint8_t * data);
void BMP280_TempMes(uint8_t * data);
void BMP280_PresMes(uint8_t * data);
void BMP280_Init();

#endif /* INC_BMP280_H_ */
