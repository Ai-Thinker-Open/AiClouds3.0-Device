#include <stdio.h>
#include "esp_system.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "common.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_err.h"
#include "internal/esp_wifi_internal.h"
#include "nvs_flash.h"
#include "rom/ets_sys.h"
#include "driver/uart.h"
#include "lwip/netdb.h"
#include "driver/gpio.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
#include "smartconfig_ack.h"
#include "airkiss.h"
#include "driver/hw_timer.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "mbedtls/base64.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "rom/ets_sys.h"
#include "router.h"
#include "button.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "xpwm.h"

void TaskSmartConfigAirKiss2Net(void *parm);

// *    基于 esp-idf esp8266芯片 rtos3.0 sdk 开发，配合 xClouds-php 可实现微信配网绑定控制 + 天猫精灵语音控制 + 小爱同学控制；
//  *
//  *   这是esp-touch或 微信airkiss配网以及近场发现的功能和连接MQTT服务器的的demo示范！
//  *
//  *   LED接线参考 XPWM.h 头文件定义，按键接线 GPIO0 下降沿有效；
//  *   按键长按 ，进去配网模式，搜索 "安信可科技" 微信公众号点击 WiFi配置；
//  *
//  *    有任何技术问题邮箱： 870189248@qq.com
//  *    @team: Ai-Thinker Open Team 安信可开源团队-半颗心脏

typedef struct __User_data
{
	char allData[1024];
	int dataLen;
} User_data;

User_data user_data;

static const char *TAG = "AIThinker-2.XClouds Log";
static EventGroupHandle_t wifi_event_group;
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const int AIRKISS_DONE_BIT = BIT2;
static xTaskHandle handleLlocalFind = NULL;
static xTaskHandle handleMqtt = NULL;
static xQueueHandle ParseJSONQueueHandler = NULL; //解析json数据的队列
static xTaskHandle mHandlerParseJSON = NULL;	  //任务队列

#define BUF_SIZE (1024)
//近场发现自定义消息
uint8_t deviceInfo[100] = {};
//当前是否配网模式
int flagNet = 0;
char deviceUUID[17];
char MqttTopicSub[30], MqttTopicPub[30];
int sock_fd;

//按键定义
#define BUTTON_GPIO 0
//设备信息
#define DEVICE_TYPE "rgbLight"
//设备信息
#define FW_VERSION "1.0"
//mqtt
static esp_mqtt_client_handle_t client;

//是否连接服务器
bool isConnect2Server = false;

char udp_msg[512]; //固定的本地广播数据

