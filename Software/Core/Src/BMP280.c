/*
 * BMP280.c
 *
 *  Created on: Dec 15, 2025
 *      Author: david
 */


#include "BMP280.h"
#include "i2c.h"


uint8_t buf = 0;

#define BMP280_ADDR (0x77<<1)
#define ID_ADDR 0xD0

uint8_t reg = ID_ADDR;

void BMP280_I2CWrite(uint8_t reg, uint8_t data){
	uint8_t buf[2];
	buf[0] = reg;
	buf[1] = data;
	HAL_I2C_Master_Transmit(&hi2c1,BMP280_ADDR,buf,2, HAL_MAX_DELAY);

}

void BMP280_I2CRead(uint8_t reg){
	uint8_t buf = 0;
	HAL_I2C_Master_Transmit(&hi2c1,BMP280_ADDR,&reg,1, HAL_MAX_DELAY);
	HAL_I2C_Master_Receive(&hi2c1,BMP280_ADDR, &buf,1,HAL_MAX_DELAY);
	printf("The registered address value is : 0x%02X\r\n",buf);
}

void BMP280_I2CMultiRead(uint8_t startReg,uint8_t size, uint8_t * data){
	HAL_I2C_Master_Transmit(&hi2c1,BMP280_ADDR,&startReg,1, HAL_MAX_DELAY);
	HAL_I2C_Master_Receive(&hi2c1,BMP280_ADDR,data,size,HAL_MAX_DELAY);
}

void BMP280_TempMes(uint8_t * data){
	BMP280_I2CMultiRead(0xFA,3,data);
	for(int i = 0; i < 3; i++)
		    {
		        printf("%02X", data[i]);
		    }

	printf("H\r\n");
}

void BMP280_PresMes(uint8_t * data){
	BMP280_I2CMultiRead(0xF7,3,data);
	for(int i = 0; i < 3; i++)
		    {
		        printf("%02X", data[i]);
		    }
	printf("H\r\n");
}

void BMP280_Init(){

	BMP280_I2CWrite(0xF4,0x5F);
	BMP280_I2CRead(0xF4);
	HAL_I2C_Master_Transmit(&hi2c1,BMP280_ADDR,&reg,1, HAL_MAX_DELAY);
	HAL_I2C_Master_Receive(&hi2c1,BMP280_ADDR, &buf,1,HAL_MAX_DELAY);
	printf("The ID address is : 0x%02X\r\n",buf);
	uint8_t data[26];
	BMP280_I2CMultiRead(0x88,26,data);
	for(int i = 0; i < 26; i++)
	    {
	        printf("0x%02X ", data[i]);
	    }
	printf("\r\n");

	//BMP280_I2CRead
}
