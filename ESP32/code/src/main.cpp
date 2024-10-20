#include <WiFi.h>
#include "ThingSpeak.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

// Servo motor object
Servo ObjServo;

// Pin Definitions
static const int RAINDROP_PIN = 34;
static const int TEMPSENSOR_PIN = 33;
static const int LDRSENSOR_PIN = 35;
static const int WATERFLOW_PIN = 32;
static const int TRIG_PIN = 23;
static const int ECHO_PIN = 22;
static const int ServoGPIO = 13; // GPIO pin for Servo motor\\// Task handles

// Output Pin Definitions
static const int ON_Board_LED = 2; // On-Board LED for WiFi connection status
static const int RELAY_PIN_LIGHT = 4; // Relay Pin for controlling the light
static const int RELAY_PIN_FAN = 5; // Relay Pin for controlling the fan


// Task Handles
TaskHandle_t lcdTaskHandle;

//lcd pins
// SDA pin to D21
// SCL pin to D22


//define sound speed in cm/uS
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701




// set the LCD number of columns and rows
int lcdColumns = 32;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);


// WiFi Client Objects
WiFiClient client;

// DS18B20 Temperature Sensor Object
OneWire oneWire(TEMPSENSOR_PIN);
DallasTemperature DS18B20(&oneWire);

// ThingSpeak Configuration
unsigned long Channel_ID = 2694167;
const char *API_Key = "1PS6KHW9CD6UPX0B";

// Timing Variables
unsigned long last_time = 0;
const unsigned long Delay = 10000;

// Sensor Reading Variables
float temperature;
float raining;
float ldrAnalogValue;
float duration_us, distance_cm;
float volume;
volatile long pulse;
unsigned long lastTime;


// WiFi Credentials
const char *ssid = "Dark_Hetz";
const char *password = "123@Thisara1964";

// Web Server
WiFiServer server(80);


// HTTP Request Variables
String header;

// Timing Variables for HTTP Timeout
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// Forward declaration for task
void lcdTask(void *parameter);

// Forward Declarations
double ultrasonicSensor();
double rainSensor();
double tempSensor();
double ldrSensor();
double waterflowSensor();
void connectToWiFi();
void increase();


void setup()
{
    Serial.begin(115200);

    // Pin Mode Configurations
    pinMode(ON_Board_LED, OUTPUT);
    pinMode(RELAY_PIN_LIGHT, OUTPUT);
    pinMode(RELAY_PIN_FAN, OUTPUT);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(WATERFLOW_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(WATERFLOW_PIN), increase, RISING);


 



    // WiFi Configuration
    WiFi.mode(WIFI_STA);
    ThingSpeak.begin(client);

    // Connect to WiFi
    connectToWiFi();

    // initialize LCD
    lcd.init();
    // turn on LCD backlight
    lcd.backlight();

    // Create the lcdTask to run in parallel
    xTaskCreate(
        lcdTask,   /* Task function. */
        "lcdTask", /* name of task. */
        10000,     /* Stack size of task */
        NULL,      /* parameter of the task */
        1,         /* priority of the task */
        &lcdTaskHandle); /* Task handle to keep track of created task */


}

void loop()
{
  

    if ((millis() - last_time) > Delay)
    {
        
        volume = waterflowSensor();
        temperature = tempSensor();
        ldrAnalogValue = ldrSensor();
        distance_cm = ultrasonicSensor();
        raining = rainSensor();

      

        // water flow sensor
        Serial.println("Water Flow: " + String(volume) + " L/min");

        // ldr sensor
        Serial.println("LDR Value: " + String(ldrAnalogValue));

        // temperature sensor
        Serial.println("Temperature: " + String(temperature) + " C");

        // ultrasonic sensor
        float old_distance_cm = distance_cm;

        if (distance_cm > 0 && old_distance_cm != distance_cm)
        {
            Serial.println("Distance: " + String(distance_cm) + " cm");
        }

        // Update ThingSpeak Fields
        ThingSpeak.setField(1, raining);
        ThingSpeak.setField(2, temperature);
        ThingSpeak.setField(3, ldrAnalogValue);
        ThingSpeak.setField(4, distance_cm);
        ThingSpeak.setField(5, volume);

        int data = ThingSpeak.writeFields(Channel_ID, API_Key);

        if (data == 200)
        {
            Serial.println("Channel updated successfully!");
        }
        else
        {
            Serial.println("Problem updating channel. HTTP error code: " + String(data));
        }

        // outputController();

        
        last_time = millis();


    }
}

