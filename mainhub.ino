#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

String r,g,b;

// Network credentials
const char* ssid = "XIII PC";
const char* password = "rares3001";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>NodeHUB</h2>
  %BUTTONPLACEHOLDER%
<script>
function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
  xhr.send();
}

function color(element){
  var hex = element.value;
  var r = parseInt(hex.slice(1, 3), 16);
  var g = parseInt(hex.slice(3, 5), 16);
  var b = parseInt(hex.slice(5, 7), 16);
  var url = "/color?R="+r+"&G="+g+"&B="+b; 

  var xhr = new XMLHttpRequest();
  xhr.open("GET", url, true);
  xhr.send();
}

</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    buttons += "<h4>SmallServo</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"1\" " + outputState(1) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>BigServoRight</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"2\" " + outputState(2) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>BigServoLeft</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"3\" " + outputState(3) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>LED</h4><label><input type=\"color\" onchange=\"color(this)\" id=\"color\" /></label>";

    return buttons;
  }
  return String();
}

String outputState(int output){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
}


//=============================================

// RECEIVER MAC Address
uint8_t broadcastAddressSmall[] = {0x4C, 0x75, 0x25, 0x38, 0xCB, 0x77};
uint8_t broadcastAddressBig[] = {0xBC, 0xFF, 0x4D, 0xCF, 0xC0, 0x97};
// Structure to send data
typedef struct struct_message {
  int onoff;
} struct_message;

int onoff = 0;

// Create a struct_message called myData
struct_message myData;

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}

void setup() {
  
  // Init Serial Monitor
  Serial.begin(115200);
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddressSmall, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  esp_now_add_peer(broadcastAddressBig, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1;
    String inputMessage2;
    if (request->hasParam("output") && request->hasParam("state")) {
      inputMessage1 = request->getParam("output")->value();
      inputMessage2 = request->getParam("state")->value();
      myData.onoff = inputMessage2.toInt();
      if (inputMessage1.toInt()==1){
        esp_now_send(broadcastAddressSmall, (uint8_t *) &myData, sizeof(myData));
        Serial.println("Small");}
      if (inputMessage1.toInt()==2){
        esp_now_send(broadcastAddressBig, (uint8_t *) &myData, sizeof(myData));
        Serial.println("Big");}
      if (inputMessage1.toInt()==3){
        myData.onoff*=2;
        esp_now_send(broadcastAddressBig, (uint8_t *) &myData, sizeof(myData));
        Serial.println("Big");}
    }
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    Serial.print("GPIO: ");
    Serial.print(inputMessage1);
    Serial.print(" - Set to: ");
    Serial.println(inputMessage2);
    request->send(200, "text/plain", "OK");
  });
  // Send a GET request to <ESP_IP>/color?R=<r>&G=<g>&B=<b>
  server.on("/color", HTTP_GET, [](AsyncWebServerRequest *request) {
    if(request->hasParam("R") && request->hasParam("G") && request->hasParam("B")) {
      r = request->getParam("R")->value();
      g = request->getParam("G")->value();
      b = request->getParam("B")->value();
      Serial.print("r=");
      Serial.println(r);
      Serial.print("g=");
      Serial.println(g);
      Serial.print("b=");
      Serial.println(b);
    }
    request->send(200, "text/plain", "OK");
  });

  // Start server
  server.begin();
}
 
void loop() {
   // Send values to RGB led
   analogWrite(D5,r.toInt());
   analogWrite(D4,g.toInt());
   analogWrite(D3,b.toInt());
}
