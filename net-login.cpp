#include "net-login.h"

#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#include "conf.h"

bool buptLogin(const String& url, const String& ref) {
  WiFiClient client;
  HTTPClient http;
  bool res = false;
  // Serial.print("[BUPTNET Login] begin...\n");
  if (http.begin(client, url)) {  // HTTP

    // Serial.print("[BUPTNET Login] POST...\n");
    // start connection and send HTTP header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String postBody = "user=" + String(conf["buptnet_user"]) +
                      "&pass=" + String(conf["buptnet_pass"]);
    // // Serial.println(postBody);
    int httpCode = http.POST(postBody);

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      // Serial.printf("[BUPTNET Login] code: %d\n", httpCode);
      if (httpCode != HTTP_CODE_OK) {
        // Serial.println("[BUPTNET Login] fail");
        // Serial.println(http.getLocation());
      } else if (http.getString().indexOf("登录成功") == -1) {
        // Serial.println("[BUPTNET Login] incorrect password");
        // Serial.println(http.getString());
      } else {
        res = true;
      }

    } else {
      // Serial.printf("[BUPTNET Login] GET... failed, error: %s\n",
      //               http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    // Serial.printf("[BUPTNET Login] Unable to connect\n");
  }

  return res;
}

bool testPage(const String& url, String& retLoc) {
  WiFiClient client;
  HTTPClient httpTest;
  bool res = false;
  // Serial.print("[Test Page] begin...\n");
  if (httpTest.begin(client, url)) {  // HTTP

    // Serial.print("[Test Page] GET " + url + "...\n");
    // start connection and send HTTP header
    int httpCode = httpTest.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      // Serial.printf("[Test Page] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        res = true;
      } else {
        retLoc = httpTest.getLocation();
      }
    } else {
      // Serial.printf("[Test Page] GET... failed, error: %s\n",
      //               httpTest.errorToString(httpCode).c_str());
    }

    httpTest.end();
  } else {
    // Serial.printf("[Test Page] Unable to connect\n");
  }

  return res;
}

bool testPage(String url) {
  String nu;
  return testPage(url, nu);
}