
 *    基于 esp-idf esp8266芯片 rtos3.0 sdk 开发，配合 xClouds-php 可实现微信配网绑定控制 + 天猫精灵语音控制 + 小爱同学控制；
 * 
 *   这是esp-touch或 微信airkiss配网以及近场发现的功能和连接MQTT服务器的的demo示范！
 * 
 *   LED接线参考 XPWM.h 头文件定义，按键接线 GPIO0 下降沿有效；
 *   按键长按 ，进去配网模式，微信扫码下面微信公众号二维码点击 添加设备；
 *
 *    有任何技术问题邮箱： 870189248@qq.com
 *    @team: Ai-Thinker Open Team 安信可开源团队-半颗心脏
 

 
<p align="center">
  <img src="./docs/_static/wechat.jpeg" width="300px" height="300px" alt="Banner" />
</p>
 

## 上报指令

- 主题：/rgbLight/${设备mac地址}/devPub
- 设备上报格式：

```
{
	"header": {
		"type": "rgbLight",
		"fw": "12.5",
		"mac": "6001947a70a7"
	},
	"attr": [{
			"name": "powerstate",
			"value": "on"
		},
		{
			"name": "colorTemperature",
			"value": "4000"
		},
		{
			"name": "mode",
			"value": "nightLight"
		},
		{
			"name": "brightness",
			"value": "100"
		},
		{
			"name": "color",
			"value": "Yellow"
		}
	]
}
```
## 控制指令

- 控制开灯
- 主题：/aithinker/${设备mac地址}/devSub


paylaod：

```
// 开灯
{
	"header": {
		"name": "TurnOn",
		"namespace": "AliGenie.Iot.DeviceCenter.Control",
		"payLoadVersion ": 1
	},
	"payload": {
		"attribute": "powerstate",
		"deviceId": "9",
		"deviceType": "light",
		"value": "0"
	}
}
```

------------------

```
// 关灯
{
	"header": {
		"name": "TurnOff",
		"namespace": "AliGenie.Iot.DeviceCenter.Control",
		"payLoadVersion ": 1
	},
	"payload": {
		"attribute": "powerstate",
		"deviceId": "9",
		"deviceType": "light",
		"value": "0"
	}
}
```

--------------

```
// 设置颜色为拉蓝色
{
	"header": {
		"name": "SetColor",
		"namespace": "AliGenie.Iot.DeviceCenter.Control",
		"payLoadVersion ": 1
	},
	"payload": {
		"attribute": "color",
		"deviceId": "9",
		"deviceType": "light",
		"value": "Blue"
	}
}
```