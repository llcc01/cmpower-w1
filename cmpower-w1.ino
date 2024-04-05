#include <Arduino.h>
extern "C" {
#include "user_interface.h"
}
#include <ArduinoMqttClient.h>
#include <Arduino_JSON.h>
#include <Crypto.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <LittleFS.h>
#include <Ticker.h>
#include <WiFiClient.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>

#include "conf.h"
#include "net-login.h"
#include "sy7t609.h"
// #include "test.h"

/************************* WiFi Access Point *********************************/

#define TIMESTAMP_YEAR2024 1704067200

#define USE_TUYA 1

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

String clientId;
String username;
String password;

String subTopic;
String pubTopic;

// for campatible with tuya
#if USE_TUYA
String setTopic;
String execTopic;

String TYDeviceID;
String TYDeviceSecret;
#endif

// WiFiFlientSecure for SSL/TLS support
WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and
// login details.
MqttClient mqtt(client);

/**NTP**/
const char* ntpServerName = "cn.pool.ntp.org";

/**** Ticker ****/
#define MQTT_PING_TIME 120  // second

Ticker sensorTicker;
#define SENSOR_TIME 60  // second
static bool sensorTickerTimeoutFlag = true;

#define CMD_SWITCH_UNKNOWN -1
#define CMD_SWITCH_ON 1
#define CMD_SWITCH_OFF 0

/**** config ****/
WiFiManager wm;

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
  // Serial.println(key + ": " + (f ? "on" : "off"));
  return f ? CMD_SWITCH_ON : CMD_SWITCH_OFF;
}

