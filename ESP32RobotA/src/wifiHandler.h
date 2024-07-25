#ifndef WIFIHANDLER_H
#define WIFIHANDLER_H

#include <ESPAsyncWebServer.h>


void setupWiFi(const char* ssid);
void connectToWiFi(const char* ssid);
//void setupEndpoints(); 
String getWiFiStatus();

#endif //WIFIHANDLER_H