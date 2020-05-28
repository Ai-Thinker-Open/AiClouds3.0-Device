
 *    基于 esp-idf esp8266芯片 rtos3.0 sdk 开发，共勉！
 * 
 *   直播课程  微信小程序控制安信可 NodeMCU ESP8266
 * 
 *    有任何技术问题邮箱： 870189248@qq.com
 *    @team: Ai-Thinker Open Team 安信可开源团队-半颗心脏
 

## 上报指令

- 主题：/light/deviceOut
- 设备上报 payload：

```
{
	"power": false,
	"Red": 0,
	"Gren": 186,
	"Blue": 0
}
```
## 控制指令

- 控制 rgb 对应的通道
- 主题：/light/deviceIn

```
{
	"change": "pwm",
	"value": [255, 0, 0]
}
```

- 控制关灯
- 主题：/aithinker/${设备mac地址}/devSub

```
{
	"change": "power",
	"value": "false"
}
```