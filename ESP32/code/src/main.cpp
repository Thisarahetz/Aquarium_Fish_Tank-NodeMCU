#include <WiFi.h>
#include "ThingSpeak.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>

// Servo motor object
Servo ObjServo;

// Pin Definitions
static const int RAINDROP_PIN = 34;
static const int TEMPSENSOR_PIN = 33;
static const int LDRSENSOR_PIN = 35;
static const int WATERFLOW_PIN = 32;
static const int TRIG_PIN = 23;
static const int ECHO_PIN = 22;
static const int ServoGPIO = 13; // GPIO pin for Servo motor

// Output Pin Definitions
const int redLED = 2;
const int roofMotor = 13;
const int waterPump = 12;
const int ON_Board_LED = 4; // On-Board LED for WiFi connection status

// WiFi Client Objects
WiFiClient client;
WiFiClient client2;

// DS18B20 Temperature Sensor Object
OneWire oneWire(TEMPSENSOR_PIN);
DallasTemperature DS18B20(&oneWire);

// ThingSpeak Configuration
unsigned long Channel_ID = 1935507;
const char *API_Key = "ZFLCJT4LWWZJ5ODH";

// Timing Variables
unsigned long last_time = 0;
const unsigned long Delay = 30000;

// Sensor Reading Variables
float temperature;
float raining;
float ldrAnalogValue;
float duration_us, distance_cm;
float volume;
volatile long pulse;
unsigned long lastTime;

// Slider Position Variables
String valueString = "0";
int position1 = 0;
int position2 = 0;

// WiFi Credentials
const char *ssid = "Dark_Hetz";
const char *password = "123@Thisara1964";

// Web Server
WiFiServer server(80);

// Output State Variables
String outputRedState = "off";
String outputRoofMotor = "off";
String outputWaterPump = "off";

// HTTP Request Variables
String header;

// Timing Variables for HTTP Timeout
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// Forward Declarations
double ultrasonicSensor();
double rainSensor();
double tempSensor();
double ldrSensor();
double waterflowSensor();
void connectToWiFi();
void outputController();
void sendHTTPResponse(WiFiClient &client2);
void handleClientRequest();
void increase();

void setup()
{
    Serial.begin(115200);

    // Pin Mode Configurations
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(WATERFLOW_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(WATERFLOW_PIN), increase, RISING);

    ObjServo.attach(ServoGPIO); // Attach servo to GPIO pin

    // Initialize Output Pins
    pinMode(redLED, OUTPUT);
    pinMode(roofMotor, OUTPUT);
    pinMode(waterPump, OUTPUT);

    // Set Outputs to LOW
    digitalWrite(redLED, LOW);
    digitalWrite(roofMotor, LOW);
    digitalWrite(waterPump, LOW);

    // WiFi Configuration
    WiFi.mode(WIFI_STA);
    ThingSpeak.begin(client);
}

void loop()
{

    if ((millis() - last_time) > Delay)
    {
        // connectToWiFi();
        // server.begin();

        // Update Sensor Readings
        // raining = rainSensor();
        // temperature = tempSensor();
        // ldrAnalogValue = ldrSensor();
        distance_cm = ultrasonicSensor();

        //old distance_cm
        float old_distance_cm = distance_cm;

        if (distance_cm > 0 && old_distance_cm != distance_cm)

        
        {
            Serial.println("Distance: " + String(distance_cm) + " cm");
        }
        // volume = waterflowSensor();

        //     // Update ThingSpeak Fields
        //     ThingSpeak.setField(1, raining);
        //     ThingSpeak.setField(2, temperature);
        //     ThingSpeak.setField(3, ldrAnalogValue);
        //     ThingSpeak.setField(4, distance_cm);
        //     ThingSpeak.setField(5, volume);

        //     int data = ThingSpeak.writeFields(Channel_ID, API_Key);
        //     outputController();

        //     if (data == 200) {
        //         Serial.println("Channel updated successfully!");
        //     } else {
        //         Serial.println("Problem updating channel. HTTP error code: " + String(data));
        //     }
        //     last_time = millis();
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
    Serial.println(raining);
    return raining;
}

double tempSensor()
{
    DS18B20.requestTemperatures();
    temperature = DS18B20.getTempCByIndex(0);
    Serial.print("Temperature: ");
    Serial.println(temperature);
    return temperature;
}

double ldrSensor()
{
    ldrAnalogValue = analogRead(LDRSENSOR_PIN);
    Serial.print("LDR Value: ");
    Serial.print(ldrAnalogValue);

    if (ldrAnalogValue < 100)
    {
        Serial.println(" - Very bright");
    }
    else if (ldrAnalogValue < 200)
    {
        Serial.println(" - Bright");
    }
    else if (ldrAnalogValue < 500)
    {
        Serial.println(" - Light");
    }
    else if (ldrAnalogValue < 800)
    {
        Serial.println(" - Dim");
    }
    else
    {
        Serial.println(" - Dark");
    }

    return ldrAnalogValue;
}

double ultrasonicSensor()
{
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2); // Ensure the trigger pin is low for at least 2 microseconds
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10); // Send a 10-microsecond pulse to the TRIG pin
    digitalWrite(TRIG_PIN, LOW);

    duration_us = pulseIn(ECHO_PIN, HIGH);

    if (duration_us == 0)
    {
        // Serial.println("Timeout, no echo received.");
        return -1; // or another appropriate value to indicate no measurement
    }
    distance_cm = 0.017 * duration_us;

    Serial.print("Distance: ");
    Serial.print(distance_cm);
    Serial.println(" cm");

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

