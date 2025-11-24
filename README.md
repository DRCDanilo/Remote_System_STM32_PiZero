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

A. Les adresses I²C possibles pour ce composant sont:
