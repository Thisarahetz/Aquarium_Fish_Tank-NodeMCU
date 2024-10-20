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
static const int ServoGPIO = 13; 

// Output Pin Definitions
static const int ON_Board_LED = 2;
static const int RELAY_PIN_LIGHT = 4;
static const int RELAY_PIN_FAN = 5;

//lcd pins
// SDA pin to D21
// SCL pin to D22


//define sound speed in cm/uS
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701


// Task Handles
TaskHandle_t lcdTaskHandle;

// LCD pins and setup
int lcdColumns = 32;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// WiFi Credentials
const char *ssid = "Dark_Hetz";
const char *password = "123@Thisara1964";

// WiFi Client and ThingSpeak configuration
WiFiClient client;
unsigned long Channel_ID = 2694167;
const char *API_Key = "1PS6KHW9CD6UPX0B";

// Timing Variables
unsigned long last_time = 0;
const unsigned long Delay = 10000; // 10 seconds delay

// DS18B20 Temperature Sensor Object
OneWire oneWire(TEMPSENSOR_PIN);
DallasTemperature DS18B20(&oneWire);

// Sensor Reading Variables
volatile float temperature;
volatile float raining;
volatile float ldrAnalogValue;
volatile float duration_us, distance_cm;
volatile float volume;
volatile long pulse;
unsigned long lastTime = 0;

// WiFi reconnect timer
unsigned long previousReconnectAttempt = 0;
const unsigned long reconnectInterval = 5000; // 5 seconds

// HTTP Timeout
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
    lcd.backlight();

    // Create the lcdTask to run in parallel
    xTaskCreate(
        lcdTask, "lcdTask", 10000, NULL, 1, &lcdTaskHandle);
}

void loop()
{
    if (millis() - last_time > Delay)
    {
        // Sensor Readings
        volume = waterflowSensor();
        temperature = tempSensor();
        ldrAnalogValue = ldrSensor();
        distance_cm = ultrasonicSensor();
        raining = rainSensor();

        // Print sensor values
        Serial.println("Water Flow: " + String(volume) + " L/min");
        Serial.println("LDR Value: " + String(ldrAnalogValue));
        Serial.println("Temperature: " + String(temperature) + " C");
        Serial.println("Distance: " + String(distance_cm) + " cm");

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

        last_time = millis(); // Reset delay timer
    }

    // Reconnect WiFi if needed
    if (WiFi.status() != WL_CONNECTED && millis() - previousReconnectAttempt > reconnectInterval)
    {
        connectToWiFi();
        previousReconnectAttempt = millis();
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
    Serial.println(raining > 0 ? "Yes" : "No");

    return raining;
}

double tempSensor()
{
    DS18B20.requestTemperatures();
    temperature = DS18B20.getTempCByIndex(0);

    // Control fan based on temperature
    digitalWrite(RELAY_PIN_FAN, temperature < 30 ? HIGH : LOW);

    return temperature;
}

double ldrSensor()
{
    ldrAnalogValue = analogRead(LDRSENSOR_PIN);
    Serial.print("LDR Value: ");
    Serial.print(ldrAnalogValue);

    // Control light based on LDR value
    digitalWrite(RELAY_PIN_LIGHT, ldrAnalogValue < 3000 ? HIGH : LOW);

    return ldrAnalogValue;
}

double ultrasonicSensor()
{
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    duration_us = pulseIn(ECHO_PIN, HIGH);
    distance_cm = duration_us * SOUND_SPEED / 2;

    float distance_inch = distance_cm * CM_TO_INCH;
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
void lcdTask(void *parameter)
{
    while (true)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        lcd.print(temperature);
        lcd.print(" C");

        lcd.setCursor(0, 1);
        lcd.print("Rain: ");
        lcd.print(raining);

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
