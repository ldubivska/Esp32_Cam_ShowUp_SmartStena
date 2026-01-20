# **Esp32\_Cam\_ShowUp\_SmartStena**

**Katolícka univerzita v Ružomberku** **Pedagogická fakulta** **Predmet:** Internet vecí **Autor:** Lenka Dubivská **Študijný program:** RŠ informatika

**1\. Úvod**

**Slovensky:** Projekt "ShowUp SmartStena" využíva modul ESP32-CAM na vytvorenie inteligentnej fotopasce s webovým rozhraním. Systém automaticky detekuje pohyb pomocou PIR senzora, vyhotoví fotografiu a prostredníctvom webového prehliadača na nej aplikuje grafický rámik. Výsledné snímky sú ukladané na SD kartu a spravované cez lokálny web server.

**English:** The "ShowUp SmartStena" project uses the ESP32-CAM module to create a smart photo trap with a web interface. The system automatically detects motion using a PIR sensor, captures a photo, and applies a graphical frame via a web browser. The resulting images are stored on an SD card and managed through a local web server.

**2\. Charakteristika**

**Popis práce:** Hlavným riadiacim prvkom je doska ESP32-CAM (AI-Thinker), ktorá spravuje kamerový modul, SD kartu a Wi-Fi konektivitu. Zariadenie pracuje v stavovom automate (IDLE, COUNTDOWN, CAPTURING, PROCESSING, COOLDOWN). Po zachytení pohybu PIR senzorom (cez prerušenie/interrupt) sa spustí 5-sekundové odpočítavanie signalizované NeoPixel LED diódou. Po expozícii sa surový záber (RAW) uloží na SD kartu.

Cez prepojenie s webovým rozhraním klientsky prehliadač pomocou HTML5 Canvas elementu spojí vyfotený obrázok s predpripraveným rámikom (frame.png) a výsledok pošle späť na ESP32 (uloženie do priečinka /framed). Systém využíva asynchrónne spracovanie a JavaScript na dynamické sledovanie stavu zariadenia. Na indikáciu stavov (čakanie, fotenie, ukladanie) slúži jedna adresovateľná RGB LED dióda.

| Farba | Stav systému | Popis |
| :---- | :---- | :---- |
| **Červená** | Chyba / Štart | Počas inicializácie, ak zlyhá kamera alebo SD karta. |
| **Tmavozelená** | IDLE | Systém je v pohotovostnom režime a čaká na pohyb. |
| **Oranžová (blikanie)** | COUNTDOWN | 5-sekundový odpočet pred fotením. |
| **Biela** | CAPTURING | Práve prebieha snímanie fotografie. |
| **Modrá** | PROCESSING | Systém čaká, kým webové rozhranie spracuje a odošle zarámovanú fotku. |
| **Zelená/Biela (blikanie)** | COOLDOWN | Fotka bola uložená, systém sa krátko "ochladzuje" pred návratom do IDLE. |

## 

**3\. Použité komponenty**

* **ESP32-CAM** (AI-Thinker module)  
* **PIR senzor** (detekcia pohybu HC-SR501)  
* **NeoPixel LED** (RGB indikácia stavu WS2812B)  
* **MicroSD karta**   
* **Stabilizovaný napájací zdroj** (5V)  
* **Prepojovacie vodiče**

**4\. Použitý softvér**

* **Arduino IDE** – vývojové prostredie.  
* **Jazyk C/C++ (Arduino)** – logika firmvéru a správa hardvéru.  
* **HTML / JavaScript** – webové rozhranie a spracovanie obrazu na strane klienta.  
* **Knižnice:** esp\_camera.h (ovládanie kamery), SD\_MMC.h (práca s kartou), WebServer.h (lokálny server), Adafruit\_NeoPixel.h (ovládanie LED).

**5\. Odkazy na inšpiráciu**

* [Using SD Card with ESP32-CAM](https://randomnerdtutorials.com/esp32-cam-take-photo-save-microsd-card/)  
* [HTML5 Canvas Image Merging Tutorial](https://www.youtube.com/watch?v=W1xG_XJb0FU)

* https://randomnerdtutorials.com/esp32-cam-take-photo-display-web-server/

