/**
 * ESP A: FINAL DOOR UNIT (Active High Logic)
 * Logic: Laser ON = 4095 | Laser BLOCKED = 0
 * Trigger: Alarm if value DROPS below 2000
 */
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// --- HARDWARE ---
const int sensorPin = 34;   // Using Pin 34
const int laserPin = 2;     
const int threshold = 2000; // Midpoint between 0 and 4095

bool isArmed = false; 
unsigned long armingTime = 0; 

// --- RADIO ---
typedef struct struct_message {
  char type; 
  int val;
} struct_message;

struct_message myMsg;
struct_message incomingMsg;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peerInfo;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingMsg, incomingData, sizeof(incomingMsg));
  if (incomingMsg.type == 'C') {
    if (incomingMsg.val == 1) {
       // ARM
       digitalWrite(laserPin, HIGH); 
       armingTime = millis();        
       isArmed = true;
    } else {
       // DISARM
       isArmed = false;
       digitalWrite(laserPin, LOW);  
    }
  }
}

void setup() {
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);
  
  // Pin 34 Input (Requires Sensor VCC to be connected!)
  pinMode(sensorPin, INPUT); 

  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) return;
  
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0; peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
  
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

void loop() {
  if (isArmed) {
    // 3 Second Grace Period
    if (millis() - armingTime > 3000) {
      
      int lightLevel = analogRead(sensorPin);
      
      // --- LOGIC FLIP ---
      // BEFORE: if (lightLevel > threshold) ...
      // NOW: We alarm if lightLevel DROPS below 2000 (Darkness)
      
      if (lightLevel < threshold) { 
        // BEAM BROKEN (Darkness detected)
        myMsg.type = 'A'; 
        esp_now_send(broadcastAddress, (uint8_t *) &myMsg, sizeof(myMsg));
        
        digitalWrite(laserPin, LOW);
        isArmed = false; 
        delay(1000); 
      }
    }
  }
  delay(20);
}