void outputController()
{
    WiFiClient client2 = server.available();

    if (client2)
    {
        Serial.println("New Client.");
        String currentLine = "";
        currentTime = millis();
        previousTime = currentTime;

        while (client2.connected() && currentTime - previousTime <= timeoutTime)
        {
            currentTime = millis();
            if (client2.available())
            {
                char c = client2.read();
                Serial.write(c);
                header += c;

                if (c == '\n')
                {
                    if (currentLine.length() == 0)
                    {
                        sendHTTPResponse(client2);
                        break;
                    }
                    else
                    {
                        currentLine = "";
                    }
                }
                else if (c != '\r')
                {
                    currentLine += c;
                }
            }
        }
        header = "";
        client2.stop();
        Serial.println("Client disconnected.");
    }
}

void sendHTTPResponse(WiFiClient &client2)
{
    client2.println("HTTP/1.1 200 OK");
    client2.println("Content-type:text/html");
    client2.println("Connection: close");
    client2.println();

    handleClientRequest();

    // HTML Content
    client2.println("<!DOCTYPE html><html>");
    client2.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    client2.println("<link rel=\"icon\" href=\"data:,\">");
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
    client2.println("<body><h1>Home Aquarium Control System</h1>");
    client2.println("<h2>Inferred Bulb Control</h2>");
    client2.println("<p>Red LED is " + outputRedState + "</p>");

    if (outputRedState == "on")
    {
        client2.println("<p><a href=\"/red/off\"><button class=\"buttonRed\">OFF</button></a></p>");
    }
    else
    {
        client2.println("<p><a href=\"/red/on\"><button class=\"buttonOff\">ON</button></a></p>");
    }

    // Roof Motor Controls
    client2.println("<h2>Roof Motor Control</h2>");
    client2.println("<p>Motor is " + outputRoofMotor + "</p>");

    if (outputRoofMotor == "on")
    {
        client2.println("<p><a href=\"/roofmotor/off\"><button class=\"buttonYellow\">Roof is ON</button></a></p>");
    }
    else
    {
        client2.println("<p><a href=\"/roofmotor/on\"><button class=\"buttonOff\">Roof is OFF</button></a></p>");
    }

    // Water Pump Controls
    client2.println("<h2>Water Pump Control</h2>");
    client2.println("<p>Water pump is " + outputWaterPump + "</p>");

    if (outputWaterPump == "on")
    {
        client2.println("<p><a href=\"/waterpump/off\"><button class=\"buttonGreen\">Pump is ON</button></a></p>");
    }
    else
    {
        client2.println("<p><a href=\"/waterpump/on\"><button class=\"buttonOff\">Pump is OFF</button></a></p>");
    }

    client2.println("<h4>Slider Controlled Motor</h4>");
    client2.println("<div><input type=\"range\" class=\"slider\" id=\"myRange\" value=\"90\" min=\"0\" max=\"180\"></div>");
    client2.println("<p>Motor position: <span id=\"demo\"></span></p>");
    client2.println("<script>var slider = document.getElementById(\"myRange\"); var output = document.getElementById(\"demo\");");
    client2.println("output.innerHTML = slider.value; slider.oninput = function() { output.innerHTML = this.value;");
    client2.println("$.get(\"ajax_inputs\" + this.value); }</script></body></html>");

    client2.println();
}

void handleClientRequest()
{
    if (header.indexOf("GET /red/on") >= 0)
    {
        outputRedState = "on";
        digitalWrite(redLED, HIGH);
    }
    else if (header.indexOf("GET /red/off") >= 0)
    {
        outputRedState = "off";
        digitalWrite(redLED, LOW);
    }
    else if (header.indexOf("GET /roofmotor/on") >= 0)
    {
        outputRoofMotor = "on";
        ObjServo.write(45);
    }
    else if (header.indexOf("GET /roofmotor/off") >= 0)
    {
        outputRoofMotor = "off";
        ObjServo.write(90);
    }
    else if (header.indexOf("GET /waterpump/on") >= 0)
    {
        outputWaterPump = "on";
        digitalWrite(waterPump, HIGH);
    }
    else if (header.indexOf("GET /waterpump/off") >= 0)
    {
        outputWaterPump = "off";
        digitalWrite(waterPump, LOW);
    }
}
