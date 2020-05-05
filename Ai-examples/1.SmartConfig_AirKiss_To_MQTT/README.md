
 *    基于 esp-idf esp8266芯片 rtos3.0 sdk 开发，共勉！
 * 
 *   这是esp-touch或 微信airkiss配网以及近场发现的功能和连接MQTT服务器的的demo示范！
 * 
 *   按键接线 GPIO0引脚下降沿触发，LED的正极接GPIO12，负极接GND；
 *   按键短按 ，改变灯具状态并上报状态到服务器；
 *   按键长按 ，进去配网模式，搜索 "安信可科技" 微信公众号点击 WiFi配置；
 *
 *    有任何技术问题邮箱： 870189248@qq.com
 *    @team: Ai-Thinker Open Team 安信可开源团队-半颗心脏
 

## 上报指令

- 主题：/aithinker/${设备mac地址}/devPub
- 设备上报关灯：

```
{
	"header": {
		"type": "aithinker",
		"mac": "60019421dc59"
	},
	"attr": [{
		"name": "powerstate",
		"value": "off"
	}]
}
```

- 主题：/aithinker/${设备mac地址}/devPub
- 设备上报开灯：

```
{
	"header": {
		"type": "aithinker",
		"mac": "60019421dc59"
	},
	"attr": [{
		"name": "powerstate",
		"value": "on"
	}]
}
```
## 控制指令

- 控制开灯
- 主题：/aithinker/${设备mac地址}/devSub

```
{
	"msg": 1
}
```

- 控制关灯
- 主题：/aithinker/${设备mac地址}/devSub

```
{
	"msg": 0
}
```