/* 
 * @Description: 上报数据到服务器
 * @param: null
 * @return: 
*/
static void post_data_to_clouds()
{

	if (!isConnect2Server)
		return;

	char colorMode[10];

	if (light_driver_get_color_mode(colorMode) != ESP_OK)
	{
		sprintf(colorMode, "%s", "NULL");
	}

	cJSON *pRoot = cJSON_CreateObject();
	cJSON *pHeader = cJSON_CreateObject();
	cJSON *pAttr = cJSON_CreateArray();

	/* add 1st car to cars array */
	cJSON_AddStringToObject(pHeader, "type", (DEVICE_TYPE));
	cJSON_AddStringToObject(pHeader, "fw", (FW_VERSION));
	cJSON_AddStringToObject(pHeader, "mac", deviceUUID);

	//开关
	cJSON *pAttrPower = cJSON_CreateObject();
	cJSON_AddStringToObject(pAttrPower, "name", "powerstate");
	cJSON_AddStringToObject(pAttrPower, "value", light_driver_get_switch() == true ? ("on") : ("off"));
	cJSON_AddItemToArray(pAttr, pAttrPower);

	//色温
	cJSON *pAttr_colorTemperature = cJSON_CreateObject();
	cJSON_AddStringToObject(pAttr_colorTemperature, "name", "colorTemperature");
	cJSON_AddStringToObject(pAttr_colorTemperature, "value", itoaa(light_driver_get_color_temperature()));
	cJSON_AddItemToArray(pAttr, pAttr_colorTemperature);

	//情景模式
	cJSON *pAttr_mode = cJSON_CreateObject();
	cJSON_AddStringToObject(pAttr_mode, "name", "mode");
	cJSON_AddStringToObject(pAttr_mode, "value", itoaa(light_driver_get_mode()));
	cJSON_AddItemToArray(pAttr, pAttr_mode);

	//亮度
	cJSON *pAttr_brightness = cJSON_CreateObject();
	cJSON_AddStringToObject(pAttr_brightness, "name", "brightness");
	cJSON_AddStringToObject(pAttr_brightness, "value", itoaa(light_driver_get_brightness()));
	cJSON_AddItemToArray(pAttr, pAttr_brightness);

	//颜色
	cJSON *pAttr_color = cJSON_CreateObject();
	cJSON_AddStringToObject(pAttr_color, "name", "color");
	cJSON_AddStringToObject(pAttr_color, "value", colorMode);
	cJSON_AddItemToArray(pAttr, pAttr_color);

	cJSON_AddItemToObject(pRoot, "header", pHeader);
	cJSON_AddItemToObject(pRoot, "attr", pAttr);

	char *s = cJSON_Print(pRoot);

	ESP_LOGI(TAG, "post_data_to_clouds topic end : %s", MqttTopicPub);
	ESP_LOGI(TAG, "post_data_to_clouds payload : %s", s);
	esp_mqtt_client_publish(client, MqttTopicPub, s, strlen(s), 1, 0);
	cJSON_free((void *)s);
	cJSON_Delete(pRoot);
}

