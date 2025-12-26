# Remote_System_STM32_PiZero

Travail pratique de Bus et Reseaux par David CONTION et Danilo DEL RIO CISNEROS. Novembre 2025.


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

**Primitives I²C sous STM32_HAL**

L'API HAL (Hardware Abstraction Layer) fournit par ST propose entre autres 2 primitives permettant d'interagir avec le bus I²C en mode Master:

    HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout)

    HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout)

où:

    - I2C_HandleTypeDef hi2c: structure stockant les informations du contrôleur I²C

    - uint16_t DevAddress: adresse I³C du périphérique Slave avec lequel on souhaite interagir.

    - uint8_t *pData: buffer de données

    - uint16_t Size: taille du buffer de données

    - uint32_t Timeout: peut prendre la valeur HAL_MAX_DELAY


## Communication avec le BMP280

**Identification du BMP280**

L'identification du BMP280 consiste en la lecture du registre ID

En I²C, la lecture se déroule de la manière suivante:

- Envoyer l'adresse du registre ID
- Recevoir 1 octet correspondant au contenu du registre

On a bien vérifié que le contenu du registre correspond bien à la datasheet : 0x58


<img width="318" height="156" alt="image" src="https://github.com/user-attachments/assets/db89eaef-5ad4-4c51-a9d0-c0bdf6eafc60" />


**Configuration du BMP280**

Avant de pouvoir faire une mesure, il faut configurer le BMP280.

Pour commencer, nous allons utiliser la configuration suivante: mode normal, Pressure oversampling x16, Temperature oversampling x2.

Le registre pour configurer le capteur est le registre 0xF4 "ctrl_meas".

ctrl_meas:

osrs_t[2:0] osrs_p[2:0] mode[1:0]

De la datasheet et pour avoir la configuration souhaitée:

osrs_t       0  1  0

osrs_p                1  0  1

mode                            1  1

**ctrl_meas : 0101 0111 = 0x57**

En I²C, l'écriture dans un registre se déroule de la manière suivante:

- Envoyer l'adresse du registre à écrire, suivi de la valeur du registre
- Si on reçoit immédiatement, la valeur reçu sera la nouvelle valeur du registre

Vérifiez que la configuration a bien été écrite dans le registre.

On a réussi à écrire la valeur correcte dans le registre : 

<img width="451" height="190" alt="image" src="https://github.com/user-attachments/assets/22bb390b-e915-458d-b638-c476de07462b" />


**Récupération de l'étalonnage, de la température et de la pression**


**Calcul des températures et des pression compensées**



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

On a créé d'autre utilisateur avec utilisateur différent de pi, remplacer XXX par le nom de votre choix, avec les droits de sudo et  d'accès au port série (dialout): 

```
sudo adduser dada
sudo usermod -aG sudo dada
sudo usermod -aG dialout dada
```


On a Délogguez vous, puis relogguez vous en tant que dada.

Créez un répertoire pour le développement de votre serveur. Dans ce répertoire, créez un fichier nommé requirement.txt dans lequel vous placerez le texte suivants:

```
pyserial
flask
```

En fait, après on a doit installer flask en utilisant la command:

```
sudo apt install python3-flask
```

Pour pyserial on a utilisé: 

```
sudo apt install python3-serial
```

Après, on a À nouveau, délogguez vous, puis relogguez vous en tant que XXX pour mettre à jour le PATH et permettre de lancer flask.


**Premier fichier Web**


### 3.2 Première page REST


**Première route**

Quel est le rôle du décorateur @app.route?

Le décorateur serve pour indiquer l'itineraire de la page.

Quel est le role du fragment ```int:index```?

Crée de sub-pages avec la valeur de index.

Pour pouvoir prétendre être RESTful, votre serveur va devoir:

    répondre sous forme JSON.
    différencier les méthodes HTTP

C’est ce que nous allons voir maintenant.


**Première page REST**

**Réponse JSON**

On a modifié le fihcier hello.py. On a ajouté la ligne import json et on a remplacé la dernière ligne de la fonction api_welcome_index par:

```
return json.dumps({"index": index, "val": welcome[index]})
```

Le résultat est:

IMGAGE DU ORDINATEUR DE DAVID AVEC {"index":6, "val": "c"}.

Est-ce suffisant pour dire que la réponse est bien du JSON? : Non, car le Content-Type n'est pas JSON, c'est HTML.


**1re solution**

On a modifié la réponse, ....,  en ajoutant la commande suivante: 

```
return json.dumps({"index": index, "val": welcome[index]}), {"Content-Type": "application/json"}
```

IMAGEN DE DAVID OÙ IL DIT OBJECT TYPE: JSON


**2e solution**
On a utilisé jsonify() existe dans la bibliothèque. On a ajouté ```from flask import jsonify```.


```
return jsonify({"index": index, "val": welcome[index]})
```

**Erreur 404**