void onMqttMessage(int len) {
  // Serial.println("onMqttMessage");
  String topic = mqtt.messageTopic();
  // Serial.println(topic);
  JSONVar root;
  JSONVar cmd;

  String data = mqtt.readString();
  // Serial.println(data);

  if (topic == subTopic) {
    cmd = JSON.parse(data);
    if (JSON.typeof(root) == "undefined") {
      // Serial.println("Parsing cmd failed!");
      return;
    }
#if USE_TUYA
  } else if (topic == setTopic) {
    root = JSON.parse(data);
    if (JSON.typeof(root) == "undefined") {
      // Serial.println("Parsing setObj failed!");
      return;
    }
    if (root.hasOwnProperty("data") == false) {
      // Serial.println("Parsing data failed!");
      return;
    }
    cmd = root["data"];
  } else if (topic == execTopic) {
    root = JSON.parse(data);
    if (JSON.typeof(root) == "undefined") {
      // Serial.println("Parsing execObj failed!");
      return;
    }
    if (root.hasOwnProperty("data") == false) {
      // Serial.println("Parsing data failed!");
      return;
    }
    JSONVar data = root["data"];
    if (data.hasOwnProperty("actionCode") == false) {
      // Serial.println("Parsing actionCode failed!");
      return;
    }
    const char* actionCode = data["actionCode"];
    // Serial.println(actionCode);
    cmd[actionCode] = "1";
#endif
  } else {
    // Serial.println("Unknown topic");
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

  if (cmd.hasOwnProperty("counterReset")) {
    clearEnergyCounters();
  }

  if (cmd.hasOwnProperty("sensorUpdate")
#if USE_TUYA
      || topic != subTopic
#endif
  ) {
    sensorUpdate();
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
  // Serial.print("Connecting to ");
  // Serial.println(wm.getWiFiSSID());
  WiFi.begin(wm.getWiFiSSID(), wm.getWiFiPass());
  // // Serial.println(WLAN_SSID);
  // WiFi.begin(WLAN_SSID, WLAN_PASS);
  // WiFiMulti.addAP(WLAN_SSID, WLAN_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    // while (WiFiMulti.run() != WL_CONNECTED) {
    delay(250);
    // Serial.print(".");
    digitalWrite(LED_BLUE, !digitalRead(LED_BLUE));
  }
  // Serial.println();
  digitalWrite(LED_BLUE, LED_ON);

  // Serial.println("WiFi connected");
  // Serial.println("IP address: ");
  // Serial.println(WiFi.localIP());
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

#if USE_TUYA
  while (time(nullptr) < TIMESTAMP_YEAR2024) {
    // Serial.println("waiting for ntp sync");
    delay(1000);
  }

  clientId = "tuyalink_" + TYDeviceID;

  String timestamp = String(time(nullptr));
  username = TYDeviceID + "|signMethod=hmacSha256,timestamp=" + timestamp +
             ",secureMode=1,accessType=1";
  String content = "deviceId=" + TYDeviceID + ",timestamp=" + timestamp +
                   ",secureMode=1,accessType=1";

  password = experimental::crypto::SHA256::hmac(content, TYDeviceSecret.c_str(),
                                                TYDeviceSecret.length(), 32);
  password.toLowerCase();
  subTopic = "tylink/" + TYDeviceID + "/channel/raw/down";
  pubTopic = "tylink/" + TYDeviceID + "/channel/raw/up";
  setTopic = "tylink/" + TYDeviceID + "/thing/property/set";
  execTopic = "tylink/" + TYDeviceID + "/thing/action/execute";
#else
  subTopic = String("/") + username + "/cmd";
  pubTopic = String("/") + username + "/status";

  username = "ESP_" + String(idChar);
  clientId = "ESP_" + String(idChar);
  password = (const char*)conf["password"];
#endif
  // Serial.printf("clientId: %s\nusername: %s\npassword: %s\nsub topic: %s\n",
  //               clientId.c_str(), username.c_str(), password.c_str(),
  //               subTopic.c_str());

  mqtt.setId(clientId);
  mqtt.setUsernamePassword(username, password);
  mqtt.setKeepAliveInterval(MQTT_PING_TIME);
  mqtt.onMessage(onMqttMessage);
}

void mqttConnect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  // Serial.printf("Connecting to mqtt-> %s:%d\n",
  //               (const char*)conf["mqtt_server"], (int)conf["mqtt_port"]);

  uint8_t retries = 5;
  while (!mqtt.connect(
      (const char*)conf["mqtt_server"],
      (int)conf["mqtt_port"])) {  // connect will return 0 for connected
    digitalWrite(LED_RED, !digitalRead(LED_RED));
    // Serial.print("MQTT connection failed! Error code = ");
    // Serial.println(mqtt.connectError());
    char buf[256] = {0};
    int lastSSLError = client.getLastSSLError(buf, sizeof(buf));
    if (lastSSLError) {
      String caStr = (const char*)conf["ca_cert"];
      caStr.replace(" CERTIFICATE", "#CERTIFICATE");
      caStr.replace(" ", "\n");
      caStr.replace("#", " ");
      // Serial.printf("ca_cert: %s\n", caStr.c_str());
      // Serial.printf("fingerprint: %s\n", (const char*)conf["fingerprint"]);
      // Serial.printf("SSL error: %d\n%s\n", lastSSLError, buf);
    }
    // Serial.println("Retrying MQTT connection in 5 seconds...");
    // mqtt.disconnect();
    delay(1000);
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      // while (1) {
      //   delay(1000);
      // }
      return;
    }
  }

  // Serial.println("MQTT Connected!");

  mqtt.subscribe(subTopic);
#if USE_TUYA
  mqtt.subscribe(setTopic);
  mqtt.subscribe(execTopic);
#endif

  tickerStart();
}

void sensorTickerTimeout() { sensorTickerTimeoutFlag = true; }

void tickerStart() { sensorTicker.attach(SENSOR_TIME, sensorTickerTimeout); }

