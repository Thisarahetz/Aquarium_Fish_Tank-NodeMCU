#include <WiFi.h>
#include "ThingSpeak.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>
Servo ObjServo; // Make object of Servo motor from Servo library

// Variable init
static const int RAINDROP_PIN = 34; 
static const int TEMPSENSOR_PIN = 33;
static const int LDRSENSOR_PIN = 35;
static const int WATERFLOW_PIN = 32;

static const int TRIG_PIN = 23;
static const int ECHO_PIN = 22;

static const int ServoGPIO = 13; // define the GPIO pin with which servo is connected
//output
// Assign output variables to GPIO pins
const int redLED = 2;
const int roofmoter = 13;
const int waterPump = 12;

const int ON_Board_LED = 4;  //--> Defining an On Board LED, used for indicators when the process of connecting to a wifi router

WiFiClient  client;
WiFiClient  client2;

//DS18B20'  declared
OneWire oneWire(TEMPSENSOR_PIN);
DallasTemperature DS18B20(&oneWire);

unsigned long Channel_ID = 1935507;
const char * API_Key = "ZFLCJT4LWWZJ5ODH";

unsigned long last_time = 0;
unsigned long Delay = 30000;

// Variables to store sensor readings
float temperature;
float raining;
float ldranalogValve;
float duration_us, distance_cm;
float volume;

volatile long pulse;
unsigned long lastTime;

// These variables used to store slider position 
String valueString = String(0);
int positon1 = 0;
int positon2 = 0;

// Enter your wifi network name and Wifi Password
const char* ssid = "Dark_Hetz";
const char* password = "123@Thisara1964";

// Set web server port number to 80
WiFiServer server(80);

// These variables store current output state of LED
String outputRedState = "off";
String outputroofmoter = "off";
String outputwaterPump = "off";

// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;


void setup() {
  Serial.begin(115200);
  // configure the trigger pin to output mode
  pinMode(TRIG_PIN, OUTPUT);
  // configure the echo pin to input mode
  pinMode(ECHO_PIN, INPUT);
  // config water flow  pin
  pinMode(WATERFLOW_PIN, INPUT);  
  attachInterrupt(digitalPinToInterrupt(WATERFLOW_PIN), increase, RISING);

  ObjServo.attach(ServoGPIO); // it will attach the ObjServo to ServoGPIO pin  

  // Initialize the output variables as outputs
  pinMode(redLED, OUTPUT);
  pinMode(roofmoter, OUTPUT);
  pinMode(waterPump,OUTPUT);
  // Set outputs to LOW
  digitalWrite(redLED, LOW);
  digitalWrite(roofmoter, LOW);
  digitalWrite(waterPump, LOW);
  
  WiFi.mode(WIFI_STA);   
  ThingSpeak.begin(client);
}


void loop() {
  if ((millis() - last_time) > Delay) {
    
    // Connect or reconnect to WiFi
    if(WiFi.status() != WL_CONNECTED){
      Serial.print("Connecting...");
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, password); 
        delay(5000);     
      } 
      Serial.println("\nConnected.");
      Serial.print(".");
      //----------------------------------------Make the On Board Flashing LED on the process of connecting to the wifi router.
      digitalWrite(ON_Board_LED, LOW);
      delay(250);
      digitalWrite(ON_Board_LED, HIGH);
      delay(250);
    }
    digitalWrite(ON_Board_LED, HIGH); //--> Turn off the On Board LED when it is connected to the wifi router.
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();
    
    
    // Obtaining a new sensor reading for all fields
    raining = rainSensor();
    //read temprage
    temperature = tempSensor();    
    //reading LDRAnalog    
    ldranalogValve = ldrSensor();
    //distance    
    distance_cm = ultrasonicSensor();
    //waterflow meter
    volume =  waterflowSensor();
       
    ThingSpeak.setField(1, raining);

    ThingSpeak.setField(2, temperature);

    ThingSpeak.setField(3, ldranalogValve);

    ThingSpeak.setField(4, distance_cm);

    ThingSpeak.setField(5, volume);
       
    int Data = ThingSpeak.writeFields(Channel_ID, API_Key);
outputcontroler();   
    if(Data == 200){
      Serial.println("Channel updated successfully!");
    }
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(Data));
    }
    last_time = millis();
  }
}

/*
Get valuve of rain
 */
double rainSensor() {
  raining = analogRead(RAINDROP_PIN);  // Read the analog value from sensor
  raining = map(raining, 0, 1023, 255, 0); // map the 10-bit data to 8-bit data
  //Rain
    Serial.print("raining : ");
    Serial.println(raining);
  return raining;             // Return analog rain value
}
/*
Get temperate valve
 */
double tempSensor(){
  DS18B20.requestTemperatures();       // send the command to get temperatures
  temperature = DS18B20.getTempCByIndex(0);  // read temperature in °C
  //Temperate
    Serial.print("Temperate : ");
    Serial.println(temperature);   
  return temperature;
}
/*
Get reads the input on analog pin LDR
 */
double ldrSensor(){
  // reads the input on analog pin A0 (value between 0 and 1023)
  ldranalogValve = analogRead(LDRSENSOR_PIN); 
// We'll have a few threshholds, qualitatively determined
  Serial.print(ldranalogValve);
  if (ldranalogValve < 100) {
    Serial.println(" - Very bright");
  } else if (ldranalogValve < 200) {
    Serial.println(" - Bright");
  } else if (ldranalogValve < 500) {
    Serial.println(" - Light");
  } else if (ldranalogValve < 800) {
    Serial.println(" - Dim");
  } else {
    Serial.println(" - Dark");
  }
  return ldranalogValve;
}
/*
Get reads the distance_cm
 */
