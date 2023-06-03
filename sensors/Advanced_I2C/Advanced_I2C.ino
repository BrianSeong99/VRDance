#include <HTTPClient.h>
#include "MPU9250.h"
#include <time.h>

String deviceLocation = "RArm";

// an MPU9250 object with the MPU-9250 sensor on I2C bus 0 with address 0x68
MPU9250 IMU(Wire,0x68);
int status;

int motorPin = 4; //motor transistor is connected to pin 3

// const char* ssid = "TP-Link_1011";
// const char* password = "18245233";
const char* ssid = "Robotics2.4G";
const char* password = "quickboat917";

int count = 0;

const std::string access_token = "k80ktrhq8yA534kpQBJzCNrjPhdYXTrTZKofZR5B";

void setup() {
  // serial to display data
  Serial.begin(115200);
  while(!Serial) {}

  // start communication with IMU 
  status = IMU.begin();
  if (status < 0) {
    Serial.println("IMU initialization unsuccessful");
    Serial.println("Check IMU wiring or try cycling power");
    Serial.print("Status: ");
    Serial.println(status);
    while(1) {}
  }
  // setting the accelerometer full scale range to +/-8G 
  IMU.setAccelRange(MPU9250::ACCEL_RANGE_8G);
  // setting the gyroscope full scale range to +/-500 deg/s
  IMU.setGyroRange(MPU9250::GYRO_RANGE_500DPS);
  // setting DLPF bandwidth to 20 Hz
  IMU.setDlpfBandwidth(MPU9250::DLPF_BANDWIDTH_20HZ);
  // setting SRD to 19 for a 50 Hz update rate
  IMU.setSrd(19);

  pinMode(motorPin, OUTPUT);

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
  // read the sensor
  IMU.readSensor();

  // display the data
  Serial.print(IMU.getAccelX_mss(), 6);
  Serial.print("\t");
  Serial.print(IMU.getAccelY_mss(), 6);
  Serial.print("\t");
  Serial.print(IMU.getAccelZ_mss(), 6);
  Serial.print("\t");
  Serial.print(IMU.getGyroX_rads(), 6);
  Serial.print("\t");
  Serial.print(IMU.getGyroY_rads(), 6);
  Serial.print("\t");
  Serial.print(IMU.getGyroZ_rads(), 6);
  Serial.print("\t");
  Serial.print(IMU.getMagX_uT(), 6);
  Serial.print("\t");
  Serial.print(IMU.getMagY_uT(), 6);
  Serial.print("\t");
  Serial.println(IMU.getMagZ_uT(), 6);

  String AX = String(IMU.getAccelX_mss());
  String AY = String(IMU.getAccelY_mss());
  String AZ = String(IMU.getAccelZ_mss());
  String GX = String(IMU.getGyroX_rads());
  String GY = String(IMU.getGyroY_rads());
  String GZ = String(IMU.getGyroZ_rads());
  String MX = String(IMU.getMagX_uT());
  String MY = String(IMU.getMagY_uT());
  String MZ = String(IMU.getMagZ_uT());
  // Serial.println(IMU.getTemperature_C(),6);
  delay(20);

  HTTPClient http;

  time_t now = time(nullptr);
  String currentTime = String(now);

  http.begin("https://vrdance-b9bde-default-rtdb.firebaseio.com/" + deviceLocation + ".json");
  http.addHeader("Content-Type", "application/json");
  String postData = "{\"AX\": \"" + AX + "\", \"AY\": \"" + AY + "\", \"AZ\": \"" + AZ + "\", \"GX\": \"" + GX + "\", \"GY\": \"" + GY + "\", \"GZ\": \"" + GZ + "\", \"MX\": \"" + MX + "\", \"MY\": \"" + MY + "\", \"MZ\": \"" + MZ + "\"}";

  int httpResponseCode = http.PUT(postData);
  if (httpResponseCode > 0) {
    Serial.printf("HTTP POST request successful, response code: %d\n", httpResponseCode);
  } else {
    Serial.printf("HTTP POST request failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
  }
  http.end();

  digitalWrite(motorPin, HIGH); //vibrate
  delay(100);  // delay one second
  digitalWrite(motorPin, LOW);  //stop vibrating
  delay(100); //wait 50 seconds.
}
