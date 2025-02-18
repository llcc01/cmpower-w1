# CMPOWER-W1 定制固件

## 固件功能

- WiFiManager 配置无线及其他参数，断电保存，无线OTA
- MQTT + TLS 链接服务器，接受指令控制，定时上报状态
- 支持通用MQTT模式和涂鸦MQTT模式
- 自动检测北邮校园网并登录

## 开发环境

- Arduino IDE

## CMPOWER-W1 硬件

### 配置

- ESP-12S 模块
- 带计量功能， 计量芯片为美国矽力杰 SY7T609，免校准
- 两个5A的欧姆龙继电器，两路常通，两路可控：其中一个继电器控制总火线，另一个控制两路的火线

### ESP8266 引脚连接

- 干路继电器：GPIO0, GPIO15 上升触发
- 分路继电器：GPIO12, GPIO15 上升触发
- 插排按键：GPIO4
- wifi指示灯：蓝色led-GPIO16，红色led-GPIO14 ，原厂固件的wifi指示灯用于远程配网、绑定设备序列号之类。
- 电源指示灯：GPIO5 ，原厂固件的电源指示灯用作分路继电器的指示灯
- 插排按键指示灯，与干路继电器绑定，无法独立控制。
- SY7T609 计量计：串口0通信

## MQTT

### 模式切换

编译时使用 `USE_TUYA` 宏区分是否使用涂鸦格式，目前仅适配了指令下发格式，状态上报需要使用平台的自定义透传，编写js脚本转换。

### 消息格式

JSON 封装

以下订阅和发布中ESP_000000的后6位为模块ID

#### 控制

订阅消息：/ESP_000000/cmd

可选字段：

- ry1
  - 干路继电器控制，true/false
- ry2
  - 支路继电器控制，true/false
- ry1_restart
  - 干路继电器断开整数n秒后接通，最长10秒
- ry2_restart
  - 支路继电器断开整数n秒后接通，最长10秒
- counterReset
  - 重置计量数据
- sensorUpdate
  - 更新并上报传感器数据

#### 上报

发布消息：/ESP_000000/status ，60s上报一次

字段：

- ry1
  - 干路继电器状态，true/false
- ry2
  - 支路继电器状态，true/false
- power
  - 实时功率，单位mW
- avg_power
  - 平均功率，单位mW
- vrms
  - 电压均方根值，单位mV
- irms
  - 电流均方根值，单位mA
- freq
  - 频率，单位1/1000Hz
- pf
  - 功率因数，单位1/1000
- epp_cnt
  - Positive Active Energy Count，单位Wh
- epm_cnt
  - Negative Active Energy Count，单位Wh

## 指示灯与按键

### 指示灯

上电时白红蓝全亮，两秒内依次熄灭。提示用户按下按键进入配置模式。
配置模式下红蓝灯200ms闪烁。
正常启动后，WiFi连接时蓝灯500ms闪烁，北邮校园网登录时红蓝灯1s交替闪烁。
正常连接WiFi和MQTT服务器时红蓝灯常亮。

### 按键

上电2S时检测按键状态，若按下则进入配置模式。普通模式下，长按按键1S关闭所有继电器，长按5S重启系统。

## 配置模式

进入模式后连接ESP_XXXXXX热点，自动跳转页面或手动输入 http://192.168.4.1/ 进入页面。

配置页面可修改网络，以下为其他配置

- mqtt server
  - 服务器IP地址或域名（暂时仅支持Ipv4服务器）
- mqtt port
  - 服务器端口
- ca cert
  - 与fingerprint二选一，优先ca cert。TLS使用的CA证书，可以为自签名CA证书或自行导入CA根证书
- fingerprint
  - 与ca cert二选一，优先ca cert。TLS用校验证书（由于库限制，仅支持SHA1）
- password
  - MQTT客户端密码
- buptnet_user
  - 北邮校园网用户名
- buptnet_pass
  - 北邮校园网密码，不填时不修改

## 接入 HA

作为 MQTT 客户端接入服务器，参考配置文件 `ha_configuration.yaml`

## 参考

[更新-计量已搞定]20 元的中移铁通插排：ESP+功率计量
https://bbs.hassbian.com/thread-23623-1-1.html
(出处: 『瀚思彼岸』» 智能家居技术论坛)

中移铁通插排改造成磁保持继电器【全球首款？】
https://bbs.hassbian.com/thread-23958-1-1.html
(出处: 『瀚思彼岸』» 智能家居技术论坛)