void webConfig() {
  String oriServer = (const char*)conf["mqtt_server"];
  WiFiManagerParameter custom_mqtt_server(
      "server", "mqtt server",
      oriServer.length() ? oriServer.c_str() : "m1.tuyacn.com", 64);
  int oriPort = (int)conf["mqtt_port"];
  WiFiManagerParameter custom_mqtt_port(
      "port", "mqtt port", String(oriPort ? oriPort : 8883).c_str(), 6);
  WiFiManagerParameter custom_mqtts_ca_cert("ca_cert", "ca cert",
                                            conf["ca_cert"], 2048);
  WiFiManagerParameter custom_buptnet_user("buptnet_user", "buptnet user",
                                           conf["buptnet_user"], 16);
  WiFiManagerParameter custom_buptnet_pass("buptnet_pass", "buptnet pass", NULL,
                                           64);

#if USE_TUYA
  WiFiManagerParameter custom_ty_device_id("ty_device_id", "tuya device id",
                                           conf["ty_device_id"], 64);
  WiFiManagerParameter custom_ty_device_secret("ty_device_secret",
                                               "tuya device secret", NULL, 64);
#else
  WiFiManagerParameter custom_password("password", "mqtt password",
                                       conf["password"], 64);
#endif

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtts_ca_cert);
  wm.addParameter(&custom_buptnet_user);
  wm.addParameter(&custom_buptnet_pass);

#if USE_TUYA
  wm.addParameter(&custom_ty_device_id);
  wm.addParameter(&custom_ty_device_secret);
#else
  wm.addParameter(&custom_password);
#endif

  wm.setConfigPortalBlocking(false);
  wm.startConfigPortal();
  while (wm.getConfigPortalActive()) {
    wm.process();
    delay(100);
    digitalWrite(LED_RED, !digitalRead(LED_RED));
    digitalWrite(LED_BLUE, !digitalRead(LED_BLUE));
  }
  // Serial.println("config finish");
  conf["mqtt_server"] = custom_mqtt_server.getValue();
  conf["mqtt_port"] = String(custom_mqtt_port.getValue()).toInt();

  conf["ca_cert"] = custom_mqtts_ca_cert.getValue();
  // Serial.println("ca_cert: ");
  // Serial.println(custom_mqtts_ca_cert.getValue());

#if USE_TUYA
  conf["ty_device_id"] = custom_ty_device_id.getValue();
  if (strlen(custom_ty_device_secret.getValue())) {
    conf["ty_device_secret"] = custom_ty_device_secret.getValue();
  }
#else
  conf["password"] = custom_password.getValue();
#endif

  conf["buptnet_user"] = custom_buptnet_user.getValue();
  if (strlen(custom_buptnet_pass.getValue())) {
    conf["buptnet_pass"] = custom_buptnet_pass.getValue();
  }
  saveConfig();
  wm.reboot();
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
      sensorTickerTimeoutFlag = true;  // force update
      return;
    }
  }

  setRy1(false);
  setRy2(false);
  sensorTickerTimeoutFlag = true;  // force update

  for (int i = 0; i < 400; i++) {
    delayMicroseconds(10000);
    if (digitalRead(BUTTON) == HIGH) {
      return;
    }
  }

  wm.reboot();
}

void sensorUpdate() {
  // Serial.println("sensor update");
  // digitalWrite(LED_RED, !digitalRead(LED_RED));

  JSONVar data;
  data["ry1"] = digitalRead(RY1_IO) == RY_ON;
  data["ry2"] = digitalRead(RY2_IO) == RY_ON;

  sy7t609_info_t info = getSy7t609Info();
  sy7t609MeasurementProcess();

#if USE_TUYA
  data["power"] = info.power / 1000.0;
  data["avg_power"] = info.avg_power / 1000.0;
  data["vrms"] = info.vrms / 1000.0;
  data["irms"] = info.irms / 1000.0;
  data["freq"] = info.freq / 1000.0;
  data["pf"] = info.pf / 1000.0;
  data["epp_cnt"] = info.epp_cnt;
  // data["epm_cnt"] = info.epm_cnt;
#else
  data["power"] = info.power;
  data["avg_power"] = info.avg_power;
  data["vrms"] = info.vrms;
  data["irms"] = info.irms;
  data["freq"] = info.freq;
  data["pf"] = info.pf;
  data["epp_cnt"] = info.epp_cnt;
  data["epm_cnt"] = info.epm_cnt;
#endif

#if USE_TUYA
  // add special data for tuya
  mqttPublish(pubTopic, (" " + JSON.stringify(data)).c_str());
#else
  mqttPublish(pubTopic, JSON.stringify(data).c_str());
#endif
}

