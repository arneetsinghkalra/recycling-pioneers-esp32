/**
 * Recycling Pioneers
 * Adapted from various resources by Arneet Kalra
 * 
 * Libraries used for this sketch:
 * 
 * Ultrasonic sensor uses NewPing library
 * https://bitbucket.org/teckel12/arduino-new-ping/wiki/Home
 * HX711_ADC library to connect with load cell amplifier
 * Wifi Library for esp32
 * Time Library 
 * 
 * IMPORTANT: Please consult the README.md file to know which lines of code to change when setting up each sensor.
 */

#include <WiFi.h>
#include <NewPing.h>
#include <FirebaseESP32.h>
#include <HX711_ADC.h>
#include "time.h"
#include "arduino_secrets.h"

/*
 * The following code has been placed in a secret file for security reasons. The template follows as below:
#define SECRET_FIREBASE_HOST "firebase host here"
#define SECRET_FIREBASE_AUTH "firebase authentication here"
#define SECRET_WIFI_SSID "wifi name here"
#define SECRET_WIFI_PASSWORD "wifi password here"
*/


//Pins required for Distance Sensors: MAKE SURE THEY ARE CONNECTED AT THE RIGHT GPIO PINS

#define TRIGGER_PIN_WASTE_1  23  // Arduino pin tied to trigger pin on the ultrasonic sensor
#define ECHO_PIN_WASTE_1     23  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define TRIGGER_PIN_WASTE_2  22  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_WASTE_2     22  // Arduino pin tied to echo pin on the ultrasonic sensor.

#define TRIGGER_PIN_COMPOST_1 19 // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_COMPOST_1    19   // Arduino pin tied to echo pin on the ultrasonic sensor.
#define TRIGGER_PIN_COMPOST_2  18  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_COMPOST_2     18  // Arduino pin tied to echo pin on the ultrasonic sensor.

#define TRIGGER_PIN_PLASTIC_1  32 // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_PLASTIC_1     32  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define TRIGGER_PIN_PLASTIC_2  33// Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_PLASTIC_2     33  // Arduino pin tied to echo pin on the ultrasonic sensor.

#define TRIGGER_PIN_RECYCLE_1  25  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_RECYCLE_1     25  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define TRIGGER_PIN_RECYCLE_2  26 // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_RECYCLE_2     26  // Arduino pin tied to echo pin on the ultrasonic sensor.


//Pins for Load Sensors
#define DOUT_WASTE 5
#define CLK_WASTE 4

#define DOUT_COMPOST 15
#define CLK_COMPOST 17

#define DOUT_PLASTIC 27
#define CLK_PLASTIC  14

#define DOUT_RECYCLE 34
#define CLK_RECYCLE 35

#define MAX_DISTANCE 300 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
#define UPDATE_INTERVAL 600000 //Time in milliseconds between updating firebase database (10 min = 600000)

//IMPORTANT- UPDATE THIS EVERYTIME: Name of node in firebase data ***********************************
const String BIN_NAME = "trottier1050";
const String BUILDING_NAME = "TROT";
const String FACULTY_NAME = "ECSE";
const int FLOOR_NUMBER = 1;
const String CLOSEST_ROOM = "1050";

//THE FOLLOWING IS AN EXAMPLE: FOLLOW THIS TEMPLATE
/*

*/
//**************************************************************************************************

const char *FIREBASE_HOST = SECRET_FIREBASE_HOST;
const char *FIREBASE_AUTH = SECRET_FIREBASE_AUTH;
const char *WIFI_SSID = SECRET_WIFI_SSID;
const char *WIFI_PASSWORD = SECRET_WIFI_PASSWORD;


//Information for time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -18000; //Offset of 18000s (5 hours) for our timezone
const int   daylightOffset_sec = 3600; //Daylight saving offset one hour

//Fields required for Firebase
FirebaseData firebaseData;
FirebaseJson jsonAllData;

//Fields required for weight
float calibrationValue_WASTE = 20764.00; //Custom calibration value for our sensor
float calibrationValue_PLASTIC = 20764.00; //Custom calibration value for our sensor
float calibrationValue_COMPOST = 20764.00; //Custom calibration value for our sensor
float calibrationValue_RECYCLE = 20764.00; //Custom calibration value for our sensor