/* 
 * @Description: 解析下发数据的队列逻辑处理
 * @param: null
 * @return: 
*/
void Task_ParseJSON(void *pvParameters)
{
	printf("[SY] Task_ParseJSON_Message creat ... \n");
	while (1)
	{
		struct __User_data *pMqttMsg;

		printf("[%d] Task_ParseJSON_Message xQueueReceive wait ... \n", esp_get_free_heap_size());
		xQueueReceive(ParseJSONQueueHandler, &pMqttMsg, portMAX_DELAY);

		////首先整体判断是否为一个json格式的数据
		cJSON *pJsonRoot = cJSON_Parse(pMqttMsg->allData);
		//如果是否json格式数据
		if (pJsonRoot == NULL)
		{
			printf("[SY] Task_ParseJSON_Message xQueueReceive not json ... \n");
			goto __cJSON_Delete;
		}

		cJSON *pJSON_Header = cJSON_GetObjectItem(pJsonRoot, "header");

		if (!pJSON_Header)
		{
			printf("[SY] Task_ParseJSON_Message pJSON_Header not json ... \n");
			post_data_to_clouds(client, MqttTopicPub);

			goto __cJSON_Delete;
		}

		cJSON *pJSON_Payload = cJSON_GetObjectItem(pJsonRoot, "payload");
		cJSON *pJSON_Name = cJSON_GetObjectItem(pJSON_Header, "name");
		cJSON *cjsonValue = cJSON_GetObjectItem(pJSON_Payload, "value");

		if (!pJSON_Payload)
		{
			printf("[SY] Task_ParseJSON_Message pJSON_Payload not json ... \n");
			goto __cJSON_Delete;
		}

		if (!pJSON_Name)
		{
			printf("[SY] Task_ParseJSON_Message pJSON_Name not json ... \n");
			goto __cJSON_Delete;
		}

		if (!cjsonValue)
		{
			printf("[SY] Task_ParseJSON_Message cjsonValue not json ... \n");
			goto __cJSON_Delete;
		}

		// //start TurnOn
		if (strcmp(pJSON_Name->valuestring, "TurnOn") == 0)
		{
			//set Value ！执行开灯指令
			light_driver_set_switch(1);
		}
		//end TurnOn

		//start TurnOff
		else if (strcmp(pJSON_Name->valuestring, "TurnOff") == 0)
		{
			light_driver_set_switch(0);
		}
		//end TurnOn

		// start  SetColorTemperature
		else if (strcmp(pJSON_Name->valuestring, "SetColorTemperature") == 0)
		{
			cJSON *pJSON_Value = cJSON_GetObjectItem(pJSON_Payload, "value");
			//判断是否为字符串类型
			if (cJSON_IsString(pJSON_Value))
			{
				//判断是否最大色温
				if (strcmp(pJSON_Value->valuestring, "max") == 0)
				{
					/* code */ light_driver_set_colorTemperature(APK_MIN_COLORTEMP);
				}
				//判断是否最小色温
				else if (strcmp(pJSON_Value->valuestring, "min") == 0)
				{
					/* code */ light_driver_set_colorTemperature(APK_MAX_COLORTEMP);
				}
				else
				{
					/* code */ light_driver_set_colorTemperature(APK_MAX_COLORTEMP - atoi(pJSON_Value->valuestring) + APK_MIN_COLORTEMP);
				}
			}
			else
			{
				light_driver_set_colorTemperature(APK_MAX_COLORTEMP - (pJSON_Value->valueint) + APK_MIN_COLORTEMP);
			}
		}
		// end SetColorTemperature

		// start  SetBrightness
		else if (strcmp(pJSON_Name->valuestring, "SetBrightness") == 0)
		{
			cJSON *pJSON_Value = cJSON_GetObjectItem(pJSON_Payload, "value");

			//判断是否为字符串类型
			if (cJSON_IsString(pJSON_Value))
			{
				//判断是否最大亮度
				if (strcmp(pJSON_Value->valuestring, "max") == 0)
				{
					/* code */ light_driver_set_brightness(100);
				}
				//判断是否最小亮度
				else if (strcmp(pJSON_Value->valuestring, "min") == 0)
				{
					/* code */ light_driver_set_brightness(0);
				}
				else
				{
					/* code */ light_driver_set_brightness(atoi(pJSON_Value->valuestring));
				}
			}
			else
			{
				light_driver_set_brightness((pJSON_Value->valueint));
			}
		}
		// end SetBrightness

		//start SetColor
		else if (strcmp(pJSON_Name->valuestring, "SetColor") == 0)
		{
			cJSON *pJSON_Value = cJSON_GetObjectItem(pJSON_Payload, "value");
			if (cJSON_IsString(pJSON_Value))
			{
				if (strcmp(pJSON_Value->valuestring, "Red") == 0)
				{
					light_driver_set_rgb(APK_MAX_COLOR, 0, 0);
				}
				else if (strcmp(pJSON_Value->valuestring, "Green") == 0)
				{
					light_driver_set_rgb(0, APK_MAX_COLOR, 0);
				}
				else if (strcmp(pJSON_Value->valuestring, "Blue") == 0)
				{
					light_driver_set_rgb(0, 0, APK_MAX_COLOR);
				}
				else if (strcmp(pJSON_Value->valuestring, "Yellow") == 0)
				{
					light_driver_set_rgb(APK_MAX_COLOR, APK_MAX_COLOR, 0);
				}
				else if (strcmp(pJSON_Value->valuestring, "White") == 0)
				{
					light_driver_set_rgb(APK_MAX_COLOR, APK_MAX_COLOR, APK_MAX_COLOR);
				}
			}
			else if (cJSON_IsArray(pJSON_Value))
			{
				light_driver_set_rgb(cJSON_GetArrayItem(pJSON_Value, 0),
									 cJSON_GetArrayItem(pJSON_Value, 1),
									 cJSON_GetArrayItem(pJSON_Value, 2));
			}
		}
		//end SetColor

		//start SetColor
		else if (strcmp(pJSON_Name->valuestring, "SetMode") == 0)
		{
			cJSON *pJSON_Value = cJSON_GetObjectItem(pJSON_Payload, "value");
			if (strcmp(pJSON_Value->valuestring, "reading") == 0)
			{
				light_driver_set_mode(0);
			}
			else if (strcmp(pJSON_Value->valuestring, "movie") == 0)
			{
				light_driver_set_mode(1);
			}
			else if (strcmp(pJSON_Value->valuestring, "sleep") == 0)
			{
				light_driver_set_mode(2);
			}
			else if (strcmp(pJSON_Value->valuestring, "live") == 0)
			{
				light_driver_set_mode(3);
			}
			else if (strcmp(pJSON_Value->valuestring, "nightLight") == 0 || strcmp(pJSON_Value->valuestring, "night") == 0)
			{
				light_driver_set_mode(4);
			}
		}
		//end SetColor

		post_data_to_clouds();

	__cJSON_Delete:
		cJSON_Delete(pJsonRoot);
	}
}
/* 
 * @Description: MQTT服务器的下发消息回调
 * @param: 
 * @return: 
*/
esp_err_t MqttCloudsCallBack(esp_mqtt_event_handle_t event)
{
	int msg_id;
	client = event->client;
	switch (event->event_id)
	{
		//连接成功
	case MQTT_EVENT_CONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
		msg_id = esp_mqtt_client_subscribe(client, MqttTopicSub, 1);
		ESP_LOGI(TAG, "sent subscribe[%s] successful, msg_id=%d", MqttTopicSub, msg_id);
		ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
		//post_data_to_clouds();
		isConnect2Server = true;
		break;
		//断开连接回调
	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
		isConnect2Server = false;
		break;
		//订阅成功
	case MQTT_EVENT_SUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
		break;
		//订阅失败
	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		break;
		//推送发布消息成功
	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		break;
		//服务器下发消息到本地成功接收回调
	case MQTT_EVENT_DATA:
	{
		printf("TOPIC=%.*s \r\n", event->topic_len, event->topic);
		printf("DATA=%.*s \r\n\r\n", event->data_len, event->data);
		//发送数据到队列
		struct __User_data *pTmper;
		sprintf(user_data.allData, "%s", event->data);
		pTmper = &user_data;
		user_data.dataLen = event->data_len;
		xQueueSend(ParseJSONQueueHandler, (void *)&pTmper, portMAX_DELAY);
		break;
	}
	default:
		break;
	}
	return ESP_OK;
}

