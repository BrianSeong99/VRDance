#include <HTTPClient.h>
#include "MPU9250.h"
#include "eeprom_utils.h"
#include <time.h>

String deviceLocation = "RUpperArm";
String deviceLocation = "RForeArm";


// an MPU9250 object with the MPU-9250 sensor on I2C bus 0 with address 0x68
MPU9250 mpu;

// const char* ssid = "TP-Link_1011";
// const char* password = "18245233";
const char* ssid = "Robotics2.4G";
const char* password = "quickboat917";

unsigned int previousTimeLocalMillis = 0; // Variable to store the previous loop time
bool secondBaseChanged = false;
time_t currentTime;

unsigned long pMillis = 0;
unsigned long interval = 250;

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

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("Wifi Connected");

  configTime(0, 0, "pool.ntp.org");
  while (time(nullptr) < 1631160000) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("Time Connected");
}

void loop() {
   unsigned long cMillis = millis();
  if (cMillis - pMillis >= interval)
  {
    pMillis = cMillis;
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
      HTTPClient http;

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
      deltaTimeLocalMillis = roundToNearest250(deltaTimeLocalMillis);
      deltaTimeLocalMillis = deltaTimeLocalMillis < 1000 ? deltaTimeLocalMillis : 750;
      if (secondBaseChanged) {
        previousTimeLocalMillis = currentLocalTimeMillis; // Update the previous time for the next loop
        secondBaseChanged = false;
      }
      String currentTimeMillis = String(currentTime) + String(deltaTimeLocalMillis);
      // Serial.print("currentTime: ");
      // Serial.println(currentTime);
      // Serial.print("deltaTimeMillis: ");
      // Serial.println(deltaTimeLocalMillis);
      // Serial.print("currentTimeMillis: ");
      // Serial.println(currentTimeMillis);

      http.begin("https://vrdance-b9bde-default-rtdb.firebaseio.com/RightArm/" + currentTimeMillis + "/" + deviceLocation + ".json");
      http.addHeader("Content-Type", "application/json");
      String postData = "{\"AX\":" + String(AX) + ", \"AY\":" + String(AY) + ", \"AZ\": " + String(AZ) + ", \"GX\": " + String(GX) + ", \"GY\": " + String(GY) + ", \"GZ\": " + String(GZ) + ", \"MX\": " + String(MX) + ", \"MY\": " + String(MY) + ", \"MZ\": " + String(MZ) + ", \"LAX\": " + String(LAX) + ", \"LAY\": " + String(LAY) + ", \"LAZ\": " + String(LAZ) + "}";

      int httpResponseCode = http.PUT(postData);
      if (httpResponseCode > 0) {
        Serial.printf("HTTP POST request successful, response code: %d\n", httpResponseCode);
      } else {
        Serial.printf("HTTP POST request failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
      }
      http.end();
    }
  }
}

unsigned int roundToNearest250(unsigned int number)
{
  unsigned int remainder = number % 250;
  unsigned int result = number - remainder;
  
  if (remainder >= 125)
  {
    result += 250;
  }
  
  return result;
}


