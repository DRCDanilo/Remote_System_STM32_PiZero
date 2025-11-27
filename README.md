# Remote_System_STM32_PiZero

Travail pratique de Bus et Reseaux par David CONTION et Danilo DEL RIO CISNEROS. Novembre 2025.

Ce travail pratique a pour but de 

Le schéma du matériel à utiliser est :

<img width="1047" height="447" alt="path1315-0-9-37" src="https://github.com/user-attachments/assets/23408b40-98c5-46c7-8e92-ac812d4f631c" />

## 1. Bus I²C

Objectif: Interfacer un STM32 avec des capteurs I2C.

La première étape est de mettre en place la communication entre le microcontrôleur et les capteurs (température, pression, accéléromètre...) via  le bus I²C.

Le capteur comporte 2 composants I²C, qui partagent le même bus. Le STM32 jouera le rôle de Master sur le bus.

Le code du STM32 sera écrit en langage C, en utilisant la bibliothèque HAL.

### 1.1 Capteur BMP280

* **Les adresses I²C possibles pour ce composant sont:** L'adresse de 7 bits du composant est 111011x. Le dernier bit est variable pour la valeur de SDO.
  Si SDO est connecté à GND, l'adresse est 1110110 (0x76). Si SDO est connecté à VDDIO, l'adresse est 1110111 (0x77).
* **Le registre et la valeur permettant d'identifier ce composant:** Le registre est 0xD0 qui contient le numéro d'identification de la puce chip_id[7:0]. La valeur de ce registre est 0x58.
* **Le registre et la valeur permettant de placer le composant en mode normal:** Le registre de control est 0xF4. Pour configurer le mode du composant il faut changer la valeur des bits mode[1:0]. mode[1:0] = 00 -> Sleep Mode, 01 et 10 -> Forced Mode, 11 -> Normal Mode.
* **Les registres contenant l'étalonnage du composant:** Les registres sont calib25...calib00, avec les adresses 0xA1...0X88.
* **Les registres contenant la température (ainsi que le format):** La donnée brute de la température ut[19:0] est dans les registres temps. Les registres sont: temp_msb[7:0] avec l'adresse 0xFA qui contient la part MSB d'ut[19:12], temp_lsb[7:0] avec l'adresse 0xFB qui contient la part LSB d'ut[11:4], temp_xlsb[3:0] (bit 7, 6, 5, 4) avec l'adresse 0xFC qui contient la part XLSB d'ut[3:0].
* **Les registres contenant la pression (ainsi que le format):** La donnée brute de la pression up[19:0] est dans les registres press. Les registres sont: press_msb[7:0] avec l'adresse 0xF7 qui contient la part MSB d'up[19:12], press_lsb[7:0] avec l'adresse 0xF8 qui contient la part LSB d'ut[11:4], temp_xlsb[3:0] (bit 7, 6, 5, 4) avec l'adresse 0xF9 qui contient la part XLSB d'ut[3:0].
* **Les fonctions permettant le calcul de la température et de la pression compensées, en format entier 32 bits:**
  "Compensation formula in 32 bit fixed point"
  ```C
  // Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
  // t_fine carries fine temperature as global value
  BMP280_S32_t t_fine;
  BMP280_S32_t bmp280_compensate_T_int32(BMP280_S32_t adc_T)
  {
    BMP280_S32_t var1, var2, T;
    var1 = ((((adc_T>>3) – ((BMP280_S32_t)dig_T1<<1))) * ((BMP280_S32_t)dig_T2)) >> 11;
    var2 = (((((adc_T>>4) – ((BMP280_S32_t)dig_T1)) * ((adc_T>>4) – ((BMP280_S32_t)dig_T1))) >> 12) *
    ((BMP280_S32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
  }

  // Returns pressure in Pa as unsigned 32 bit integer. Output value of “96386” equals 96386 Pa = 963.86 hPa
  BMP280_U32_t bmp280_compensate_P_int32(BMP280_S32_t adc_P)
  {
    BMP280_S32_t var1, var2;
    BMP280_U32_t p;
    var1 = (((BMP280_S32_t)t_fine)>>1) – (BMP280_S32_t)64000;
    var2 = (((var1>>2) * (var1>>2)) >> 11 ) * ((BMP280_S32_t)dig_P6);
    var2 = var2 + ((var1*((BMP280_S32_t)dig_P5))<<1);
    var2 = (var2>>2)+(((BMP280_S32_t)dig_P4)<<16);
    var1 = (((dig_P3 * (((var1>>2) * (var1>>2)) >> 13 )) >> 3) + ((((BMP280_S32_t)dig_P2) * var1)>>1))>>18;
    var1 =((((32768+var1))*((BMP280_S32_t)dig_P1))>>15);
    if (var1 == 0)
    {
      return 0; // avoid exception caused by division by zero
    }
    p = (((BMP280_U32_t)(((BMP280_S32_t)1048576)-adc_P)-(var2>>12)))*3125;
    if (p < 0x80000000)
    {
      p = (p << 1) / ((BMP280_U32_t)var1);
    }
    else
    {
      p = (p / (BMP280_U32_t)var1) * 2;
    }
    var1 = (((BMP280_S32_t)dig_P9) * ((BMP280_S32_t)(((p>>3) * (p>>3))>>13)))>>12;
    var2 = (((BMP280_S32_t)(p>>2)) * ((BMP280_S32_t)dig_P8))>>13;
    p = (BMP280_U32_t)((BMP280_S32_t)p + ((var1 + var2 + dig_P7) >> 4));
    return p;
  }
  
  ```

### 1.2 Setup du STM32

Pour ce TP, on a utilisé:
* Une liaison I²C. I2C2 : I2C2_SCL -> PB_6. I2C2_SDA -> PB_3.
* Une liasion UART sur USB : UART2 sur les broches PA_2 et PA_3.
* Une liaison UART indépendante pour la communication avec le Raspberry : UART1 : UART1_RX -> PA_10. UART1_TX -> PA_9.
* Une liaison CAN : CAN1 : CAN1_RD -> PB_8. CAN1_TD -> PB_9.
