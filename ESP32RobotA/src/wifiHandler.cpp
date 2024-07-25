#include "WiFiHandler.h"
#include <WiFi.h>
#include <esp_wpa2.h>

void setupWiFi(const char* ssid){
    // Configure Wi-Fi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    // Configure WPA2-Enterprise
    //esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)username, strlen(username));
    //esp_wifi_sta_wpa2_ent_set_username((uint8_t *)username, strlen(username));
    //esp_wifi_sta_wpa2_ent_set_password((uint8_t *)password, strlen(password));
    //esp_wifi_sta_wpa2_ent_enable();
    connectToWiFi(ssid);
}

void connectToWiFi(const char* ssid) {
    Serial.print("Connecting to ");
    Serial.println(ssid);
  
    //WiFi.begin(ssid);
    //WiFi.begin("BRIANNAGOPAUL", "12345678"); 
    WiFi.begin("nate1234", "nate1234");
    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < 15) {
        delay(1000);
        //Serial.print(".");
        Serial.println(getWiFiStatus());
        retryCount++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected.");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("");
        Serial.println("Failed to connect to WiFi. Retrying in 10 seconds...");
        delay(10000); // Wait for 20 seconds before retrying
        connectToWiFi(ssid); // Try to reconnect
    }
}


//void setupEndpoints(){}
 //Currently easier to setup endpoints in main.cpp

//For debugging
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