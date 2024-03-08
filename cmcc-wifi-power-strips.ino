#include <Arduino.h>
extern "C" {
#include "user_interface.h"
}
#include <Arduino_JSON.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <Ticker.h>
#include <WiFiClient.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* WiFi Access Point *********************************/

// #define MQTT_SERVER "lllccc.top"
// #define MQTT_PORT 8883

#define RY1_IO 0
#define RY2_IO 12
#define RY_CLK 15
#define RY_ON HIGH
#define RY_OFF LOW
#define LED_ON LOW
#define LED_OFF HIGH
#define LED BUILTIN_LED

ESP8266WiFiMulti WiFiMulti;

static const char* fingerprint PROGMEM =
    "07a5d99a25df6ad4051950e2a5e08103aa3668b2";
// const char caCert[] PROGMEM = R"EOF(
// -----BEGIN CERTIFICATE-----
// MIIFFjCCAv6gAwIBAgIRAJErCErPDBinU/bWLiWnX1owDQYJKoZIhvcNAQELBQAw
// TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
// cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjAwOTA0MDAwMDAw
// WhcNMjUwOTE1MTYwMDAwWjAyMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg
// RW5jcnlwdDELMAkGA1UEAxMCUjMwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK
// AoIBAQC7AhUozPaglNMPEuyNVZLD+ILxmaZ6QoinXSaqtSu5xUyxr45r+XXIo9cP
// R5QUVTVXjJ6oojkZ9YI8QqlObvU7wy7bjcCwXPNZOOftz2nwWgsbvsCUJCWH+jdx
// sxPnHKzhm+/b5DtFUkWWqcFTzjTIUu61ru2P3mBw4qVUq7ZtDpelQDRrK9O8Zutm
// NHz6a4uPVymZ+DAXXbpyb/uBxa3Shlg9F8fnCbvxK/eG3MHacV3URuPMrSXBiLxg
// Z3Vms/EY96Jc5lP/Ooi2R6X/ExjqmAl3P51T+c8B5fWmcBcUr2Ok/5mzk53cU6cG
// /kiFHaFpriV1uxPMUgP17VGhi9sVAgMBAAGjggEIMIIBBDAOBgNVHQ8BAf8EBAMC
// AYYwHQYDVR0lBBYwFAYIKwYBBQUHAwIGCCsGAQUFBwMBMBIGA1UdEwEB/wQIMAYB
// Af8CAQAwHQYDVR0OBBYEFBQusxe3WFbLrlAJQOYfr52LFMLGMB8GA1UdIwQYMBaA
// FHm0WeZ7tuXkAXOACIjIGlj26ZtuMDIGCCsGAQUFBwEBBCYwJDAiBggrBgEFBQcw
// AoYWaHR0cDovL3gxLmkubGVuY3Iub3JnLzAnBgNVHR8EIDAeMBygGqAYhhZodHRw
// Oi8veDEuYy5sZW5jci5vcmcvMCIGA1UdIAQbMBkwCAYGZ4EMAQIBMA0GCysGAQQB
// gt8TAQEBMA0GCSqGSIb3DQEBCwUAA4ICAQCFyk5HPqP3hUSFvNVneLKYY611TR6W
// PTNlclQtgaDqw+34IL9fzLdwALduO/ZelN7kIJ+m74uyA+eitRY8kc607TkC53wl
// ikfmZW4/RvTZ8M6UK+5UzhK8jCdLuMGYL6KvzXGRSgi3yLgjewQtCPkIVz6D2QQz
// CkcheAmCJ8MqyJu5zlzyZMjAvnnAT45tRAxekrsu94sQ4egdRCnbWSDtY7kh+BIm
// lJNXoB1lBMEKIq4QDUOXoRgffuDghje1WrG9ML+Hbisq/yFOGwXD9RiX8F6sw6W4
// avAuvDszue5L3sz85K+EC4Y/wFVDNvZo4TYXao6Z0f+lQKc0t8DQYzk1OXVu8rp2
// yJMC6alLbBfODALZvYH7n7do1AZls4I9d1P4jnkDrQoxB3UqQ9hVl3LEKQ73xF1O
// yK5GhDDX8oVfGKF5u+decIsH4YaTw7mP3GFxJSqv3+0lUFJoi5Lc5da149p90Ids
// hCExroL1+7mryIkXPeFM5TgO9r0rvZaBFOvV2z0gp35Z0+L4WPlbuEjN/lxPFin+
// HlUjr8gRsI3qfJOQFy/9rKIJR0Y/8Omwt/8oTWgy1mdeHmmjk7j1nYsvC9JSQ6Zv
// MldlTTKB3zhThV1+XWYp6rjd5JW1zbVWEkLNxE7GJThEUG3szgBVGP7pSWTUTsqX
// nLRbwHOoq7hHwg==
// -----END CERTIFICATE-----
// )EOF";

