#include <Arduino.h>
#include <KasaSmartPlug.h>
#include <WiFi.h>
#include "time.h"
#include "esp_system.h"
#include "esp_sntp.h"

// Wi-Fi details and the Kasa switch name live in include/secrets.h, which is
// git-ignored. Copy include/secrets.example.h to include/secrets.h to set them.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#warning "secrets.h not found - using the placeholder values from secrets.example.h"
#include "secrets.example.h"
#endif

// Pins
#define LED_PIN              27
#define MOTION_PIN           12

// Hardware Config
#define PWM_FREQ    5000
#define PWM_CHANNEL 1
#define PWM_RES     8

// Config Settings
#define LED_DAY_BRIGHTNESS   255
#define LED_NIGHT_BRIGHTNESS 100
#define START_NIGHT          22
#define END_NIGHT            6
#define LIGHT_TIMEOUT        5
#define LED_FADE_TIME        2

// WIFI
const char* ssid       = WIFI_SSID;
const char* password   = WIFI_PASSWORD;

// Time
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const char* time_zone = "PST8PDT,M3.2.0,M11.1.0";  // POSIX TZ rule: US Pacific, with daylight saving
struct tm timeinfo;

// WIFI Switch
KASAUtil kasaUtil;
KASASmartPlug *lightSwitch = NULL;

// Timer
hw_timer_t *My_timer = NULL;

// Flags
bool motionFlag = false;
bool timeFlag = false;
bool turnOnLights = true;
bool lights_are_on = false;


/*--------------- Functions -----------------*/
void printLocalTime() {
  if(!getLocalTime(&timeinfo)){
    Serial.println("No time available (yet)");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  int current_min = (timeinfo.tm_hour * 60) + timeinfo.tm_min;
  Serial.println(current_min);
}

void lightsOn() {
  if(turnOnLights){
    if(lightSwitch != NULL){
      lightSwitch->SetRelayState(1);
    }
    for(int dutyCycle = ledcRead(PWM_CHANNEL); dutyCycle <= LED_DAY_BRIGHTNESS; dutyCycle++){
      ledcWrite(PWM_CHANNEL, dutyCycle);   
      delay(5);
    }
    ledcWrite(PWM_CHANNEL, LED_DAY_BRIGHTNESS); 
  }
  else{
    for(int dutyCycle = ledcRead(PWM_CHANNEL); dutyCycle <= LED_NIGHT_BRIGHTNESS; dutyCycle++){
      ledcWrite(PWM_CHANNEL, dutyCycle);
      delay(8);
    }
    ledcWrite(PWM_CHANNEL, LED_NIGHT_BRIGHTNESS); 
  }
}

void lightsOff() {
  if(lightSwitch != NULL){
    lightSwitch->SetRelayState(0);
  }
  for(int dutyCycle = ledcRead(PWM_CHANNEL); dutyCycle >= 0; dutyCycle--){
    ledcWrite(PWM_CHANNEL, dutyCycle);   
    delay(15);
  }
  ledcWrite(PWM_CHANNEL, 0); 
}

void IRAM_ATTR motionISR() {
  motionFlag = true;
}

void IRAM_ATTR onTimer() {
  timeFlag = true;
}

/*--------------- Setup -----------------*/
void setup() {
  Serial.begin(115200);

  // Time server setup
  sntp_servermode_dhcp(1);    // (optional)
  configTzTime(time_zone, ntpServer1, ntpServer2);

  // Connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  
  // Switch setup
  int found;
  found = kasaUtil.ScanDevices();
  Serial.printf("\r\n Found device = %d", found);
  for (int i = 0; i < found; i++)
  {
    KASASmartPlug *p = kasaUtil.GetSmartPlugByIndex(i);
    if (p != NULL)
    {
      Serial.printf("\r\n %d. %s IP: %s Relay: %d", i, p->alias, p->ip_address, p->state);
    }
  }
  lightSwitch = kasaUtil.GetSmartPlug(SWITCH_NAME); 
  
  Serial.printf("\r\n lightSwitch = %p", (void *)lightSwitch);
  // Motion sensor setup
  pinMode(MOTION_PIN, INPUT);

  // LED setup
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(LED_PIN, PWM_CHANNEL);

  Serial.println("Setup Finished");
  printLocalTime();
}

/*--------------- Main Loop -----------------*/
void loop() {
  // Turn lights on 
  Serial.println(digitalRead(MOTION_PIN));
  if(!lights_are_on && digitalRead(MOTION_PIN)){
    lightsOn();
    lights_are_on = true;
  }
  // Turn lights off if time is up
  if(lights_are_on && !digitalRead(MOTION_PIN)){
    lightsOff();
    lights_are_on = false;
  }
  // Scan for the device if it was not found at start
  if(lightSwitch == NULL){
    kasaUtil.ScanDevices(200);
    lightSwitch = kasaUtil.GetSmartPlug(SWITCH_NAME); 
  }
  // Check time of day
  if(getLocalTime(&timeinfo)){
    int current_min = (timeinfo.tm_hour * 60) + timeinfo.tm_min;
    turnOnLights = ((END_NIGHT * 60 < current_min) && (current_min < START_NIGHT * 60));
  }
  delay(1000);
}