/* 
 * @Description: MQTT参数连接的配置
 * @param: 
 * @return: 
*/
void TaskXMqttRecieve(void *p)
{
	//连接的配置参数
	esp_mqtt_client_config_t mqtt_cfg = {
		.host = "www.xuhongv.com", //连接的域名 ，请务必修改为您的
		.port = 1883,			   //端口，请务必修改为您的
		.username = "admin",	   //用户名，请务必修改为您的
		.password = "xuhong123",   //密码，请务必修改为您的
		.client_id = deviceUUID,
		.event_handle = MqttCloudsCallBack, //设置回调函数
		.keepalive = 120,					//心跳
		.disable_auto_reconnect = false,	//开启自动重连
		.disable_clean_session = false,		//开启 清除会话
	};
	client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_start(client);

	vTaskDelete(NULL);
}

/* 
 * @Description:  微信配网近场发现，可注释不要
 * @param: 
 * @return: 
*/
static void TaskCreatSocket(void *pvParameters)
{

	printf("TaskCreatSocket to create!\n");

	char rx_buffer[128];
	uint8_t tx_buffer[512];
	uint8_t lan_buf[300];
	uint16_t lan_buf_len;
	struct sockaddr_in server_addr;
	int sock_server; /* server socked */
	int err;
	int counts = 0;
	size_t len;

	sock_server = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_server == -1)
	{
		printf("failed to create sock_fd!\n");
		vTaskDelete(NULL);
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // inet_addr("255.255.255.255");
	server_addr.sin_port = htons(LOCAL_UDP_PORT);

	err = bind(sock_server, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (err == -1)
	{
		vTaskDelete(NULL);
	}

	//base64加密要发送的数据
	if (mbedtls_base64_encode(tx_buffer, strlen((char *)tx_buffer), &len, deviceInfo, strlen((char *)deviceInfo)) != 0)
	{
		printf("[xuhong] fail mbedtls_base64_encode %s\n", tx_buffer);
		vTaskDelete(NULL);
	}

	printf("[xuhong] mbedtls_base64_encode %s\n", tx_buffer);

	struct sockaddr_in sourceAddr;
	socklen_t socklen = sizeof(sourceAddr);
	while (1)
	{
		memset(rx_buffer, 0, sizeof(rx_buffer));
		int len = recvfrom(sock_server, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&sourceAddr, &socklen);

		ESP_LOGI(TAG, "IP:%s:%d", (char *)inet_ntoa(sourceAddr.sin_addr), htons(sourceAddr.sin_port));
		//ESP_LOGI(TAG, "Received %s ", rx_buffer);

		// Error occured during receiving
		if (len < 0)
		{
			ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
			break;
		}
		// Data received
		else
		{
			rx_buffer[len] = 0;												   // Null-terminate whatever we received and treat like a string
			airkiss_lan_ret_t ret = airkiss_lan_recv(rx_buffer, len, &akconf); //检测是否为微信发的数据包
			airkiss_lan_ret_t packret;
			switch (ret)
			{
			case AIRKISS_LAN_SSDP_REQ:

				lan_buf_len = sizeof(lan_buf);
				//开始组装打包
				packret = airkiss_lan_pack(AIRKISS_LAN_SSDP_NOTIFY_CMD, ACCOUNT_ID, tx_buffer, 0, 0, lan_buf, &lan_buf_len, &akconf);
				if (packret != AIRKISS_LAN_PAKE_READY)
				{
					ESP_LOGE(TAG, "Pack lan packet error!");
					continue;
				}
				ESP_LOGI(TAG, "Pack lan packet ok ,send: %s", tx_buffer);
				//发送至微信客户端
				int err = sendto(sock_server, (char *)lan_buf, lan_buf_len, 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));
				if (err < 0)
				{
					ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
				}
				else if (counts++ > COUNTS_BOACAST)
				{
					shutdown(sock_fd, 0);
					closesocket(sock_fd);
					handleLlocalFind = NULL;
					vTaskDelete(NULL);
				}
				break;
			default:
				break;
			}
		}
	}
}
static void TaskRestartSystem(void *p)
{
	router_wifi_clean_info();
	vTaskDelay(2500 / portTICK_RATE_MS);
	esp_restart();
	vTaskDelete(NULL);
}