void connectToWiFi()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("Connecting...");
        while (WiFi.status() != WL_CONNECTED)
        {
            WiFi.begin(ssid, password);
            delay(5000);
        }
        Serial.println("\nConnected.");
        digitalWrite(ON_Board_LED, HIGH);
        
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }
}

double rainSensor()
{
    raining = analogRead(RAINDROP_PIN);
    
    raining = map(raining, 0, 1023, 255, 0);
    Serial.print("Raining: ");
    if(raining > 0)
    {
        Serial.println("Yes");
    }
    else
    {
        Serial.println("No");
    }
    return raining;
}

double tempSensor()
{
    DS18B20.requestTemperatures();
  
     temperature = DS18B20.getTempCByIndex(0);

     //fan on
        if(temperature < 30)
        {
            digitalWrite(RELAY_PIN_FAN, HIGH);
        }
        else
        {
            digitalWrite(RELAY_PIN_FAN, LOW);
        }

        return temperature;
}

double ldrSensor()
{
    ldrAnalogValue = analogRead(LDRSENSOR_PIN);
    Serial.print("LDR Value: ");
    Serial.print(ldrAnalogValue);

   if (ldrAnalogValue < 3000) // Adjust this threshold for darkness
{
    // It's dark, turn ON the relay (light on)
    Serial.println(" - Light is ON (Relay Activated)");
    digitalWrite(RELAY_PIN_LIGHT, HIGH);  // Relay ON
}
else
{
    // It's bright, turn OFF the relay (light off)
    Serial.println(" - Light is OFF (Relay Deactivated)");
    digitalWrite(RELAY_PIN_LIGHT, LOW);   // Relay OFF
}


   
    return ldrAnalogValue;
}

double ultrasonicSensor()
{
    // Ensure the TRIG pin is low for at least 2 microseconds
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    
    // Send a 10-microsecond pulse to the TRIG pin
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);


    // Measure the duration of the ECHO pin
    duration_us = pulseIn(ECHO_PIN, HIGH);

    // Calculate the distance in cm
    distance_cm = duration_us * SOUND_SPEED / 2;

    // Calculate the distance in inches
    float distance_inch = distance_cm * CM_TO_INCH;

    // Print the distance in cm and inches
    Serial.print("Distance: ");
    Serial.print(distance_cm);
    Serial.print(" cm, ");
    Serial.print(distance_inch);
    Serial.println(" inch");

    return distance_cm;

}

double waterflowSensor()
{
    volume = 2.663 * pulse / 1000 * 30;
    if (millis() - lastTime > 2000)
    {
        pulse = 0;
        lastTime = millis();
    }
    Serial.print("Water Flow: ");
    Serial.println(volume);
    return volume;
}

ICACHE_RAM_ATTR void increase()
{
    pulse++;
}




// Task function to handle the LCD display
void lcdTask(void *parameter) {
    while (true) {
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        lcd.print(temperature);
        lcd.print(" C");

        lcd.setCursor(0, 1);
        lcd.print("Rain: ");
        lcd.print(raining);
        lcd.print(" L/min");
        delay(15000); // Display for 15 seconds

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("LDR: ");
        lcd.print(ldrAnalogValue);

        lcd.setCursor(0, 1);
        lcd.print("Dist: ");
        lcd.print(distance_cm);
        lcd.print(" cm");
        delay(15000); // Display for 15 seconds

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Water: ");
        lcd.print(volume);
        lcd.print(" L/min");
        delay(2000); // Display for 2 seconds
    }
}