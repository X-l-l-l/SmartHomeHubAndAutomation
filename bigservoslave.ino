#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Servo.h>

// Structure to receive data
typedef struct struct_message {
    int onoff;
} struct_message;
const uint8_t servoPin = D1;
// Create a struct_message called myData
struct_message myData;
int a = 0;
Servo servo;
// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("OnOff: ");
  Serial.println(myData.onoff);
  //Stationary servo
  if (myData.onoff == 0)
    servo.write(90);
  //Servo turn clockwise
  if (myData.onoff == 1)
    servo.write(180);
  //servo turn counter clockwise
  if (myData.onoff == 2)
    servo.write(0);
}
 
void setup() {
  Serial.begin(115200);
  servo.attach(servoPin);
  servo.write(90);
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to get recv packer info
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
}
