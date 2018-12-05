/*---------------------------------------------------------------------------------------------

  ESP8266 Muse Monitor OSC Receiver Example by James Clutterbuck https://www.MuseMonitor.com

  Requirements:
  * Arduino OSC library - https://github.com/CNMAT/OSC
  * ESP8266 NodeMCU 1.0 (ESP-12E Module)
  * SSD1331 Color OLED (96x64)
  
  SSD1331 OLED Wiring:
  ESP  -> GND, 3V3,  D5,  D7,  D4, D1, D2
           |    |    |    |    |   |   |
  OLED -> GND, VCC, SCL, SDA, RES, DC, CS

  Features:
  * Splashscreen showing Wifi SSID Name, IP Address and Port.
  * Live OSC bundle message processing from Muse Monitor.
  * Autodetection of Single or four channel absolute waves (/muse/elements/*_absolute).
  * Horseshoe indicator showing headband fit (/muse/elements/horseshoe, /muse/elements/touching_forehead).
  * Relative brainwaves (Delta, Theta, Alpha, Beta, Gamma) calculated from absolutes.
  * Automatic timeout to splashscreen when no data received. 
 
--------------------------------------------------------------------------------------------- */
//Settings
#define WIFI_SSID "*****"
#define WIFI_PASSWORD "*****"
#define LISTEN_PORT 5000
#define TIMEOUT_MILLIS 5000

#define HSI_X 85
#define HSI_Y 9
#define WAVE_SPACING 18
#define LINE_HEIGHT_MAX 50
#define OLED_HEIGHT 63

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

//SSD1331 Color OLED driver. 96x64
#include <SPI.h>
#include <SSD_13XX.h>
#include "_fonts/unborn_small.c"

//OLED Wiring Pins
#define __CS1    D2
#define __DC     D1
#define LCDRESET D4

SSD_13XX display = SSD_13XX(__CS1, __DC);
WiFiUDP Udp;

bool splashScreenCleared = false;

//Absolute
float da = 0;
float ta = 0;
float aa = 0;
float ba = 0;
float ga = 0;

//Relative
float dr = 0;
float tr = 0;
float ar = 0;
float br = 0;
float gr = 0;

//Height
int drh = 0;
int trh = 0;
int arh = 0;
int brh = 0;
int grh = 0;

int hsiTF = 0;//Touching Forehead

long timeoutTargetMS = 0;

#include "display_funcs.h"

void setup() {
  //Reset LCD in case intermittent power turned it off.
  pinMode(LCDRESET,OUTPUT);
  digitalWrite(LCDRESET,LOW);
  delay(500);
  digitalWrite(LCDRESET,HIGH);
  delay(100);
  
  display.begin();
  display.setTextScale(1);
  display.fillScreen(BLACK);
  
  Serial.begin(115200);
  Serial.println("\n\nESP Booted.");
  
  setupWifi();
  showSplashScreen();
}

void touchingForehead(OSCMessage &msg) {
  hsiTF = msg.getInt(0);
}

void horseshoe(OSCMessage &msg) {
  int hsiTP9  = (int)msg.getFloat(0);
  int hsiAF7  = (int)msg.getFloat(1);
  int hsiAF8  = (int)msg.getFloat(2);
  int hsiTP10 = (int)msg.getFloat(3);
  
  //Touching Forehead
  hsiTF?display.fillCircle(HSI_X, HSI_Y-6, 2, BLACK):drawRing(HSI_X, HSI_Y-6);

  //Draw HSI 1=Good, 2=Medium, 4=Bad
  if (hsiTP9==1){
    display.fillCircle(HSI_X-6, HSI_Y+3, 2, GREEN);
  } else if (hsiTP9==2){
    drawRing(HSI_X-6, HSI_Y+3);
  } else {
    display.fillCircle(HSI_X-6, HSI_Y+3, 2, WHITE);
  }  

  if (hsiAF7==1){
    display.fillCircle(HSI_X-6, HSI_Y-3, 2, GREEN);
  } else if (hsiAF7==2){
    drawRing(HSI_X-6, HSI_Y-3);
  } else {
    display.fillCircle(HSI_X-6, HSI_Y-3, 2, WHITE);
  }

  if (hsiAF8==1){
    display.fillCircle(HSI_X+6, HSI_Y-3, 2, GREEN);
  } else if (hsiAF8==2){
    drawRing(HSI_X+6, HSI_Y-3);
  } else {
    display.fillCircle(HSI_X+6, HSI_Y-3, 2, WHITE);
  }

  if (hsiTP10==1){
    display.fillCircle(HSI_X+6, HSI_Y+3, 2, GREEN);
  } else if (hsiTP10==2){
    drawRing(HSI_X+6, HSI_Y+3);
  } else {
    display.fillCircle(HSI_X+6, HSI_Y+3, 2, WHITE);
  }
}

