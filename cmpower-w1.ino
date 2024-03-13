#include <Arduino.h>
extern "C" {
#include "user_interface.h"
}
#include <Arduino_JSON.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <LittleFS.h>
#include <Ticker.h>
#include <WiFiClient.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "FS.h"

/************************* WiFi Access Point *********************************/


#define RY1_IO 0
#define RY2_IO 12
#define RY_CLK 15
#define RY_ON LOW
#define RY_OFF HIGH
#define LED_ON LOW
#define LED_OFF HIGH
// #define LED BUILTIN_LED
#define LED_BLUE 16
#define LED_RED 14
#define LED_PWR 5
#define BUTTON 4

ESP8266WiFiMulti WiFiMulti;

char idChar[] = "000000";
char username[] = "ESP_000000";
char subTopic[] = "/lock/ESP_000000/cmd";

JSONVar conf;

// WiFiFlientSecure for SSL/TLS support
WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and
// login details.
Adafruit_MQTT_Client* mqtt;

Adafruit_MQTT_Subscribe* cmdCallbackSub;

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

bool loadConfig() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  conf = JSON.parse(configFile.readString());

  if (JSON.typeof(conf) == "undefined") {
    Serial.println("Failed to parse config file");
    return false;
  }

  configFile.close();
  return true;
}

bool saveConfig() {
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  String jsonStr = JSON.stringify(conf);
  configFile.println(jsonStr);
  configFile.close();
  return true;
}

void setRy1(bool state) {
  digitalWrite(RY1_IO, state ? RY_ON : RY_OFF);
  digitalWrite(RY_CLK, HIGH);
  delayMicroseconds(1000);
  digitalWrite(RY_CLK, LOW);
}

void setRy2(bool state) {
  digitalWrite(RY2_IO, state ? RY_ON : RY_OFF);
  digitalWrite(RY_CLK, HIGH);
  delayMicroseconds(1000);
  digitalWrite(RY_CLK, LOW);
  digitalWrite(LED_PWR, state ? LED_ON : LED_OFF);
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
  pinMode(RY2_IO, OUTPUT);
  pinMode(RY_CLK, OUTPUT);
  // pinMode(LED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_PWR, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
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
    digitalWrite(LED_BLUE, !digitalRead(LED_BLUE));
  }
  Serial.println();
  digitalWrite(LED_BLUE, LED_ON);

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

  if (strlen((const char*)conf["ca_cert"])) {
    String caStr = (const char*)conf["ca_cert"];
    caStr.replace(" CERTIFICATE", "#CERTIFICATE");
    caStr.replace(" ", "\n");
    caStr.replace("#", " ");
    X509List* ca = new X509List(caStr.c_str());
    client.setTrustAnchors(ca);
  } else if (strlen((const char*)conf["fingerprint"])) {
    client.setFingerprint((const char*)conf["fingerprint"]);
  }

  sprintf(subTopic, "/lock/%s/cmd", username);
  printf("username: %s\npassword: %s\nsub topic: %s\n", username,
         (const char*)conf["password"], subTopic);

  mqtt = new Adafruit_MQTT_Client(&client, (const char*)conf["mqtt_server"],
                                  (int)conf["mqtt_port"], username, username,
                                  (const char*)conf["password"]);

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

  Serial.printf("Connecting to mqtt->.. %s:%d\n",
                (const char*)conf["mqtt_server"], (int)conf["mqtt_port"]);

  uint8_t retries = 4;
  while ((ret = mqtt->connect()) != 0) {  // connect will return 0 for connected
    digitalWrite(LED_RED, !digitalRead(LED_RED));
    Serial.println(mqtt->connectErrorString(ret));
    char buf[256] = {0};
    int lastSSLError = client.getLastSSLError(buf, sizeof(buf));
    if (lastSSLError) {
      String caStr = (const char*)conf["ca_cert"];
      caStr.replace(" CERTIFICATE", "#CERTIFICATE");
      caStr.replace(" ", "\n");
      caStr.replace("#", " ");
      Serial.printf("ca_cert: %s\n", caStr.c_str());
      Serial.printf("fingerprint: %s\n", (const char*)conf["fingerprint"]);
      Serial.printf("SSL error: %d\n%s\n", lastSSLError, buf);
    }
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt->disconnect();
    delay(5000);
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

  digitalWrite(LED_RED, LED_OFF);
  if (!mqtt->ping()) {
    mqtt->disconnect();
    pingTicker.detach();
    return;
  }
  delay(100);
  digitalWrite(LED_RED, LED_ON);
}

