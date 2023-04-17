#include <WiFi.h>
#include <WebSocketsServer.h>

const char *ssid = "TP-Link_1011";
const char *password = "18245233";

const uint16_t server_port = 80;

WebSocketsServer server(server_port);

int current_buffer[10];

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  server.onEvent(onWebSocketEvent);
}

void loop() {
  server.loop();
}

void onWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("Client %u connected\n", client_num);
      break;

    case WStype_DISCONNECTED:
      Serial.printf("Client %u disconnected\n", client_num);
      break;

    case WStype_TEXT: {
      int receivedInt = atoi((const char *) payload);
      // Serial.printf("Client %u sent integer: %d\n", client_num, receivedInt);
      current_buffer[client_num] = receivedInt;
      Serial.printf("current_buffer %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n", 
        current_buffer[0], current_buffer[1], current_buffer[2], current_buffer[3], current_buffer[4],
        current_buffer[5], current_buffer[6], current_buffer[7], current_buffer[8], current_buffer[9]
      );
      break;
    }
    default:
      break;
  }
}
