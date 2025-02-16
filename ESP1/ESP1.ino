#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <math.h>  // For log calculations

const char* ssid = "-";           // WiFi SSID
const char* password = "-";      // WiFi Password
const char* mqtt_server = "-"; // PC IP

WiFiClient espClient;
PubSubClient client(espClient);

String GAS_SENSOR_ID = "ESP-1";

// MQ-2 Sensor Constants
const int mq2Pin = A0;
const float RL = 10.0;  // Load resistor value (kΩ)
const float R0 = 9.83;  // R0 (calibrated in clean air)
const float m = -0.58;  // Slope (MQ-2 datasheet)
const float b = 1.58;   // Intercept (MQ-2 datasheet)

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi!");

    client.setServer(mqtt_server, 1883);
    while (!client.connected()) {
        Serial.print("Connecting to MQTT...");
        if (client.connect(GAS_SENSOR_ID.c_str())) {
            Serial.println("Connected!");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" retrying in 5 seconds...");
            delay(5000);
        }
    }
}

void loop() {
    int gasValue = analogRead(mq2Pin);  // Read MQ-2 sensor (0-1023)
    
    float sensorVoltage = (gasValue / 1023.0) * 3.3;  // Convert ADC value to voltage
    float RS = ((3.3 - sensorVoltage) / sensorVoltage) * RL; // Calculate sensor resistance (RS)
    float ratio = RS / R0;// Compute RS/R0 ratio
    float ammoniaPPM = pow(10, ((log10(ratio) - b) / m)); // Calculate NH3 PPM using MQ-2 datasheet formula

    // Create MQTT message (send PPM, not raw ADC)
    String payload = GAS_SENSOR_ID + ":" + String(ammoniaPPM, 2);  // 2 decimal places
    client.publish("sensor/ammonia", payload.c_str());

    Serial.print("Sent to MQTT: "); 
    Serial.println(payload);
    
    delay(5000);  // Send data every 5 sec
}