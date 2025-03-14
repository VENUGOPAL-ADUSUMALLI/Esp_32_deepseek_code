#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h> // Include this library for secure connections
#include "secrets.h" // Include the external file for secrets

const char* model = "deepseek-r1-distill-llama-70b";
const int max_tokens = 4096;
const float temperature = 0.6;
const float top_p = 0.95;
String res = "";

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Connect to WiFi
    WiFi.begin(SSID, PASSWORD);
    Serial.print("Connecting to ");
    Serial.println(SSID);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    Serial.println("\nConnected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop() {
    Serial.print("Ask your Question: ");
    
    while (!Serial.available());

    res = Serial.readStringUntil('\n');
    res.trim(); // Trim spaces and newline characters
    
    if (res.length() < 1) return;

    Serial.println(res);

    WiFiClientSecure client;
    client.setInsecure();  // Disable SSL certificate validation

    HTTPClient https;
    if (https.begin(client, "https://api.groq.com/openai/v1/chat/completions")) {
        https.addHeader("Content-Type", "application/json");
        https.addHeader("Authorization", "Bearer " + String(GROQ_API_KEY));

        String payload = "{";
        payload += "\"model\": \"" + String(model) + "\",";
        payload += "\"messages\": [{\"role\": \"user\", \"content\": \"" + res + "\"}],";
        payload += "\"temperature\": " + String(temperature) + ",";
        payload += "\"max_tokens\": " + String(max_tokens) + ",";
        payload += "\"top_p\": " + String(top_p);
        payload += "}";

        Serial.print("Sending Payload: ");
        Serial.println(payload);

        int httpCode = https.POST(payload);
        Serial.printf("HTTP Response Code: %d\n", httpCode);

        if (httpCode > 0) {
            String response = https.getString();
            Serial.println("Response: ");
            Serial.println(response);

            DynamicJsonDocument doc(2048);
            DeserializationError error = deserializeJson(doc, response);

            if (!error) {
                if (doc.containsKey("choices") && doc["choices"][0].containsKey("message") &&
                    doc["choices"][0]["message"].containsKey("content")) {
                    String answer = doc["choices"][0]["message"]["content"].as<String>();
                    Serial.print("Answer: ");
                    Serial.println(answer);
                } else {
                    Serial.println("Error: Unexpected JSON format.");
                }
            } else {
                Serial.print("JSON Parsing Error: ");
                Serial.println(error.c_str());
            }
        } else {
            Serial.printf("[HTTPS] POST failed, error: %s\n", https.errorToString(httpCode).c_str());
        }

        https.end();
    } else {
        Serial.println("Connection to Groq API failed!");
    }

    res = ""; // Clear buffer
    delay(5000); // Prevent API spam
}