float getAveragePSD(OSCMessage &msg) {
  if (msg.size()==1){
    return msg.getFloat(0); //Combined average can be sent by Muse Monitor
  } else {
    return (msg.getFloat(0)+msg.getFloat(1)+msg.getFloat(2)+msg.getFloat(3))/4.0; //TP9, AF7, AF8, TP10
  }
}

void delta(OSCMessage &msg) {
  da = getAveragePSD(msg);
  //Calculate relative value
  dr = (pow(10,da) / (pow(10,da) + pow(10,ta) + pow(10,aa) + pow(10,ba) + pow(10,ga)));
  //map to screen height
  drh = (int)mapfloat(dr, 0.0, 1.0, 0.0, LINE_HEIGHT_MAX);
}

void theta(OSCMessage &msg) {
  ta = getAveragePSD(msg);
  tr = (pow(10,ta) / (pow(10,da) + pow(10,ta) + pow(10,aa) + pow(10,ba) + pow(10,ga)));
  trh = (int)mapfloat(tr, 0.0, 1.0, 0.0, LINE_HEIGHT_MAX);
}

void alpha(OSCMessage &msg) {
  aa = getAveragePSD(msg);
  ar = (pow(10,aa) / (pow(10,da) + pow(10,ta) + pow(10,aa) + pow(10,ba) + pow(10,ga)));
  arh = (int)mapfloat(ar, 0.0, 1.0, 0.0, LINE_HEIGHT_MAX);
}

void beta(OSCMessage &msg) {
  ba = getAveragePSD(msg);
  br = (pow(10,ba) / (pow(10,da) + pow(10,ta) + pow(10,aa) + pow(10,ba) + pow(10,ga)));
  brh = (int)mapfloat(br, 0.0, 1.0, 0.0, LINE_HEIGHT_MAX);
}

void gamma(OSCMessage &msg) {
  ga = getAveragePSD(msg);
  gr = (pow(10,ga) / (pow(10,da) + pow(10,ta) + pow(10,aa) + pow(10,ba) + pow(10,ga)));
  grh = (int)mapfloat(gr, 0.0, 1.0, 0.0, LINE_HEIGHT_MAX);
}

void loop() {
  OSCBundle bundle;
  int size = Udp.parsePacket();

  if (size > 0) {
    while (size--) {
      bundle.fill(Udp.read());
    }
    
    if (!bundle.hasError()) {
      if (splashScreenCleared==false){
        clearSplashScreen();
      }
      bundle.dispatch("/muse/elements/horseshoe", horseshoe);
      bundle.dispatch("/muse/elements/touching_forehead", touchingForehead);
      bundle.dispatch("/muse/elements/delta_absolute", delta);
      bundle.dispatch("/muse/elements/theta_absolute", theta);
      bundle.dispatch("/muse/elements/alpha_absolute", alpha);
      bundle.dispatch("/muse/elements/beta_absolute", beta);
      bundle.dispatch("/muse/elements/gamma_absolute", gamma);
    }
    //Reset timeout
    timeoutTargetMS=millis()+TIMEOUT_MILLIS;
  } else {    
    //Check for timeout
    if(splashScreenCleared && millis()>timeoutTargetMS){
      splashScreenCleared=false;
      showSplashScreen();
    }
  }

  if (splashScreenCleared){
    //Relative waves
    renderVBar(0, drh, RED);
    renderVBar(WAVE_SPACING, trh, PURPLE);
    renderVBar(WAVE_SPACING*2, arh, BLUE);
    renderVBar(WAVE_SPACING*3, brh, GREEN);
    renderVBar(WAVE_SPACING*4, grh, ORANGE);
  }
}