double ultrasonicSensor(){
    
 // generate 10-microsecond pulse to TRIG pin
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // measure duration of pulse from ECHO pin
  duration_us = pulseIn(ECHO_PIN, HIGH);

  // calculate the distance
  distance_cm = 0.017 * duration_us;

  // print the value to Serial Monitor
  Serial.print("distance: ");
  Serial.print(distance_cm);
  Serial.println(" cm");
  return distance_cm;

}

/*
Get reads water folw speed 
*/
 double waterflowSensor(){
   volume = 2.663 * pulse / 1000 * 30;
  if (millis() - lastTime > 2000) {
    pulse = 0;
    lastTime = millis();
  }
  Serial.print(volume);
  Serial.println(" L/m");
  return volume;
 }
 ICACHE_RAM_ATTR void increase() {
  pulse++;
}
/*Control output
*/
void outputcontroler(){
  WiFiClient client2 = server.available(); // Listen for incoming clients

if (client2) { // If a new client connects,
Serial.println("New Client."); // print a message out in the serial port
String currentLine = ""; // make a String to hold incoming data from the client
currentTime = millis();
previousTime = currentTime;
while (client2.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
currentTime = millis(); 
if (client2.available()) { // if there's bytes to read from the client,
char c = client2.read(); // read a byte, then
Serial.write(c); // print it out the serial monitor
header += c;
if (c == '\n') { // if the byte is a newline character
// if the current line is blank, you got two newline characters in a row.
// that's the end of the client HTTP request, so send a response:
if (currentLine.length() == 0) {
// HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
// and a content-type so the client knows what's coming, then a blank line:
client2.println("HTTP/1.1 200 OK");
client2.println("Content-type:text/html");
client2.println("Connection: close");
client2.println();

// turns the GPIOs on and off
if (header.indexOf("GET /2/on") >= 0) {
Serial.println("RED LED is on");
outputRedState = "on";
digitalWrite(redLED, HIGH);
} else if (header.indexOf("GET /2/off") >= 0) {
Serial.println("RED LED is off");
outputRedState = "off";
digitalWrite(redLED, LOW);
} //GET /?value=180& HTTP/1.1
else if(header.indexOf("GET /?value=")>=0) 
{
positon1 = header.indexOf('=');
positon2 = header.indexOf('&');
valueString = header.substring(positon1+1, positon2);

//Rotate the servo
ObjServo.write(valueString.toInt());
Serial.println(valueString); 
} 



// Display the HTML web page
client2.println("<!DOCTYPE html><html>");
client2.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
client2.println("<link rel=\"icon\" href=\"data:,\">");
// CSS to style the on/off buttons 
client2.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
client2.println(".buttonRed { background-color: #ff0000; border: none; color: white; padding: 16px 40px; border-radius: 60%;");
client2.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
client2.println(".buttonGreen { background-color: #00ff00; border: none; color: white; padding: 16px 40px; border-radius: 60%;");
client2.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
client2.println(".buttonYellow { background-color: #feeb36; border: none; color: white; padding: 16px 40px; border-radius: 60%;");
client2.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
client2.println(".buttonOff { background-color: #77878A; border: none; color: white; padding: 16px 40px; border-radius: 70%;");
client2.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
client2.println("body { text-align: center; font-family: \"Trebuchet MS\", Arial; margin-left:auto; margin-right:auto;}");
client2.println(".slider { width: 300px; }</style>");
client2.println("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script></head>");
// Web Page Heading
client2.println("<body><h1>Home Aquarium Control System</h1>");
// Web Page
client2.println("<h2>Inferreadbulb on</h2>");
// Display current state, and ON/OFF buttons for GPIO 2 Red LED 
client2.println("<p>Red LED is " + outputRedState + "</p>");
// If the outputRedState is off, it displays the OFF button 
if (outputRedState=="on") {
client2.println("<p><a href=\"/2/off\"><button class=\"button buttonOff\">OFF</button></a></p>");
} else {
client2.println("<p><a href=\"/2/on\"><button class=\"button buttonRed\">ON</button></a></p>");
} 


client2.println("<h2>Water pump on</h2>");
// Display current state, and ON/OFF buttons for GPIO 2 Red LED 
client2.println("<p>Water pump is " + outputRedState + "</p>");
// If the outputRedState is off, it displays the OFF button 
if (outputRedState=="on") {
client2.println("<p><a href=\"/2/off\"><button class=\"button buttonYellow\">OFF</button></a></p>");
} else {
client2.println("<p><a href=\"/2/on\"><button class=\"button buttonYellow\">ON</button></a></p>");
} 


// Web Page
client2.println("<h2>Open or close roof</h2>");
client2.println("<p>Position: <span id=\"servoPos\"></span></p>"); 
client2.println("<input type=\"range\" min=\"0\" max=\"180\" class=\"slider\" id=\"servoSlider\" onchange=\"servo(this.value)\" value=\""+valueString+"\"/>");

client2.println("<script>var slider = document.getElementById(\"servoSlider\");");
client2.println("var servoP = document.getElementById(\"servoPos\"); servoP.innerHTML = slider.value;");
client2.println("slider.oninput = function() { slider.value = this.value; servoP.innerHTML = this.value; }");
client2.println("$.ajaxSetup({timeout:1000}); function servo(pos) { ");
client2.println("$.get(\"/?value=\" + pos + \"&\"); {Connection: close};}</script>");

client2.println("</body></html>");

// The HTTP response ends with another blank line
client2.println();
// Break out of the while loop
break;
} else { // if you got a newline, then clear currentLine
currentLine = "";
}
} else if (c != '\r') { // if you got anything else but a carriage return character,
currentLine += c; // add it to the end of the currentLine
}
}
}
// Clear the header variable
header = "";
// Close the connection
client2.stop();
Serial.println("Client disconnected.");
Serial.println("");
}
}