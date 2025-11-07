#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#define ACCEPT_TYPE "text/csv"
WiFiClient client;
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_ADXL345_U.h>

Adafruit_ADXL345_Unified adxl = Adafruit_ADXL345_Unified(12345);
//================================================================//
//-----------------------dallas-------------------------------
// GPIO where the DS18B20 is connected
const int oneWireBus = 4;  // You can change this to any GPIO pin

// Setup a oneWire instance to communicate with the DS18B20
OneWire oneWire(oneWireBus);

// Pass the oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);
//-----------------------------fuel----------------------
#define FUEL_SENSOR_PIN 32  // Analog pin where the fuel level sensor is connected
#define VREF 5.0 
//-----------------------------rpm-------------------------
#define SENSOR_PIN 34  // Adjust based on your wiring
volatile int pulseCount = 0;
unsigned long lastTime = 0;
float rpm = 0;

void IRAM_ATTR countPulse() {
  pulseCount++;  // Increase count on each sensor trigger
}
//-------------------------------mq2-----------------------------
#define MQ2_PIN 35  
//=================================================================//
const char* ssid = "GLA-CSED";  
const char* password = "GLACAMPUS";
const char* host="dcs.glaitm.org";
const int httpPort=7080;

const char appKey[]= "b5df2ae4-25ff-492a-a5c2-48efc180ad3a";
const char Thing[] = "smart_generator_iiot_lab_thing";
const char Property1[] = "Genset_Temp";
const char Property2[] = "Genset_Fuel_Level";
const char Property3[] = "ADXL_X";
const char Property4[] = "ADXL_Y";
const char Property5[] = "ADXL_Z";
const char Property6[] = "MQ2_Status";
const char Property7[] = "RPM";
const char Property8[] = "Current";
const char Property9[] = "Voltage";
const char Property10[] = "Power";
const char Property11[] = "Power_Real";
const char Property12[] = "Energy";
const char Property13[] = "Freequency";
const char Property14[] = "Power_Factor";

void setup()
{
  // --- Serial Monitor ---
  Serial.begin(115200);
  //----------------------dallas------------------------------
  // Initialize the DS18B20 sensor
  sensors.begin();
  //-----------------------------adxl sick-----------------
  if (!adxl.begin()) {
        Serial.println("ADXL345 not detected. Check connections!");
        while (1);
    }

    adxl.setRange(ADXL345_RANGE_16_G); // Set range to ±16g (adjustable)

    Serial.println("ADXL345 Initialized.");

  //--------------------------------rpm----------------------------
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), countPulse, FALLING);
  
  Serial.println();
  Serial.print("connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
{
    delay(500);
    Serial.print(".");
}
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void Put(String ThingName, String ThingProperty, float Value)
  {
    Serial.println(host);
  if (!client.connect(host, httpPort))
{
    Serial.println("connection failed");
    return;
  } else
{
    Serial.println("Connected to ThingWorx.");
  }
  String url = "/Thingworx/Things/"+ThingName+"/Properties/"+ThingProperty;
  Serial.print("requesting URL: ");
  Serial.println(url);
  String strPUTReqVal = "{\""+ThingProperty+"\":\""+Value+"\"}";  
  Serial.print("PUT Value: ");
  Serial.println(strPUTReqVal);
  client.print(String("PUT ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "appKey:"+ appKey + "\r\n" +
               "x-thingworx-session: false" + "\r\n" +
               "Accept: application/json" + "\r\n" +
               "Connection: close" + "\r\n" +
               "Content-Type: application/json" + "\r\n" +
               "Content-Length: " + String(strPUTReqVal.length()) + "\r\n\r\n" +
               strPUTReqVal + "\r\n\r\n");  
while (client.connected())
{
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  client.stop();
}
void loop()
{
  //---------------------------dallas
  // Request temperature readings from the sensor
  sensors.requestTemperatures();

  // Get the temperature in Celsius
  float temperatureC = sensors.getTempCByIndex(0);

  // Print the temperature to the Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(temperatureC);
  Serial.println(" °C");
  //--------------------------fuel
  // === Fuel Level Sensor ===
  int rawValue = analogRead(FUEL_SENSOR_PIN);             // Read raw ADC value (0–1023)
  float voltage = rawValue * (VREF / 1023.0);              // Convert to voltage

  // Example mapping (adjust according to your sensor)
  // Assuming 0V = empty tank, 5V = full tank (adjust if your range is different)
  float fuelPercentage = map(rawValue, 4095, 1830, 0, 100);  // Calibrated range

  // Clamp values
  fuelPercentage = constrain(fuelPercentage, 0, 100);

  // Print to Serial Monitor
  Serial.print("Raw ADC: ");
  Serial.print(rawValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 2);
  Serial.print(" V | Fuel Level: ");
  Serial.print(fuelPercentage);
  Serial.println(" %");
  delay(1000);
  //------------------------------------------
  sensors_event_t event;
    adxl.getEvent(&event);
    long adxl_x=event.acceleration.x;
    long adxl_y=event.acceleration.y;
    long adxl_z=event.acceleration.z;
    Serial.print("X: "); Serial.print(adxl_x); Serial.print(" m/s² ");
    Serial.print("Y: "); Serial.print(adxl_y); Serial.print(" m/s² ");
    Serial.print("Z: "); Serial.print(adxl_z); Serial.println(" m/s²");

    delay(500); // Adjust for vibration monitoring frequency
    //------------------------mq2---------------// === MQ2 ===
  int sensorValue = analogRead(MQ2_PIN);
  float voltage_mq = sensorValue * (3.3 / 4095.0);

  Serial.print("Smoke Level (ADC Value): ");
  Serial.print(sensorValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage_mq);
  Serial.println(" V");
  //------------------------rpm--------------------------
  unsigned long currentTime = millis();
  
  if (currentTime - lastTime >= 1000) {  // Update every second
    rpm = (pulseCount * 60);  // Assuming 1 pulse per revolution
    Serial.print("RPM: ");
    Serial.println(rpm);
    
    pulseCount = 0;  // Reset count
    lastTime = currentTime;
  }
  /*
  String dbj=(sensorValue > 2000) ? "Smoke Detected" : "Normal";



  Serial.print("Status: ");
  Serial.println(dbj);
*/






  delay(1000);
    Put(Thing,Property1,temperatureC);
    delay(1000) ;
    Put(Thing,Property2,fuelPercentage);
    delay(1000) ;
    Put(Thing,Property3,adxl_x);
    delay(1000) ;
    Put(Thing,Property4,adxl_y);
    delay(1000) ;
    Put(Thing,Property5,adxl_z);
    delay(1000) ;
    Put(Thing,Property6, sensorValue);
    delay(1000) ;
    Put(Thing,Property7, rpm);
    delay(1000) ;
    Put(Thing,Property8, 9);
    delay(1000) ;
    Put(Thing,Property9, 230);
    delay(1000) ;
    Put(Thing,Property10, 25);
    delay(1000) ;
    Put(Thing,Property11, 20);
    delay(1000) ;
    Put(Thing,Property12, 20);
    delay(1000) ;
    Put(Thing,Property13, 49.5);
    delay(1000) ;
    Put(Thing,Property14, 0.8);
    delay(1000) ;
 
}