float weightData_WASTE = 0.00;
float ave_WASTE;
float weight_history_WASTE[4];
float weightData_PLASTIC = 0.00;
float ave_PLASTIC;
float weight_history_PLASTIC[4];
float weightData_COMPOST = 0.00;
float ave_COMPOST;
float weight_history_COMPOST[4];
float weightData_RECYCLE = 0.00;
float ave_RECYCLE;
float weight_history_RECYCLE[4];

//Fields required for distance
int distanceData_WASTE = 0;
int distanceData_PLASTIC = 0;
int distanceData_COMPOST = 0;
int distanceData_RECYCLE = 0;

//Field required for Time
char createdAt[30]; //Char array for time field
unsigned long previousMillis = 0; //Last time updated


NewPing sonar_waste_1(TRIGGER_PIN_WASTE_1, ECHO_PIN_WASTE_1, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing sonar_waste_2(TRIGGER_PIN_WASTE_2, ECHO_PIN_WASTE_2, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

NewPing sonar_plastic_1(TRIGGER_PIN_WASTE_1, ECHO_PIN_WASTE_1, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing sonar_plastic_2(TRIGGER_PIN_WASTE_2, ECHO_PIN_WASTE_2, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

NewPing sonar_compost_1(TRIGGER_PIN_WASTE_1, ECHO_PIN_WASTE_1, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing sonar_compost_2(TRIGGER_PIN_WASTE_2, ECHO_PIN_WASTE_2, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

NewPing sonar_recycle_1(TRIGGER_PIN_WASTE_1, ECHO_PIN_WASTE_1, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing sonar_recycle_2(TRIGGER_PIN_WASTE_2, ECHO_PIN_WASTE_2, MAX_DISTANCE); // NewPing setup of pins and maximum distance.


HX711_ADC LoadCell_WASTE(DOUT_WASTE,CLK_WASTE);   //HX711 constructor (dout pin, sck pin)
HX711_ADC LoadCell_PLASTIC(DOUT_PLASTIC,CLK_PLASTIC);   //HX711 constructor (dout pin, sck pin)
HX711_ADC LoadCell_COMPOST(DOUT_COMPOST,CLK_COMPOST);   //HX711 constructor (dout pin, sck pin)
HX711_ADC LoadCell_RECYCLE(DOUT_RECYCLE,CLK_RECYCLE);   //HX711 constructor (dout pin, sck pin)

void connectToFirebaseDatabase(){
  //Connect to firebase database
  Serial.println("Beginning Connection with Firebase Realtime Database...");
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Serial.println("Connected to Firebase Database: Welcome");
}

void initWeightSensors(){
  // Set up the HX711 library
  Serial.println("Initializing Load Sensors...");
  LoadCell_WASTE.begin();
  LoadCell_PLASTIC.begin();
  LoadCell_COMPOST.begin();
  LoadCell_RECYCLE.begin();
  
  long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell_WASTE.start(stabilizingtime, _tare);
  if (LoadCell_WASTE.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell_WASTE.setCalFactor(calibrationValue_WASTE); // set calibration value (float)
    Serial.println("Load Sensors Startup & Tare: Online");
  }

  LoadCell_PLASTIC.start(stabilizingtime, _tare);
  if (LoadCell_PLASTIC.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell_PLASTIC.setCalFactor(calibrationValue_PLASTIC); // set calibration value (float)
    Serial.println("Load Sensors Startup & Tare: Online");
  }

  LoadCell_COMPOST.start(stabilizingtime, _tare);
  if (LoadCell_COMPOST.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell_COMPOST.setCalFactor(calibrationValue_COMPOST); // set calibration value (float)
    Serial.println("Load Sensors Startup & Tare: Online");
  }

  LoadCell_RECYCLE.start(stabilizingtime, _tare);
  if (LoadCell_RECYCLE.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell_RECYCLE.setCalFactor(calibrationValue_RECYCLE); // set calibration value (float)
    Serial.println("Load Sensors Startup & Tare: Online");
  }
}

int getDistanceDataWaste(){
  int distance1 = sonar_waste_1.convert_cm(sonar_waste_1.ping_median(10)); // read distance from ultrasonic sensor (using median of 10 iterations)
  int distance2 = sonar_waste_2.convert_cm(sonar_waste_2.ping_median(10)); // read distance from ultrasonic sensor (using median of 10 iterations)
  int finalDistance = (distance1+distance2)/2;
  return finalDistance; 
}

int getDistanceDataPlastic(){
  int distance1 = sonar_plastic_1.convert_cm(sonar_plastic_1.ping_median(10)); // read distance from ultrasonic sensor (using median of 10 iterations)
  int distance2 = sonar_plastic_2.convert_cm(sonar_plastic_2.ping_median(10)); // read distance from ultrasonic sensor (using median of 10 iterations)
  int finalDistance = (distance1+distance2)/2;
  return finalDistance; 
}

int getDistanceDataCompost(){
  int distance1 = sonar_compost_1.convert_cm(sonar_compost_1.ping_median(10)); // read distance from ultrasonic sensor (using median of 10 iterations)
  int distance2 = sonar_compost_2.convert_cm(sonar_compost_2.ping_median(10)); // read distance from ultrasonic sensor (using median of 10 iterations)
  int finalDistance = (distance1+distance2)/2;
  return finalDistance; 
}

int getDistanceDataRecycle(){
  int distance1 = sonar_recycle_1.convert_cm(sonar_recycle_1.ping_median(10)); // read distance from ultrasonic sensor (using median of 10 iterations)
  int distance2 = sonar_recycle_2.convert_cm(sonar_recycle_2.ping_median(10)); // read distance from ultrasonic sensor (using median of 10 iterations)
  int finalDistance = (distance1+distance2)/2; 
  return finalDistance; 
}

boolean weightReadyWaste() {
  if (LoadCell_WASTE.update()) {
    weightData_WASTE = LoadCell_WASTE.getData();
    
    //Get the last 4 weight measurements and compare them 
    weight_history_WASTE[3] = weight_history_WASTE[2];
    weight_history_WASTE[2] = weight_history_WASTE[1];
    weight_history_WASTE[1] = weight_history_WASTE[0];
    weight_history_WASTE[0] = weightData_WASTE;
    ave_WASTE = (weight_history_WASTE[0] + weight_history_WASTE[1] + weight_history_WASTE[2] + weight_history_WASTE[3])/4;

    if ((abs(ave_WASTE - weightData_WASTE) < 0.10) && weightData_WASTE >= 0.00) {
        return true;
    }
    else {
      return false;
    }
  }
}

boolean weightReadyPlastic() {
  if (LoadCell_PLASTIC.update()) {
    weightData_PLASTIC = LoadCell_PLASTIC.getData();
    
    //Get the last 4 weight measurements and compare them 
    weight_history_PLASTIC[3] = weight_history_PLASTIC[2];
    weight_history_PLASTIC[2] = weight_history_PLASTIC[1];
    weight_history_PLASTIC[1] = weight_history_PLASTIC[0];
    weight_history_PLASTIC[0] = weightData_WASTE;
    ave_PLASTIC = (weight_history_PLASTIC[0] + weight_history_PLASTIC[1] + weight_history_PLASTIC[2] + weight_history_PLASTIC[3])/4;

    if ((abs(ave_PLASTIC - weightData_PLASTIC) < 0.10) && weightData_PLASTIC >= 0.00) {
        return true;
    }
    else {
      return false;
    }
  }
}

boolean weightReadyCompost() {
  if (LoadCell_COMPOST.update()) {
    weightData_COMPOST = LoadCell_COMPOST.getData();
    
    //Get the last 4 weight measurements and compare them 
    weight_history_COMPOST[3] = weight_history_COMPOST[2];
    weight_history_COMPOST[2] = weight_history_COMPOST[1];
    weight_history_COMPOST[1] = weight_history_COMPOST[0];
    weight_history_COMPOST[0] = weightData_COMPOST;
    ave_COMPOST = (weight_history_COMPOST[0] + weight_history_COMPOST[1] + weight_history_COMPOST[2] + weight_history_COMPOST[3])/4;

    if ((abs(ave_COMPOST - weightData_COMPOST) < 0.10) && weightData_COMPOST >= 0.00) {
        return true;
    }
    else {
      return false;
    }
  }
}

boolean weightReadyRecycle() {
  if (LoadCell_RECYCLE.update()) {
    weightData_RECYCLE = LoadCell_RECYCLE.getData();
    
    //Get the last 4 weight measurements and compare them 
    weight_history_RECYCLE[3] = weight_history_RECYCLE[2];
    weight_history_RECYCLE[2] = weight_history_RECYCLE[1];
    weight_history_RECYCLE[1] = weight_history_RECYCLE[0];
    weight_history_RECYCLE[0] = weightData_RECYCLE;
    ave_RECYCLE = (weight_history_RECYCLE[0] + weight_history_RECYCLE[1] + weight_history_RECYCLE[2] + weight_history_RECYCLE[3])/4;

    if ((abs(ave_RECYCLE - weightData_RECYCLE) < 0.10) && weightData_RECYCLE >= 0.00) {
        return true;
    }
    else {
      return false;
    }
  }
}

String getLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return ("Failed to obtain time");
  }

  strftime(createdAt,50, "%c", &timeinfo);  
  Serial.println(createdAt);

  return (createdAt);
}

void sendToFirebase(String t){

  jsonAllData.set("WD",distanceData_WASTE);
  jsonAllData.set("WW", weightData_WASTE);
  jsonAllData.set("PD",distanceData_PLASTIC);
  jsonAllData.set("PW", weightData_PLASTIC);
  jsonAllData.set("CD",distanceData_COMPOST);
  jsonAllData.set("CW", weightData_COMPOST);
  jsonAllData.set("RD",distanceData_RECYCLE);
  jsonAllData.set("RW", weightData_RECYCLE);
  jsonAllData.set("Time", t);

  if (Firebase.pushJSON(firebaseData, BIN_NAME, jsonAllData)) {
    //SUCCESS
  } else {
    Serial.print("Error in sending a JSON data object to firebase, ");
    Serial.println(firebaseData.errorReason());
  }
 
  //*********************
  //Update Latest Data
  if (Firebase.setJSON(firebaseData, "SensorInfo/"+BIN_NAME+"/LatestData", jsonAllData)) {
    //SUCCESS
  } else {
    Serial.print("Error in sending the latest JSON data to firebase, ");
    Serial.println(firebaseData.errorReason());
  }
}

void setup() {
  Serial.begin(115200); // start serial port output, check for same speed at Serial Monitor
  
   //Connect to wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.println("Succesfully Connected to the Internet.");
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  
  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getLocalTime();
  connectToFirebaseDatabase(); //Once online, connect to the database

  //Configure the weight sensor
  initWeightSensors();

  //Add bin data to firebase
  //Update Latest Data
    if (Firebase.setString(firebaseData, "SensorInfo/"+BIN_NAME+"/Building", BUILDING_NAME)) {
     //SUCCESS
    } else {
     //Failed?, get the error reason from firebaseData
     Serial.print("Error in sending Building Name, ");
     Serial.println(firebaseData.errorReason());
   }

     if (Firebase.setString(firebaseData, "SensorInfo/"+BIN_NAME+"/Faculty", FACULTY_NAME)) {
     //SUCCESS
    } else {
     //Failed?, get the error reason from firebaseData
     Serial.print("Error in sending Faculty Name, ");
     Serial.println(firebaseData.errorReason());
   }

     if (Firebase.setInt(firebaseData, "SensorInfo/"+BIN_NAME+"/Floor", FLOOR_NUMBER)) {
     //SUCCESS
    } else {
     //Failed?, get the error reason from firebaseData
     Serial.print("Error in sending Building Name, ");
     Serial.println(firebaseData.errorReason());
   }

     if (Firebase.setString(firebaseData, "SensorInfo/"+BIN_NAME+"/ClosestRoom", CLOSEST_ROOM)) {
     //SUCCESS
    } else {
     //Failed?, get the error reason from firebaseData
     Serial.print("Error in sending Closest Room Number, ");
     Serial.println(firebaseData.errorReason());
   }
}

void loop() {
  unsigned long currentMillis = millis();
  LoadCell_WASTE.update();
  LoadCell_PLASTIC.update();
  LoadCell_COMPOST.update();
  LoadCell_RECYCLE.update();
  
  //Only run every 10 minute interval as decided
  if (currentMillis - previousMillis >= UPDATE_INTERVAL) { 
       previousMillis = currentMillis; //Update the time field
       
       distanceData_WASTE = abs(getDistanceDataWaste());
       distanceData_PLASTIC = abs(getDistanceDataPlastic());
       distanceData_COMPOST = abs(getDistanceDataCompost());
       distanceData_RECYCLE = abs(getDistanceDataRecycle());

       while (weightReadyWaste() == false){
          weightReadyWaste();
       }

       while (weightReadyPlastic() == false){
          weightReadyPlastic();
       }

       while (weightReadyCompost() == false){
          weightReadyCompost();
       }

       while (weightReadyRecycle() == false){
          weightReadyRecycle();
       }
      
       String createdAt = getLocalTime();
       sendToFirebase(createdAt); //Send data to firebase with the created time
  }
}
