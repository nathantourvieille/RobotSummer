#include "env_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <map>

static std::map<std::string, std::string> envVars;  // Store the environment variables

void loadEnvVars(const std::string &filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Unable to open .env file" << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, '=')) {
            std::string value;
            if (std::getline(iss, value)) {
                setenv(key.c_str(), value.c_str(), 1);  // Set environment variable
                envVars[key] = value;  // Store the value in the map
            }
        }
    }
}

const char* getEnvVar(const std::string &key) {
    const char *val = std::getenv(key.c_str());
    if (val) {
        return val;
    }
    return envVars[key].c_str();  // Return the value from the map
}
