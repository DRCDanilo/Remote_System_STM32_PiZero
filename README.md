# Remote_System_STM32_PiZero

Travail pratique de Bus et Reseaux par David CONTION et Danilo DEL RIO CISNEROS. Novembre 2025.

Ce travail pratique a pour but de 

Le schéma du matériel à utiliser est :

<img width="523" height="223" alt="path1315-0-9-37" src="https://github.com/user-attachments/assets/23408b40-98c5-46c7-8e92-ac812d4f631c" />

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
* Une liaison I²C. I2C3 : I2C3_SCL -> PA_8. I2C3_SDA -> PB_4.
* Une liasion UART sur USB : UART2 sur les broches PA_2 et PA_3.
* Une liaison UART indépendante pour la communication avec le Raspberry : UART1 : UART1_RX -> PA_10. UART1_TX -> PA_9.
* Une liaison CAN : CAN1 : CAN1_RD -> PB_8. CAN1_TD -> PB_9.

Redirection du print

Afin de pouvoir facilement déboguer votre programme STM32, faites en sorte que la fonction printf renvoie bien ses chaînes de caractères sur la liaison UART sur USB, en ajoutant le code suivant au fichier stm32f4xx_hal_msp.c:

```C
/* USER CODE BEGIN PV */
extern UART_HandleTypeDef huart2;
/* USER CODE END PV */

/* USER CODE BEGIN Macro */
#ifdef __GNUC__ /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf    set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
/* USER CODE END Macro */

/* USER CODE BEGIN 1 */
/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART2 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}
/* USER CODE END 1 */

```

### 1.3 Communication I²C 

Identification du BMP280

## 2. Interfaçage STM32 - Raspberry

Objectif: Permettre l'interrogation du STM32 via un Raspberry Pi Zero Wifi.

### 2.1 Mise en route du Raspberry Pi Zero

**Préparation du Raspberry**

Avec l'aide du logiciel Raspberry Pi Imager on a installé l'image de l'OS Raspberry Pi OS Lite.

Pour activer le port série sur connecteur GPIO, sur la partition boot, on a modifié le fichier config.txt. On a utilisé l'editeur nano pour ajouter à la fin les lignes suivantes:

```
enable_uart=1
dtoverlay=disable-bt
```

Pour que le noyau libère le port UART, il faut retirer l'option suivante, dans le fichier cmdline.txt:

```
console=serial0,115200
```
### 2.2 Port Série

**Loopback**

Il faut brancher le port série du Raspberry en boucle: RX sur TX.

Pour testet le port série, il est possible de faire avec le logiciel minicom. On a installé le logiciel minicom, et après avec la commande on a l'exécuté:

```
minicom -D /dev/ttyAMA0
```

**Communication avec la STM32**
Le port UART pour la communication avec la Raspberri Pi Zero est le port UART1.

Le protocole de communication entre le Raspberry et la STM32 est le suivant:

<img width="680" height="197" alt="image" src="https://github.com/user-attachments/assets/98c2a7a0-5822-4a7f-8bd1-bcf31a509976" />

### 2.3 Commande depuis Python

On a installé pip pour python3 sur le Raspberry avec les commandes:

```
sudo apt update
sudo apt install python3-pip
```

Après, on a installé la bibliothèque pyserial pour acceder au port série.

On a écrit un script en utilisant Python 3 qui permet communiquer avec le STM32. Le script est dans **...?**

## 3. Interface REST

Objectif: Développement d'une interface REST sur le Raspberry

### 3.1 Installation du serveur Python

**Installation**

**Premier fichier Web**

### 3.2 Première page REST

**Première route**

**Première page REST**
**Réponse JSON**

**1re solution**

**2e solution**

**Erreur 404**

### 3.3 Nouvelles métodes HTTP

**Méthodes POST, PUT, DELETE…**

**Méthode POST**

**API CRUD**

<img width="646" height="381" alt="image" src="https://github.com/user-attachments/assets/3342fa5b-49a4-4e99-aa7e-6dfc4e3374b9" />

### 3.4 Et encore plus fort...

