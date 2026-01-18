/**
 * ESP B: HUB (Status Reporter Version)
 */
#include <BleKeyboard.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

BleKeyboard bleKeyboard("ESP32 Tripwire", "Espressif", 100);

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct struct_message {
  char type; 
  int val;
} struct_message;

struct_message outgoingMsg;
struct_message incomingMsg;
esp_now_peer_info_t peerInfo;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingMsg, incomingData, sizeof(incomingMsg));
  
  if (incomingMsg.type == 'A') {
    // 1. Tell Python the status changed
    Serial.println("STATUS:ALARM"); 

    // 2. Trigger Alt + Tab
    if(bleKeyboard.isConnected()) {
      bleKeyboard.press(KEY_LEFT_ALT);
      bleKeyboard.press(KEY_TAB);
      delay(100);
      bleKeyboard.releaseAll();
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);
  // Force Channel 1
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) return;

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0; peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  bleKeyboard.begin();
}

void loop() {
  // Listen for Python Commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    outgoingMsg.type = 'C';
    
    if (cmd == '1') {
      outgoingMsg.val = 1;
      esp_now_send(broadcastAddress, (uint8_t *) &outgoingMsg, sizeof(outgoingMsg));
      Serial.println("STATUS:ARMED"); // Confirm back to Python
    }
    
    if (cmd == '0') {
      outgoingMsg.val = 0;
      esp_now_send(broadcastAddress, (uint8_t *) &outgoingMsg, sizeof(outgoingMsg));
      Serial.println("STATUS:DISARMED"); // Confirm back to Python
    }
  }
  delay(10);
}