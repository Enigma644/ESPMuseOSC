String localIP(){
  IPAddress ip;
  if (WIFI_SSID==""){
    ip = WiFi.softAPIP();
  } else {
    ip = WiFi.localIP();
  }
  String sip = String(ip[0]);
  sip += ".";sip += ip[1];
  sip += ".";sip += ip[2];
  sip += ".";sip += ip[3];
  return(sip);
}

void setupWifi(){
  display.setTextColor(WHITE);
  display.println("WiFi Connecting to:");
  display.setTextColor(BLUE);  
  display.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    yield();
    Serial.print(".");
    display.print(".");
  }
  Serial.println(localIP());
  Udp.begin(LISTEN_PORT);
}

void showSplashScreen(){
  display.setCursor(0,0);
  display.fillScreen(BLACK);
  
  display.setTextColor(WHITE);
  display.println("WiFi Connected to:");
  display.setTextColor(BLUE);
  display.println(WIFI_SSID);

  display.setCursor(0,18);
  display.setTextColor(WHITE);
  display.println("OSC Stream Target IP:");
  display.setTextColor(BLUE);
  display.println(localIP());

  display.setCursor(0,36);
  display.setTextColor(WHITE);
  display.println("OSC Stream Port:");
  display.setTextColor(BLUE);
  display.println(LISTEN_PORT);

  display.setCursor(0,56);
  display.setTextColor(DARK_GREY);
  display.println("Waiting for data...");
}

void clearSplashScreen(){
  display.fillScreen(BLACK);

  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.print("MuseMonitor.com");
  
  //Wave labels
  display.setCursor(4,56);
  display.setTextColor(RED,BLACK);
  display.print("D");

  display.setCursor(WAVE_SPACING+4,56);
  display.setTextColor(PURPLE,BLACK);
  display.print("T");

  display.setCursor((WAVE_SPACING*2)+4,56);
  display.setTextColor(BLUE,BLACK);
  display.print("A");

  display.setCursor((WAVE_SPACING*3)+4,56);
  display.setTextColor(GREEN,BLACK);
  display.print("B");

  display.setCursor((WAVE_SPACING*4)+4,56);
  display.setTextColor(ORANGE,BLACK);
  display.print("G");

  //HSI Arc
  display.drawArc(HSI_X, HSI_Y, 10, 7, 216.0, 144.0, WHITE);

  splashScreenCleared=true;
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
 return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void drawRing(int x, int y){
  display.drawCircle(x, y, 2, RED);
  display.fillRect(x-1, y-1, 3, 3, WHITE);
}

void renderVBar(int xOff, int h, uint16_t color)
{
  if (h>LINE_HEIGHT_MAX){h=LINE_HEIGHT_MAX;}
  if (h<0){h=0;}
  display.drawLine(xOff,OLED_HEIGHT-h,xOff,OLED_HEIGHT,color);
  display.drawLine(xOff,OLED_HEIGHT-LINE_HEIGHT_MAX,xOff,OLED_HEIGHT-h-1,BLACK);
}