void tickerTimeout() { tickerTimeoutFlag = true; }

void tickerStart() { pingTicker.attach(MQTT_PING_TIME, tickerTimeout); }

void webConfig() {
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server",
                                          conf["mqtt_server"], 64);
  WiFiManagerParameter custom_mqtt_port(
      "port", "mqtt port", String((int)conf["mqtt_port"]).c_str(), 6);
  WiFiManagerParameter custom_mqtts_ca_cert("ca_cert", "ca cert",
                                            conf["ca_cert"], 2048);
  WiFiManagerParameter custom_mqtts_fingerprint(
      "fingerprint", "sha1 fingerprint", conf["fingerprint"], 128);
  WiFiManagerParameter custom_password("password", "mqtt password",
                                       conf["password"], 64);
  WiFiManagerParameter custom_buptnet_user("buptnet_user", "buptnet user",
                                           conf["buptnet_user"], 16);
  WiFiManagerParameter custom_buptnet_pass("buptnet_pass", "buptnet pass", NULL,
                                           64);

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtts_ca_cert);
  wm.addParameter(&custom_mqtts_fingerprint);
  wm.addParameter(&custom_password);
  wm.addParameter(&custom_buptnet_user);
  wm.addParameter(&custom_buptnet_pass);
  wm.setConfigPortalBlocking(false);
  wm.startConfigPortal();
  while (wm.getConfigPortalActive()) {
    wm.process();
    delay(100);
    digitalWrite(LED_RED, !digitalRead(LED_RED));
    digitalWrite(LED_BLUE, !digitalRead(LED_BLUE));
  }
  Serial.println("config finish");
  conf["mqtt_server"] = custom_mqtt_server.getValue();
  conf["mqtt_port"] = String(custom_mqtt_port.getValue()).toInt();

  conf["ca_cert"] = custom_mqtts_ca_cert.getValue();
  Serial.println("ca_cert: ");
  Serial.println(custom_mqtts_ca_cert.getValue());

  conf["fingerprint"] = custom_mqtts_fingerprint.getValue();
  Serial.println("fingerprint: ");
  Serial.println(custom_mqtts_fingerprint.getValue());

  conf["password"] = custom_password.getValue();
  conf["buptnet_user"] = custom_buptnet_user.getValue();
  if (strlen(custom_buptnet_pass.getValue())) {
    conf["buptnet_pass"] = custom_buptnet_pass.getValue();
  }
  saveConfig();
  wm.reboot();
}

