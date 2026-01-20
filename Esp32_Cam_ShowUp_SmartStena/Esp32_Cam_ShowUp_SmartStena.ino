#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <vector>
#include <algorithm>

const char* ssid = "    ";
const char* password = "    ";

#define PIR_PIN 13
#define LED_PIN 12
#define LED_COUNT 1

// AI-Thinker Pinout
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

enum SystemState { IDLE, COUNTDOWN, CAPTURING, PROCESSING, COOLDOWN };
SystemState currentState = IDLE;

Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);

unsigned long stateStartTime = 0;
volatile bool pirTriggered = false;
String lastRawPath = "";
File currentUploadFile;

void setLED(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void IRAM_ATTR pirISR() {
  if (currentState == IDLE) pirTriggered = true;
}

String getStatusText() {
  switch(currentState) {
    case IDLE: return "IDLE";
    case COUNTDOWN: return "COUNTDOWN";
    case CAPTURING: return "CAPTURING";
    case PROCESSING: return "WAITING_FOR_FRAME"; 
    case COOLDOWN: return "SAVING";
    default: return "BUSY";
  }
}

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM; config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM; config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM; config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_UXGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  esp_err_t err = esp_camera_init(&config);
  return (err == ESP_OK);
}

void handleRoot() {
  String html = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>body{font-family:sans-serif;text-align:center;background:#000;color:#fff;padding:10px;} ";
  html += ".photo{width:100%; max-width:450px; border:5px solid #fff; margin-bottom:15px;} ";
  html += ".loader{background:#0051ff; color:white; padding:20px; font-weight:bold; margin:20px; border-radius:10px;}</style>";
  html += "<script>";
  html += "let processing = " + String(lastRawPath != "" ? "true" : "false") + ";";
  html += "setInterval(() => { ";
  html += "  fetch('/status?t=' + Date.now()).then(r => r.text()).then(st => {";
  html += "    if(st === 'WAITING_FOR_FRAME' && !processing) window.location.reload();";
  html += "    if(st === 'IDLE' && processing) window.location.replace('/');";
  html += "  });";
  html += "}, 1000);";

  if (lastRawPath != "") {
    html += "window.onload = async () => {";
    html += "  try {";
    html += "    const c = document.createElement('canvas'); const ctx = c.getContext('2d');";
    html += "    const i = new Image(); const f = new Image();";
    html += "    i.src = '/file?name=" + lastRawPath + "&t=' + Date.now();";
    html += "    f.src = '/file?name=/frame.png';";
    html += "    await Promise.all([i.decode(), f.decode()]);";
    html += "    c.width = 800; c.height = 800;";
    html += "    ctx.drawImage(i, 50, 50, 700, 700); ctx.drawImage(f, 0, 0, 800, 800);";
    html += "    c.toBlob(b => {";
    html += "      let d = new FormData(); d.append('file', b, 'f.jpg');";
    html += "      fetch('/saveFramed', {method:'POST', body:d});";
    html += "    }, 'image/jpeg', 0.85);";
    html += "  } catch(e) { setTimeout(() => window.location.reload(), 2000); }";
    html += "};";
  }
  html += "</script></head><body>";
  if (lastRawPath != "") html += "<div class='loader'>🎨 GENEROVANIE...</div>";
  else html += "<h1>ShowUp SmartStena</h1>";

  File dir = SD_MMC.open("/framed");
  std::vector<String> list;
  if(dir){
    File f = dir.openNextFile();
    while(f){ if(!f.isDirectory()) list.push_back(f.name()); f = dir.openNextFile(); }
    dir.close();
    std::sort(list.begin(), list.end(), std::greater<String>());
    for(auto& n : list) {
      html += "<img class='photo' src='/file?name=/framed/" + n + "'><br>";
      html += "<a href='/delete?file=/framed/" + n + "' style='color:gray'>Odstrániť</a><br><br>";
    }
  }
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleStatus() { server.send(200, "text/plain", getStatusText()); }

void handleFile() {
  String p = server.arg("name");
  if (SD_MMC.exists(p)) {
    File f = SD_MMC.open(p, "r");
    server.streamFile(f, p.endsWith(".png") ? "image/png" : "image/jpeg");
    f.close();
  } else server.send(404);
}

void handleSaveFramed() {
  HTTPUpload& u = server.upload();
  if (u.status == UPLOAD_FILE_START) {
    currentUploadFile = SD_MMC.open("/framed/F_" + String(millis()) + ".jpg", FILE_WRITE);
  } else if (u.status == UPLOAD_FILE_WRITE && currentUploadFile) {
    currentUploadFile.write(u.buf, u.currentSize);
  } else if (u.status == UPLOAD_FILE_END && currentUploadFile) {
    currentUploadFile.close();
    if (lastRawPath != "") { SD_MMC.remove(lastRawPath); lastRawPath = ""; }
    currentState = COOLDOWN;
    stateStartTime = millis();
    server.send(200);
  }
}

void handleDelete() {
  String f = server.arg("file");
  if (f.startsWith("/framed/")) SD_MMC.remove(f);
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  pixel.begin();
  setLED(50, 0, 0); 

  Serial.println("\n--- START ShowUp SmartStena ---");

  // 1. Inicializácia kamery
  if (!initCamera()) {
    Serial.println("Kamera: CHYBA!");
  } else {
    Serial.println("Kamera: OK");
  }

  // 2. SD Karta (1-bit mód pre uvoľnenie pinov 12 a 13)
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("SD Karta: CHYBA!");
  } else {
    Serial.println("SD Karta: OK");
  }

  // 3. WiFi
  WiFi.begin(ssid, password);
  Serial.print("Pripájanie k WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // VÝPIS IP ADRESY
  Serial.println("\nWiFi OK!");
  Serial.print("IP Adresa: http://");
  Serial.println(WiFi.localIP());
  Serial.println("---------------------------");
  
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/file", handleFile);
  server.on("/delete", handleDelete);
  server.on("/saveFramed", HTTP_POST, [](){}, handleSaveFramed);
  server.begin();

  pinMode(PIR_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), pirISR, RISING);

  setLED(0, 50, 0); 
}

void loop() {
  server.handleClient();
  unsigned long now = millis();

  switch (currentState) {
    case IDLE:
      if (pirTriggered) {
        Serial.println("PIR: Pohyb detekovaný!");
        pirTriggered = false;
        currentState = COUNTDOWN;
        stateStartTime = now;
      } else {
        setLED(0, 15, 0); 
      }
      break;

    case COUNTDOWN:
      if ((now % 1000) < 500) setLED(255, 100, 0);
      else setLED(0, 0, 0);
      
      if (now - stateStartTime >= 5000) {
        Serial.println("FOTÍM...");
        setLED(255, 255, 255);
        
        // Vyčistenie bufferov
        for(int i=0; i<2; i++){
          camera_fb_t * fb = esp_camera_fb_get();
          if(fb) esp_camera_fb_return(fb);
        }
        
        camera_fb_t * fb = esp_camera_fb_get();
        if (fb) {
          lastRawPath = "/raw/R_" + String(millis()) + ".jpg";
          File file = SD_MMC.open(lastRawPath, FILE_WRITE);
          if (file) {
            file.write(fb->buf, fb->len);
            file.close();
            Serial.println("RAW uložený: " + lastRawPath);
            currentState = PROCESSING;
            stateStartTime = now;
          } else {
            Serial.println("SD Zápis zlyhal!");
            currentState = IDLE;
          }
          esp_camera_fb_return(fb);
        } else {
          Serial.println("Kamera nezachytila snímku!");
          currentState = IDLE;
        }
      }
      break;

    case PROCESSING:
      setLED(0, 0, 255); 
      if (now - stateStartTime >= 60000) {
        if (lastRawPath != "") { SD_MMC.remove(lastRawPath); lastRawPath = ""; }
        currentState = IDLE;
      }
      break;

    case COOLDOWN:
      if ((now % 200) < 100) setLED(255, 255, 255);
      else setLED(0, 255, 0);
      if (now - stateStartTime >= 3000) currentState = IDLE;
      break;
  }
}