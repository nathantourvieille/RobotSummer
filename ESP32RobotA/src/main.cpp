#include <WiFi.h>
#include <esp_wpa2.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

// Function declarations
void connectToWiFi();
String getWiFiStatus();
void loadConfig();

// Network credentials
const char* SSID = "ubcsecure";
String USERNAME;
String PASSWORD;

// Set web server port number to 8000
const int WEB_SERVER_PORT = 8000;
AsyncWebServer server(WEB_SERVER_PORT);

const int pwmPin = 23; // GPIO 23
const int pwmChannel = 0;
const int pwmResolution = 8; // 8-bit resolution

const int IN1 = 22; 
const int IN2 = 21;

void setup() {
    Serial.begin(115200);
    delay(1000); // Ensure serial communication is initialized

    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("An error has occurred while mounting SPIFFS");
        return;
    }
    loadConfig();

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);

    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);

    // Initial PWM setup
    int initialFreq = 9000; // Frequency in Hz
    ledcSetup(pwmChannel, initialFreq, pwmResolution);
    ledcAttachPin(pwmPin, pwmChannel);

    // Configure Wi-Fi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    // Configure WPA2-Enterprise
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)USERNAME.c_str(), USERNAME.length());
    esp_wifi_sta_wpa2_ent_set_username((uint8_t *)USERNAME.c_str(), USERNAME.length());
    esp_wifi_sta_wpa2_ent_set_password((uint8_t *)PASSWORD.c_str(), PASSWORD.length());
    esp_wifi_sta_wpa2_ent_enable();
    connectToWiFi();

    // Define route to handle POST requests
    server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam("message", true)) {
            message = request->getParam("message", true)->value();
            Serial.println("Received message: " + message);

            // Convert the received message to an integer
            int newFreq = message.toInt();
            if (newFreq > 0) { // Ensure the frequency is a positive value
                // Reconfigure the PWM with the new frequency
                ledcSetup(pwmChannel, newFreq, pwmResolution);
                ledcAttachPin(pwmPin, pwmChannel); // Reattach the pin
                Serial.println("PWM frequency updated to: " + String(newFreq));
                request->send(200, "text/plain", "PWM frequency updated");
            } else {
                request->send(400, "text/plain", "Invalid frequency value. Must be a positive integer.");
            }
        } else {
            message = "No message sent";
            Serial.println("No message sent");
            request->send(400, "text/plain", "No message sent");
        }
    });
    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, attempting to reconnect...");
        connectToWiFi();
    }
  
    for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
        ledcWrite(pwmChannel, dutyCycle);
        delay(15);
    }
      delay(2000);
    // Gradually decrease brightness
    for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
        ledcWrite(pwmChannel, dutyCycle);
        delay(15);
    }

  

    // Print WiFi status every 10 seconds
    static unsigned long lastPrintTime = 0;
    if (millis() - lastPrintTime >= 10000) {
        Serial.println(getWiFiStatus());
        lastPrintTime = millis();
    }
}

void connectToWiFi() {
    Serial.print("Connecting to ");
    Serial.println(SSID);
  
    WiFi.begin(SSID);
    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < 20) {
        delay(1000);
        Serial.print(".");
        retryCount++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected.");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("");
        Serial.println("Failed to connect to WiFi. Retrying in 20 seconds...");
        delay(20000); // Wait for 20 seconds before retrying
        connectToWiFi(); // Try to reconnect
    }
}

String getWiFiStatus() {
    Serial.print("Current WiFi status: ");
    switch (WiFi.status()) {
        case WL_IDLE_STATUS:
            return "Idle";
            break;
        case WL_NO_SSID_AVAIL:
            return "No SSID available";
            break;
        case WL_SCAN_COMPLETED:
            return"Scan completed";
            break;
        case WL_CONNECTED:
            return "Connected";
            break;
        case WL_CONNECT_FAILED:
            return "Connect failed";
            break;
        case WL_CONNECTION_LOST:
            return "Connection lost";
            break;
        case WL_DISCONNECTED:
            return "Disconnected";
            break;
        default:
            return "Unknown status";
            break;
    }
}

void loadConfig() {
    File file = SPIFFS.open("/config.txt", "r");
    if (!file) {
        Serial.println("Failed to open config file");
        return;
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        int separatorIdx = line.indexOf('=');
        if (separatorIdx == -1) {
            continue;
        }

        String key = line.substring(0, separatorIdx);
        String value = line.substring(separatorIdx + 1);

        if (key == "USERNAME") {
            USERNAME = value;
        } else if (key == "PASSWORD") {
            PASSWORD = value;
        }
    }
    file.close();
}
