#include "conf.h"

#include <Arduino_JSON.h>
#include <FS.h>
#include <LittleFS.h>

JSONVar conf;

bool loadConfig() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    // Serial.println("Failed to open config file");
    return false;
  }

  conf = JSON.parse(configFile.readString());

  if (JSON.typeof(conf) == "undefined") {
    // Serial.println("Failed to parse config file");
    return false;
  }

  configFile.close();
  return true;
}

bool saveConfig() {
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    // Serial.println("Failed to open config file for writing");
    return false;
  }

  String jsonStr = JSON.stringify(conf);
  configFile.println(jsonStr);
  configFile.close();
  return true;
}