char idChar[] = "000000";
char username[] = "ESP_000000";
// char password[] = "000000";
char subTopic[] = "/lock/ESP_000000/cmd";
// const String BUPT_LOGIN_PD = "user=2021211051&pass=123456";

WiFiManagerParameter custom_mqtt_server("server", "mqtt server", "", 64);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", "8883", 6);
WiFiManagerParameter custom_password("password", "password", "", 32);
WiFiManagerParameter custom_buptnet_user("buptnet_user", "buptnet_user", "",
                                         10);
WiFiManagerParameter custom_buptnet_pass("buptnet_pass", "buptnet_pass", "",
                                         64);

// WiFiFlientSecure for SSL/TLS support
WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and
// login details.
Adafruit_MQTT_Client *mqtt;

Adafruit_MQTT_Subscribe *cmdCallbackSub;

/**NTP**/
const char* ntpServerName = "cn.pool.ntp.org";

/**** Ticker ****/
Ticker pingTicker;
#define MQTT_PING_TIME 120  // second
static bool tickerTimeoutFlag = false;

#define CMD_SWITCH_UNKNOWN -1
#define CMD_SWITCH_ON 1
#define CMD_SWITCH_OFF 0

/**** config ****/
WiFiManager wm;


void setRy1(int state) {
  digitalWrite(RY1_IO, state ? RY_ON : RY_OFF);
  digitalWrite(RY_CLK, HIGH);
  delay(1);
  digitalWrite(RY_CLK, LOW);
}

void setRy2(int state) {
  digitalWrite(RY2_IO, state ? RY_ON : RY_OFF);
  digitalWrite(RY_CLK, HIGH);
  delay(1);
  digitalWrite(RY_CLK, LOW);
}

int getCmdSwitch(JSONVar* cmd, const String& key) {
  if (!cmd->hasOwnProperty(key)) {
    return CMD_SWITCH_UNKNOWN;
  }
  bool f = (bool)(*cmd)[key];
  Serial.println(key + ": " + (f ? "on" : "off"));
  return f ? CMD_SWITCH_ON : CMD_SWITCH_OFF;
}

void cmdCallback(char* data, uint16_t len) {
  Serial.println(data);
  JSONVar cmd = JSON.parse(data);
  if (JSON.typeof(cmd) == "undefined") {
    Serial.println("Parsing cmd failed!");
    return;
  }

  int ry1 = getCmdSwitch(&cmd, "ry1");
  if (ry1 >= 0) {
    setRy1(ry1);
  }

  int ry2 = getCmdSwitch(&cmd, "ry2");
  if (ry2 >= 0) {
    setRy2(ry2);
  }
}

void ioInit() {
  pinMode(RY1_IO, OUTPUT);
  // pinMode(RY2_IO, OUTPUT);
  pinMode(LED, OUTPUT);
}

void wifiInit() {
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to ");
  Serial.println(wm.getWiFiSSID());
  WiFi.begin(wm.getWiFiSSID(), wm.getWiFiPass());
  // Serial.println(WLAN_SSID);
  // WiFi.begin(WLAN_SSID, WLAN_PASS);
  // WiFiMulti.addAP(WLAN_SSID, WLAN_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    // while (WiFiMulti.run() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
    digitalWrite(LED, !digitalRead(LED));
  }
  Serial.println();
  digitalWrite(LED, LED_ON);

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqttInit() {
  // check the fingerprint SSL cert
  // X509List *certList = new X509List(clientCert);
  // PrivateKey *priKey = new PrivateKey(clientPrivateKey);
  // client.setClientRSACert(certList, priKey);
  // client.allowSelfSignedCerts();

  // X509List* ca = new X509List(caCert);
  // client.setTrustAnchors(ca);
  client.setFingerprint(fingerprint);

  sprintf(subTopic, "sn/lock/%s/cmd", username);
  printf("username: %s\npassword: %s\nsub topic: %s\n", username, custom_password.getValue(),
         subTopic);

  mqtt = new Adafruit_MQTT_Client(&client, custom_mqtt_server.getValue(),
                              String(custom_mqtt_port.getValue()).toInt(),
                              username, username, custom_password.getValue());

  cmdCallbackSub = new Adafruit_MQTT_Subscribe(mqtt, subTopic);

  cmdCallbackSub->setCallback(cmdCallback);

  mqtt->subscribe(cmdCallbackSub);

  mqtt->setKeepAliveInterval(MQTT_PING_TIME);
}

