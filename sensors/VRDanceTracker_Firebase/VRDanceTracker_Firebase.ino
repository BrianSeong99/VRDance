#include "MPU9250.h"
#include "eeprom_utils.h"
#include <time.h>

#include <WiFi.h>
#include <FirebaseESP32.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "Robotics2.4G"
#define WIFI_PASSWORD "quickboat917"

// For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino

// Right ForeArm
String deviceLocation = "RForeArm";
#define API_KEY "AIzaSyD4CZdOeyDZ_U-slopVkP0ZIf6hH4z0QNM"
#define DATABASE_URL "https://vrdance-rforearm-default-rtdb.firebaseio.com/"

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "xdarcy96@uw.edu"
#define USER_PASSWORD "1234567890"

// Define Firebase Data object
FirebaseData fbdo;
FirebaseData stream;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

volatile bool dataChanged = false;

String childPath[1] = {deviceLocation};


// an MPU9250 object with the MPU-9250 sensor on I2C bus 0 with address 0x68
MPU9250 mpu;

unsigned int previousTimeLocalMillis = 0; // Variable to store the previous loop time
bool secondBaseChanged = false;
time_t currentTime;

unsigned long pMillis = 0;
unsigned long interval = 250;

void streamCallback(MultiPathStreamData stream)
{
  size_t numChild = sizeof(childPath) / sizeof(childPath[0]);

  for (size_t i = 0; i < numChild; i++)
  {
    if (stream.get(childPath[i]))
    {
      Serial.printf("path: %s, event: %s, type: %s, value: %s%s", stream.dataPath.c_str(), stream.eventType.c_str(), stream.type.c_str(), stream.value.c_str(), i < numChild - 1 ? "\n" : "");
    }
  }

  Serial.println();

  // This is the size of stream payload received (current and max value)
  // Max payload size is the payload size under the stream path since the stream connected
  // and read once and will not update until stream reconnection takes place.
  // This max value will be zero as no payload received in case of ESP8266 which
  // BearSSL reserved Rx buffer size is less than the actual stream payload.
  Serial.printf("Received stream payload size: %d (Max. %d)\n\n", stream.payloadLength(), stream.maxPayloadLength());

  // Due to limited of stack memory, do not perform any task that used large memory here especially starting connect to server.
  // Just set this flag and check it status later.
  dataChanged = true;
}

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
    Serial.println("stream timed out, resuming...\n");

  if (!stream.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
}


void setup() {
  // serial to display data
  Serial.begin(115200);
  Wire.begin();
  while(!Serial) {}

  // start communication with IMU 
  if (!mpu.setup(0x68)) {  // change to your own address
    while (1) {
      Serial.println("MPU connection failed. Please check your connection with `connection_check` example.");
      delay(5000);
    }
  }

#if defined(ESP_PLATFORM) || defined(ESP8266)
  EEPROM.begin(0x80);
#endif
//   // // setting the accelerometer full scale range to +/-8G 
//   // IMU.setAccelRange(MPU9250::ACCEL_RANGE_8G);
//   // // setting the gyroscope full scale range to +/-500 deg/s
//   // IMU.setGyroRange(MPU9250::GYRO_RANGE_500DPS);
//   // // setting DLPF bandwidth to 20 Hz
//   // IMU.setDlpfBandwidth(MPU9250::DLPF_BANDWIDTH_20HZ);
//   // // setting SRD to 19 for a 50 Hz update rate
//   // IMU.setSrd(19);

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

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);

  Firebase.reconnectWiFi(true);

  // if (!Firebase.beginMultiPathStream(stream, parentPath)) {
  //   Serial.printf("stream begin error, %s\n\n", stream.errorReason().c_str());
  // }

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
  if (Firebase.ready() && (cMillis - sendDataPrevMillis > 240 || sendDataPrevMillis == 0))
  {
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
      // Serial.println(IMU.getTemperature_C(),6);

      // get current timestamp in seconds
      time_t now = time(nullptr);
      if (now != currentTime) {
        secondBaseChanged = true;
        currentTime = now;
        // Serial.println("secondBaseChanged");
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
      // Serial.print("currentTime: ");
      // Serial.println(currentTime);
      // Serial.print("deltaTimeMillis: ");
      // Serial.println(deltaTimeLocalMillis);
      // Serial.print("currentTimeMillis: ");
      // Serial.println(currentTimeMillis);

      Serial.print("\nSet json...");

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

      // TODO
      // String parentPath = "RightArm/" + currentTimeMillis + "/" + deviceLocation;

      Firebase.setJSONAsync(fbdo, currentTimeMillis, json);
    }
  }
}

String roundToNearest250(unsigned int number)
{
  // Serial.print("before round: "); Serial.println(number);
  // unsigned int remainder = number % 250;
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


  // Serial.print("after round: "); Serial.println(result); Serial.println();


  return result;
}