On a téléchargé le fichier page_not:found.html (https://moodle.ensea.fr/mod/resource/view.php?id=19916). On l'ajouté dans le fichier templates. 

On a ajouté le code suivant:

```
@app.errorhandler(404)
def page_not_found(error):
    return render_template('page_not_found.html'), 404
```

On a modifié la fonctions api_welcome_index de manière à retourner cette page 404 si jamais l’index n’est pas correct. Flask fournit une fonction pour cela : abort(404). Dans le code du fichier hello.py on a ajouté:

```
from flask import abort, render_template, request
```

Aussi dans la fonction api_welcome_index on a écrit: 

```
if(index>len(welcome)):
  abort(404)
```

IMAGE DE DAVID AVEC L'ERREUR 404 DANS LE NAVIGATEUR


### 3.3 Nouvelles métodes HTTP

**Méthodes POST, PUT, DELETE…**
Pour être encore un peu plus RESTful, votre application doit gérer plusieurs méthodes (verb) HTTP

**Méthode POST**

On a modifié la fonction en ajoutant des paramètres de la manière suivante:

```
@app.route('/api/welcome/<int:index>', methods=['GET','POST'])
```

En plus, on a ajouté la fonction suivante:

```
@app.route('/api/request/', methods=['GET', 'POST'])
@app.route('/api/request/<path>', methods=['GET','POST'])
def api_request(path=None):
    resp = {
            "method":   request.method,
            "url" :  request.url,
            "path" : path,
            "args": request.args,
            "headers": dict(request.headers),
    }
    if request.method == 'POST':
        resp["POST"] = {
                "data" : request.get_json(),
                }
    return jsonify(resp)
```

<img width="795" height="208" alt="image" src="https://github.com/user-attachments/assets/c9aa6767-2189-4ee4-be45-716a155b1800" />

En plus, on a testé depuis la Raspberry:

<img width="741" height="192" alt="image" src="https://github.com/user-attachments/assets/c722d68d-3c07-4d3f-8644-56acdde5c4b6" />

On a utilisé l’extension RESTED sur firefox pour avoir une réponse qui peuple correctement les champs *args* et *data*.

On a testé la méthode POST :

<img width="1240" height="829" alt="image" src="https://github.com/user-attachments/assets/4287ce00-ae0a-478f-b2dd-71c199a301ad" />

On a testé la méthode GET : 

<img width="1237" height="847" alt="image" src="https://github.com/user-attachments/assets/860bfba0-6300-4df3-94e6-e6b2276038fb" />




**API CRUD**

<img width="646" height="381" alt="image" src="https://github.com/user-attachments/assets/3342fa5b-49a4-4e99-aa7e-6dfc4e3374b9" />

### 3.4 Et encore plus fort...


## 4. Bus CAN

Objectif: Développement d'une API Rest et mise en place d'un périphérique sur bus CAN

Les cartes STM32L476 sont équipées d'un contrôleur CAN intégré. Pour pouvoir les utiliser, il faut leur adjoindre un Tranceiver CAN. Ce rôle est dévolu à un TJA1050 (https://www.nxp.com/docs/en/data-sheet/TJA1050.pdf). Ce composant est alimenté en 5V, mais possède des E/S compatibles 3,3V.

Afin de faciliter sa mise en œuvre, ce composant a été installé sur une carte fille (shield) au format Arduino, qui peut donc s'insérer sur les cartes nucléo64:

<img width="1289" height="575" alt="image" src="https://github.com/user-attachments/assets/7674325d-4afa-4afe-9aad-5f996825ada9" />

Ce shield possède un connecteur subd9, qui permet de connecter un câble au format CAN. Pour rappel, le brochage de ce connecteur est le suivant:

<img width="745" height="708" alt="image" src="https://github.com/user-attachments/assets/868377f5-1a24-4b75-b836-f30a180d705a" />

Seules les broches 2, 3 et 7 sont utilisés sur les câbles à votre dispositions.

**Remarque**: Vous pourrez noter que les lignes CANL et CANH ont été routées en tant que paire différentielle, et qu'une boucle a été ajouté à la ligne CANL pour la mettre à la même longueur que la ligne CANH.


Vous allez utiliser le bus CAN pour piloter un module moteur pas-à-pas. Ce module s'alimente en +12V. L'ensemble des informations nécessaires pour utiliser ce module est disponible dans ce document:
https://enseafr-my.sharepoint.com/:b:/r/personal/danilo_delriocisneros_ensea_fr/Documents/ENSEA/Dipl%C3%B4me%20-%20Semestres/S9%20-%20ESE/4-Bus%20et%20r%C3%A9seaux%20industriels/docs%20publics%20for%20GitHub/moteur.pdf?csf=1&web=1&e=Sqezai

La carte moteur est un peu capricieuse et ne semble tolérer qu'une vitesse CAN de 500kbit/s. Pensez à régler CubeMx en conséquence.
Edit 2022: Il semble que ce soit surtout le ratio seg2/(seg1+seg2), qui détermine l'instant de décision, qui doit être aux alentours de 87%. Vous pouvez utiliser le calculateur suivant: http://www.bittiming.can-wiki.info/

### 4.1 Pilotage du moteur


### 4.2 Interfaçage avec le capteur