void mqttConnect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt->connected()) {
    return;
  }

  Serial.print("Connecting to mqtt->.. ");

  uint8_t retries = 4;
  while ((ret = mqtt->connect()) != 0) {  // connect will return 0 for connected
    digitalWrite(LED, !digitalRead(LED));
    Serial.println(mqtt->connectErrorString(ret));
    int lastSSLError = client.getLastSSLError();
    if (lastSSLError) {
      Serial.printf("SSL error: %d\n", lastSSLError);
    }
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt->disconnect();
    delay(500);
    retries--;
    if (retries == 0) {
      // // basically die and wait for WDT to reset me
      // while (1)
      //   ;
      return;
    }
  }

  Serial.println("MQTT Connected!");

  tickerStart();
}

void mqttPing() {
  if (!mqtt->connected()) {
    return;
  }

  digitalWrite(LED, LED_OFF);
  if (!mqtt->ping()) {
    mqtt->disconnect();
    pingTicker.detach();
    return;
  }
  delay(100);
  digitalWrite(LED, LED_ON);
}

void tickerTimeout() { tickerTimeoutFlag = true; }

void tickerStart() { pingTicker.attach(MQTT_PING_TIME, tickerTimeout); }

void webConfig() {
  wm.setConfigPortalBlocking(false);
  wm.startConfigPortal();
  while (wm.getConfigPortalActive()) {
    wm.process();
    delay(100);
    digitalWrite(LED, !digitalRead(LED));
  }
  Serial.println("config finish");
  wm.reboot();
}

bool buptLogin(const String& url, const String& ref) {
  WiFiClient client;
  HTTPClient http;
  bool res;
  Serial.print("[BUPT Login] begin...\n");
  if (http.begin(client, url)) {  // HTTP

    Serial.print("[BUPT Login] POST...\n");
    // start connection and send HTTP header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST("user=" + String(custom_buptnet_user.getValue()) +
                             "&pass=" + String(custom_buptnet_pass.getValue()));

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[BUPT Login] POST... code: %d\n", httpCode);
      if (httpCode != HTTP_CODE_OK) {
        Serial.println("[BUPT Login] fail");
        Serial.println(http.getLocation());
      } else if (http.getString().indexOf("登录成功") == -1) {
        Serial.println("[BUPT Login] incorrect password");
      } else {
        res = true;
      }

    } else {
      Serial.printf("[BUPT Login] GET... failed, error: %s\n",
                    http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.printf("[BUPT Login] Unable to connect\n");
  }

  return res;
}

bool testPage(const String& url, String& retLoc) {
  WiFiClient client;
  HTTPClient httpTest;
  bool res = false;
  Serial.print("[Test Page] begin...\n");
  if (httpTest.begin(client, url)) {  // HTTP

    Serial.print("[Test Page] GET...\n");
    // start connection and send HTTP header
    int httpCode = httpTest.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[Test Page] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        res = true;
      } else {
        retLoc = httpTest.getLocation();
      }
    } else {
      Serial.printf("[Test Page] GET... failed, error: %s\n",
                    httpTest.errorToString(httpCode).c_str());
    }

    httpTest.end();
  } else {
    Serial.printf("[Test Page] Unable to connect\n");
  }

  return res;
}

bool testPage(String url) {
  String nu;
  return testPage(url, nu);
}

void setup() {
  ioInit();
  setRy1(RY_OFF);
  setRy2(RY_OFF);
  digitalWrite(LED, LED_ON);

  Serial.begin(115200);
  Serial.println();
  uint32_t id = system_get_chip_id();
  sprintf(idChar, "%06X", id);
  Serial.print("chip id: ");
  Serial.println(id, HEX);

  sprintf(username, "ESP_%s", idChar);

  delay(3000);
  if (!wm.getWiFiIsSaved()) {
    webConfig();
  }

  wifiInit();

  String reUrl;
  if (!testPage("http://www.example.com/?cmd=redirect&arubalp=12345", reUrl)) {
    Serial.println(reUrl);
    int pos = reUrl.indexOf('/', 7);
    String serverUrl = reUrl.substring(0, pos);
    Serial.println(serverUrl);
    if (buptLogin(serverUrl + "/login", reUrl))
      while (1) {
        delay(500);
      }
  }

  configTime(8 * 3600, 0, ntpServerName, "pool.ntp.org", "time.nist.gov");

  mqttInit();
}

void loop() {
  if (!WiFi.isConnected()) {
    delay(150);
    digitalWrite(LED, !digitalRead(LED));
  } else {
    mqttConnect();
    if (mqtt->connected()) {
      digitalWrite(LED, LED_ON);
      mqtt->processPackets(10);
      if (tickerTimeoutFlag) {
        tickerTimeoutFlag = false;
        mqttPing();
      }
    }
  }

  delay(50);
}