bool buptLogin(const String& url, const String& ref) {
  WiFiClient client;
  HTTPClient http;
  bool res = false;
  Serial.print("[BUPTNET Login] begin...\n");
  if (http.begin(client, url)) {  // HTTP

    Serial.print("[BUPTNET Login] POST...\n");
    // start connection and send HTTP header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String postBody = "user=" + String(conf["buptnet_user"]) +
                      "&pass=" + String(conf["buptnet_pass"]);
    // Serial.println(postBody);
    int httpCode = http.POST(postBody);

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[BUPTNET Login] code: %d\n", httpCode);
      if (httpCode != HTTP_CODE_OK) {
        Serial.println("[BUPTNET Login] fail");
        Serial.println(http.getLocation());
      } else if (http.getString().indexOf("登录成功") == -1) {
        Serial.println("[BUPTNET Login] incorrect password");
        Serial.println(http.getString());
      } else {
        res = true;
      }

    } else {
      Serial.printf("[BUPTNET Login] GET... failed, error: %s\n",
                    http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.printf("[BUPTNET Login] Unable to connect\n");
  }

  return res;
}

bool testPage(const String& url, String& retLoc) {
  WiFiClient client;
  HTTPClient httpTest;
  bool res = false;
  Serial.print("[Test Page] begin...\n");
  if (httpTest.begin(client, url)) {  // HTTP

    Serial.print("[Test Page] GET " + url + "...\n");
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

void ICACHE_RAM_ATTR handleKeyPress() {
  delayMicroseconds(50000);
  if (digitalRead(BUTTON) == HIGH) {
    return;
  }

  for (int i = 0; i < 100; i++) {
    delayMicroseconds(10000);
    if (digitalRead(BUTTON) == HIGH) {
      if (digitalRead(RY1_IO) == RY_ON) {
        setRy2(digitalRead(RY2_IO) == RY_OFF);
      } else {
        setRy1(true);
      }
      return;
    }
  }

  setRy1(false);
  setRy2(false);

  for (int i = 0; i < 400; i++) {
    delayMicroseconds(10000);
    if (digitalRead(BUTTON) == HIGH) {
      return;
    }
  }

  wm.reboot();
}

void setup() {
  ioInit();
  setRy1(false);
  setRy2(false);
  digitalWrite(LED_PWR, LED_ON);
  digitalWrite(LED_RED, LED_ON);
  digitalWrite(LED_BLUE, LED_ON);

  Serial.begin(115200);
  Serial.println();
  Serial.print(F("\nReset reason = "));
  auto rstInfo = ESP.getResetInfoPtr();
  Serial.println(rstInfo->reason);

  uint32_t id = system_get_chip_id();
  sprintf(idChar, "%06X", id);
  Serial.print("chip id: ");
  Serial.println(id, HEX);

  sprintf(username, "ESP_%s", idChar);

  if (!LittleFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  delay(1000);
  digitalWrite(LED_PWR, LED_OFF);
  delay(1000);
  digitalWrite(LED_BLUE, LED_OFF);
  digitalWrite(LED_RED, LED_OFF);
  if (!loadConfig() || !wm.getWiFiIsSaved() ||
      rstInfo->reason == REASON_EXT_SYS_RST || digitalRead(BUTTON) == LOW) {
    webConfig();
  }

  attachInterrupt(digitalPinToInterrupt(BUTTON), handleKeyPress, FALLING);

  wifiInit();

  String reUrl;
  if (!testPage("http://www.example.com/?cmd=redirect&arubalp=12345", reUrl)) {
    Serial.println(reUrl);
    testPage(reUrl);
    int pos = reUrl.indexOf('/', 7);
    String serverUrl = reUrl.substring(0, pos);
    Serial.println(serverUrl);
    if (!buptLogin(serverUrl + "/login", reUrl)) {
      digitalWrite(LED_BLUE, LED_ON);
      digitalWrite(LED_RED, LED_OFF);
      while (1) {
        digitalWrite(LED_BLUE, !digitalRead(LED_BLUE));
        digitalWrite(LED_RED, !digitalRead(LED_RED));
        delay(500);
      }
    }
  }

  delay(1);

  configTime(8 * 3600, 0, ntpServerName, "pool.ntp.org", "time.nist.gov");

  mqttInit();
}

void loop() {
  if (!WiFi.isConnected()) {
    delay(150);
    digitalWrite(LED_BLUE, !digitalRead(LED_BLUE));
  } else {
    digitalWrite(LED_BLUE, LED_ON);
    mqttConnect();
    if (mqtt->connected()) {
      digitalWrite(LED_RED, LED_ON);
      mqtt->processPackets(10);
      if (tickerTimeoutFlag) {
        tickerTimeoutFlag = false;
        mqttPing();
      }
    }
  }

  delay(50);
}