/* 
 * @Description: 微信配网
 * @param: 
 * @return: 
*/
bool startAirkissTask()
{

	ESP_LOGI(TAG, "startAirkissTask");
	int ret = pdFAIL;
	if (handleLlocalFind == NULL)
		ret = xTaskCreate(TaskCreatSocket, "TaskCreatSocket", 1024 * 2, NULL, 6, &handleLlocalFind);

	if (ret != pdPASS)
	{
		printf("create airkiss thread failed.\n");
		return false;
	}
	else
	{
		return true;
	}
}

/* 
 * @Description:  系统的wifi协议栈回调
 * @param: 
 * @param: 
 * @return: 
*/
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	switch (event->event_id)
	{
	case SYSTEM_EVENT_STA_START:
		xTaskCreate(TaskSmartConfigAirKiss2Net, "TaskSmartConfigAirKiss2Net", 1024 * 2, NULL, 3, NULL);
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
	{
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		int ret = pdFAIL;
		if (handleMqtt == NULL)
			ret = xTaskCreate(TaskXMqttRecieve, "TaskXMqttRecieve", 1024 * 4, NULL, 5, &handleMqtt);

		if (ParseJSONQueueHandler == NULL)
			ParseJSONQueueHandler = xQueueCreate(5, sizeof(struct esp_mqtt_msg_type *));

		//开启json解析线程
		if (mHandlerParseJSON == NULL)
		{
			xTaskCreate(Task_ParseJSON, "Task_ParseJSON", 1024, NULL, 3, &mHandlerParseJSON);
		}

		if (ret != pdPASS)
		{
			printf("create TaskXMqttRecieve thread failed.\n");
		}

		break;
	}

	case SYSTEM_EVENT_STA_DISCONNECTED:
		esp_wifi_connect();
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		isConnect2Server = false;
		break;
	default:
		break;
	}
	return ESP_OK;
}

