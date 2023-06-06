#include "MPU9250.h"
#include "eeprom_utils.h"
#include <time.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// #define WIFI_SSID "Robotics2.4G"
// #define WIFI_PASSWORD "quickboat917"
#define WIFI_SSID "TP-Link_1011"
#define WIFI_PASSWORD "18245233"

// Right UpperArm
String deviceLocation = "RUpperArm";
#define DATABASE_URL "https://vrdance-rupperarm-default-rtdb.firebaseio.com/"

#define API_KEY "AIzaSyBrjbzDvey1S47C_K0QEurTu1KaAm7wlGc"

#define USER_EMAIL "xdarcy96@uw.edu"
#define USER_PASSWORD "1234567890"

FirebaseData fbdo;
FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
volatile bool dataChanged = false;
String childPath[1] = {deviceLocation};

MPU9250 mpu;

unsigned int previousTimeLocalMillis = 0; // Variable to store the previous loop time
bool secondBaseChanged = false;
time_t currentTime;

unsigned long pMillis = 0;
unsigned long interval = 250;

void streamCallback(MultiPathStreamData stream) {
  size_t numChild = sizeof(childPath) / sizeof(childPath[0]);

  for (size_t i = 0; i < numChild; i++) {
    if (stream.get(childPath[i])) {
      Serial.printf("path: %s, event: %s, type: %s, value: %s%s", stream.dataPath.c_str(), stream.eventType.c_str(), stream.type.c_str(), stream.value.c_str(), i < numChild - 1 ? "\n" : "");
    }
  }

  Serial.println();
  Serial.printf("Received stream payload size: %d (Max. %d)\n\n", stream.payloadLength(), stream.maxPayloadLength());

  dataChanged = true;
}

void streamTimeoutCallback(bool timeout) {
  if (timeout)
    Serial.println("stream timed out, resuming...\n");

  if (!stream.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
}


void setup() {
  Serial.begin(115200);
  Wire.begin();
  while(!Serial) {}

  if (!mpu.setup(0x68)) {
    while (1) {
      Serial.println("MPU connection failed. Please check your connection with `connection_check` example.");
      delay(5000);
    }
  }

#if defined(ESP_PLATFORM) || defined(ESP8266)
  EEPROM.begin(0x80);
#endif

  loadCalibration();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Firebase.setMultiPathStreamCallback(stream, streamCallback, streamTimeoutCallback);

  configTime(0, 0, "pool.ntp.org");
  while (time(nullptr) < 1631160000) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("Time Connected");
}

void loop() {
   unsigned long cMillis = millis();
  if (Firebase.ready() && (cMillis - sendDataPrevMillis > 240 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis  = cMillis;
    // read the sensor
    if (mpu.update()) {
      float AX = mpu.getAccX();
      float AY = mpu.getAccY();
      float AZ = mpu.getAccZ();
      float GX = mpu.getGyroX();
      float GY = mpu.getGyroY();
      float GZ = mpu.getGyroZ();
      float MX = mpu.getMagX();
      float MY = mpu.getMagY();
      float MZ = mpu.getMagZ();
      float LAX = mpu.getLinearAccX();
      float LAY = mpu.getLinearAccY();
      float LAZ = mpu.getLinearAccZ();

      // get current timestamp in seconds
      time_t now = time(nullptr);
      if (now != currentTime) {
        secondBaseChanged = true;
        currentTime = now;
      }

      // calculate the milliseconds passed by during each second and round it to 50 milliseconds
      unsigned int currentLocalTimeMillis = millis(); // Get the current time in milliseconds
      unsigned int deltaTimeLocalMillis = currentLocalTimeMillis - previousTimeLocalMillis; // Calculate the delta time
      if (secondBaseChanged) {
        previousTimeLocalMillis = currentLocalTimeMillis; // Update the previous time for the next loop
        secondBaseChanged = false;
      }
      String rounded = roundToNearest250(deltaTimeLocalMillis);
      String currentTimeMillis = String(currentTime) + rounded;

      FirebaseJson json;
      json.set("AX", String(AX));
      json.set("AY", String(AY));
      json.set("AZ", String(AZ));
      json.set("GX", String(GX));
      json.set("GY", String(GY));
      json.set("GZ", String(GZ));
      json.set("MX", String(MX));
      json.set("MY", String(MY));
      json.set("MZ", String(MZ));
      json.set("LAX", String(LAX));
      json.set("LAY", String(LAY));
      json.set("LAZ", String(LAZ));

      // Serial.print("\nSet json...");
      // Firebase.setJSONAsync(fbdo, currentTimeMillis, json);

      Serial.print("\nPatch json...");
      Firebase.updateNodeSilentAsync(fbdo, "/latest", json);
    }
  }
}

String roundToNearest250(unsigned int number)
{
  String result;

  if (number >= 0 && number < 250) {
    result = "000";
  } else if (number >= 250 && number < 500) {
    result = "250";
  } else if (number >= 500 && number < 750) {
    result = "500";
  } else {
    result = "750";
  }

  return result;
}