void mqttPublish(String& topic, const char* data) {
  if (!mqtt.connected()) {
    return;
  }
  // Serial.print("Publishing to ");
  // Serial.print(topic);
  // Serial.print(" -> ");
  // Serial.println(data);

  mqtt.beginMessage(topic, strlen(data), false, 0, false);
  mqtt.print(data);
  mqtt.endMessage();
}

void reportErr(String name, size_t errCode) {
  return;

  if (!mqtt.connected()) {
    return;
  }

  JSONVar data;
  data["err"] = name;
  data["errCode"] = errCode;
  mqttPublish(pubTopic, JSON.stringify(data).c_str());
}

void setup() {
  ioInit();
  setRy1(false);
  setRy2(false);
  digitalWrite(LED_PWR, LED_ON);
  digitalWrite(LED_RED, LED_ON);
  digitalWrite(LED_BLUE, LED_ON);

  Serial.begin(9600);
  // Serial.println();
  // Serial.print(F("\nReset reason = "));
  auto rstInfo = ESP.getResetInfoPtr();
  // Serial.println(rstInfo->reason);

  uint32_t id = system_get_chip_id();
  sprintf(idChar, "%06X", id);
  // Serial.print("chip id: ");
  // Serial.println(id, HEX);

  if (!LittleFS.begin()) {
    // Serial.println("Failed to mount file system");
    return;
  }

  delay(1000);
  digitalWrite(LED_PWR, LED_OFF);
  delay(1000);
  digitalWrite(LED_BLUE, LED_OFF);
  digitalWrite(LED_RED, LED_OFF);

  bool loadRes = loadConfig();

#ifdef TEST_H
  if (digitalRead(BUTTON) == LOW) {
    webConfig();
  }
#else
  if (rstInfo->reason == REASON_EXT_SYS_RST || digitalRead(BUTTON) == LOW) {
    webConfig();
  }
#endif

  attachInterrupt(digitalPinToInterrupt(BUTTON), handleKeyPress, FALLING);

#ifdef TEST_H
  conf["mqtt_server"] = MQTT_SERVER;
  conf["mqtt_port"] = MQTT_PORT;
  conf["ca_cert"] = CA_CERT;
  conf["buptnet_user"] = BUPTNET_USER;
  conf["buptnet_pass"] = BUPTNET_PASS;
  conf["ty_device_id"] = TY_DEVICE_ID;
  conf["ty_device_secret"] = TY_DEVICE_SECRET;
  conf["password"] = PASSWORD;
#else
  if (!loadRes || !wm.getWiFiIsSaved()) {
    // Serial.println("Failed to load config");
    while (1) {
      delay(1000);
    }
  }
#endif

#if USE_TUYA
  // Serial.println("use tuya");
  TYDeviceID = (const char*)conf["ty_device_id"];
  TYDeviceSecret = (const char*)conf["ty_device_secret"];
  if (!TYDeviceID.length() || !TYDeviceSecret.length()) {
    // Serial.println("tuya device id or secret is empty");
    while (1) {
      delay(1000);
    }
  }
#endif

  wifiInit();

  String reUrl;
  if (!testPage("http://www.example.com/?cmd=redirect&arubalp=12345", reUrl)) {
    // Serial.println(reUrl);
    testPage(reUrl);
    int pos = reUrl.indexOf('/', 7);
    String serverUrl = reUrl.substring(0, pos);
    // Serial.println(serverUrl);
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

  mqttConnect();

  uint8_t err = initSy7t609();
  if (err) {
    reportErr("initSy7t609", err);
  }
}

void loop() {
  if (!WiFi.isConnected()) {
    delay(150);
    digitalWrite(LED_BLUE, !digitalRead(LED_BLUE));
  } else {
    digitalWrite(LED_BLUE, LED_ON);
    mqttConnect();
    if (mqtt.connected()) {
      digitalWrite(LED_RED, LED_ON);
      mqtt.poll();
      if (sensorTickerTimeoutFlag) {
        sensorTickerTimeoutFlag = false;
        sensorUpdate();
      }
    }
  }

  delay(50);
}