/* 
 * @Description: 配网回调
 * @param: 
 * @param: 
 * @return: 
*/
static void sc_callback(smartconfig_status_t status, void *pdata)
{
	switch (status)
	{
	case SC_STATUS_LINK:
	{
		wifi_config_t *wifi_config = pdata;
		ESP_LOGI(TAG, "SSID:%s", wifi_config->sta.ssid);
		ESP_LOGI(TAG, "PASSWORD:%s", wifi_config->sta.password);
		ESP_ERROR_CHECK(esp_wifi_disconnect());
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config));
		ESP_ERROR_CHECK(esp_wifi_connect());

		router_wifi_save_info(wifi_config->sta.ssid, wifi_config->sta.password);
	}
	break;
	case SC_STATUS_LINK_OVER:
		ESP_LOGI(TAG, "SC_STATUS_LINK_OVER");
		//这里乐鑫回调目前在master分支已区分是否为微信配网还是esptouch配网，当airkiss配网才近场回调！
		if (pdata != NULL)
		{
			sc_callback_data_t *sc_callback_data = (sc_callback_data_t *)pdata;
			switch (sc_callback_data->type)
			{
			case SC_ACK_TYPE_ESPTOUCH:
				ESP_LOGI(TAG, "Phone ip: %d.%d.%d.%d", sc_callback_data->ip[0], sc_callback_data->ip[1], sc_callback_data->ip[2], sc_callback_data->ip[3]);
				ESP_LOGI(TAG, "TYPE: ESPTOUCH");
				xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
				break;
			case SC_ACK_TYPE_AIRKISS:
				ESP_LOGI(TAG, "TYPE: AIRKISS");
				xEventGroupSetBits(wifi_event_group, AIRKISS_DONE_BIT);
				break;
			default:
				ESP_LOGE(TAG, "TYPE: ERROR");
				xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
				break;
			}
		}

		break;
	default:
		break;
	}
}

/* 
 * @Description:  一键配网
 * @param: 
 * @return: 
*/
void TaskSmartConfigAirKiss2Net(void *parm)
{
	EventBits_t uxBits;
	//判别是否自动连接
	bool isAutoConnect = routerStartConnect();
	//是的，则不进去配网模式，已连接路由器
	if (isAutoConnect)
	{
		ESP_LOGI(TAG, "Next connectting router.");
		vTaskDelete(NULL);
	}
	//否，进去配网模式
	else
	{
		ESP_LOGI(TAG, "into smartconfig mode");
		ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS));
		ESP_ERROR_CHECK(esp_smartconfig_start(sc_callback));
	}
	//阻塞等待配网完成结果
	while (1)
	{
		uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT | AIRKISS_DONE_BIT, true, false, portMAX_DELAY);
		// 微信公众号配网完成
		if (uxBits & AIRKISS_DONE_BIT)
		{
			ESP_LOGI(TAG, "smartconfig over , start find device");
			esp_smartconfig_stop();

			//把设备信息通知到微信公众号
			if (handleLlocalFind == NULL)
				xTaskCreate(TaskCreatSocket, "TaskCreatSocket", 1024 * 2, NULL, 6, &handleLlocalFind);

			ESP_LOGI(TAG, "getAirkissVersion %s", airkiss_version());

			vTaskDelete(NULL);
		}
		// smartconfig配网完成
		if (uxBits & ESPTOUCH_DONE_BIT)
		{
			ESP_LOGI(TAG, "smartconfig over , but don't find device by airkiss...");
			esp_smartconfig_stop();
			vTaskDelete(NULL);
		}
	}
}

