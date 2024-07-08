#include <WiFi.h>
#include <esp_wpa2.h> // Include WPA2 library
#include <ESPAsyncWebServer.h> // Include the ESPAsyncWebServer library
#include "env_loader.h"

//Function declarations
void connectToWiFi();

//Network credentials
const char* SSID = "ubcsecure";
const char* USERNAME = getEnvVar("USERNAME");
const char* PASSWORD = getEnvVar("PASSWORD");

// Set web server port number to 8000
const int WEB_SERVER_PORT = 8000;
AsyncWebServer server(WEB_SERVER_PORT);

void setup() {
  Serial.begin(115200);
  
  loadEnvVars(".env");

  // Configure Wi-Fi
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);

  // Configure WPA2-Enterprise
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)USERNAME, strlen(USERNAME));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)USERNAME, strlen(USERNAME));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)PASSWORD, strlen(PASSWORD));
  esp_wifi_sta_wpa2_ent_enable();

  connectToWiFi();

  // Define route to handle POST requests
  server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request) {
    String message;
    if (request->hasParam("message", true)) {
      message = request->getParam("message", true)->value();
      Serial.println("Received message: " + message);
    } else {
      message = "No message sent";
      Serial.println("No message sent");
    }
    request->send(200, "text/plain", "Message received");
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, attempting to reconnect...");
    connectToWiFi();
  }
  delay(10000); // Check Wi-Fi status every 10 seconds
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