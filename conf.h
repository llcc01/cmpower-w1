#ifndef CONF_H
#define CONF_H

#include <Arduino_JSON.h>

extern JSONVar conf;

bool loadConfig();
bool saveConfig();

#endif