//短按函数
static void ButtonShortPressCallBack(void *arg)
{
	ESP_LOGI(TAG, "ButtonShortPressCallBack  esp_get_free_heap_size(): %d ", esp_get_free_heap_size());
	light_driver_set_switch(!light_driver_get_switch());
	post_data_to_clouds();
}
//长按函数
static void ButtonLongPressCallBack(void *arg)
{
	ESP_LOGI(TAG, "ButtonLongPressCallBack  esp_get_free_heap_size(): %d ", esp_get_free_heap_size());
	light_driver_set_cycle(2);
	//重启并进去配网模式
	xTaskCreate(TaskRestartSystem, "TaskRestartSystem", 1024, NULL, 6, NULL);
}

/**
 * @description: 按键驱动
 * @param {type} 
 * @return: 
 */
void TaskButton(void *pvParameters)
{
	//定义一个 gpio 下降沿触发: GPIO口，回调函数个数0开始，下降沿有效
	button_handle_t btn_handle = button_dev_init(BUTTON_GPIO, 1, BUTTON_ACTIVE_LOW);
	// 50ms按钮短按
	// BUTTON_PUSH_CB 表示按下就触发回调函数，如果设置了长按，这个依然会同时触发！
	// BUTTON_RELEASE_CB 表示释放才回调，如果设置了长按，这个依然会同时触发！
	// BUTTON_TAP_CB 此选项释放才回调，如果设置了长按，这个不会同时触发！
	button_dev_add_tap_cb(BUTTON_TAP_CB, ButtonShortPressCallBack, "TAP", 50 / portTICK_PERIOD_MS, btn_handle);
	// 设置长按 2s后触发
	button_dev_add_press_cb(0, ButtonLongPressCallBack, NULL, 2000 / portTICK_PERIOD_MS, btn_handle);

	vTaskDelete(NULL);
}

/******************************************************************************
 * FunctionName : app_main
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void app_main(void)
{
	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	printf("\n\n-------------------------------- Get Systrm Info------------------------------------------\n");
	//获取IDF版本
	printf("     SDK version:%s\n", esp_get_idf_version());
	//获取芯片可用内存
	printf("     esp_get_free_heap_size : %d  \n", esp_get_free_heap_size());
	//获取从未使用过的最小内存
	printf("     esp_get_minimum_free_heap_size : %d  \n", esp_get_minimum_free_heap_size());
	//获取芯片的内存分布，返回值具体见结构体 flash_size_map
	printf("     system_get_flash_size_map(): %d \n", CHANNLE_PWM_TOTAL);
	//获取mac地址（station模式）
	uint8_t mac[6];
	esp_read_mac(mac, ESP_MAC_WIFI_STA);
	sprintf(deviceUUID, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	sprintf((char *)deviceInfo, "{\"type\":\"%s\",\"mac\":\"%s\"}", DEVICE_TYPE, deviceUUID);
	//组建MQTT订阅的主题
	sprintf(MqttTopicSub, "/%s/%s/devSub", DEVICE_TYPE, deviceUUID);
	//组建MQTT推送的主题
	sprintf(MqttTopicPub, "/%s/%s/devPub", DEVICE_TYPE, deviceUUID);

	ESP_LOGI(TAG, "flagNet: %d", flagNet);
	ESP_LOGI(TAG, "deviceUUID: %s", deviceUUID);
	ESP_LOGI(TAG, "deviceInfo: %s", deviceInfo);
	ESP_LOGI(TAG, "MqttTopicSub: %s", MqttTopicSub);
	ESP_LOGI(TAG, "MqttTopicPub: %s", MqttTopicPub);

	//外设初始化
	xTaskCreate(TaskButton, "TaskButton", 1024, NULL, 6, NULL);
	pwm_init_data();
	light_driver_set_switch(1);

	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
}