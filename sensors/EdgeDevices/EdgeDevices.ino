#include <WiFi.h>
#include <WebSocketsClient.h>

const char *ssid = "TP-Link_1011";
const char *password = "18245233";

const char* websockets_server_host = "192.168.0.132";
const uint16_t websockets_server_port = 80;

WebSocketsClient client;

unsigned long previousMillis = 0;
const unsigned long interval = 100;
int counter = 0;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.begin(websockets_server_host, websockets_server_port, "/");
  client.onEvent(onWebSocketEvent);
}

void loop() {
  client.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    counter++;
    String intStr = String(counter);
    Serial.println(intStr);
    client.sendTXT(intStr);
  }
}

void onWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("Connected to WebSocket server");
      break;

    case WStype_DISCONNECTED:
      Serial.println("Disconnected from WebSocket server");
      break;

    case WStype_TEXT:
      Serial.print("Received text: ");
      Serial.println((const char *) payload);
      break;

    default:
      break;
  }
}
