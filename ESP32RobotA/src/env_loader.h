#ifndef ENV_LOADER_H
#define ENV_LOADER_H

#include <string>

void loadEnvVars(const std::string &filename);
const char* getEnvVar(const std::string &key);

#endif // ENV_LOADER_H
