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
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

void smartconfig_example_task(void *parm);
static void post_data_to_clouds(esp_mqtt_client_handle_t client, char *topic);

/**
 *    基于 esp-idf esp8266芯片 rtos3.0 sdk 开发，共勉！
 * 
 *    这是esp-touch或 微信airkiss配网以及近场发现的功能和连接MQTT服务器的的demo示范！
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 *    有任何技术问题邮箱： 870189248@qq.com
 *    @team: Ai-Thinker Open Team 安信可开源团队-半颗心脏
 *
 **/

typedef struct __User_data
{
	char allData[1024];
	int dataLen;
} User_data;

User_data user_data;

//当前是否配网模式
int flagNet = 0;

static EventGroupHandle_t wifi_event_group;
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const int AIRKISS_DONE_BIT = BIT2;

static xTaskHandle handleLlocalFind = NULL;
static xTaskHandle handleMqtt = NULL;
static xQueueHandle ParseJSONQueueHandler = NULL; //解析json数据的队列
static xTaskHandle mHandlerParseJSON = NULL;	  //任务队列

static const char *TAG = "AIThinkerDemo Num1";


//当前固件版本
#define FW_VERSION "1"

#define BUF_SIZE (1024)
//近场发现自定义消息
uint8_t deviceInfo[100] = {};

//设备信息
#define DEVICE_TYPE "rgbLight"
char deviceUUID[17];
char MqttTopicSub[30], MqttTopicPub[30];
int sock_fd;

//mqtt
static esp_mqtt_client_handle_t client;

//是否连接服务器
bool isConnect2Server = false;

char udp_msg[512]; //固定的本地广播数据

void Task_ParseJSON(void *pvParameters)
{
	printf("[SY] Task_ParseJSON_Message creat ... \n");
	while (1)
	{
		struct __User_data *pMqttMsg;

		printf("[%d] Task_ParseJSON_Message xQueueReceive wait ... \n", esp_get_free_heap_size());
		xQueueReceive(ParseJSONQueueHandler, &pMqttMsg, portMAX_DELAY);
		pMqttMsg->allData[pMqttMsg->dataLen] = '/0';
		printf("[SY] xQueueReceive  data [%s] \n", pMqttMsg->allData);

		////首先整体判断是否为一个json格式的数据
		cJSON *pJsonRoot = cJSON_Parse(pMqttMsg->allData);
		//如果是否json格式数据
		if (pJsonRoot == NULL)
		{
			printf("[SY] Task_ParseJSON_Message xQueueReceive not json ... \n");
			goto __cJSON_Delete;
		}

		cJSON *pJSON_Item = cJSON_GetObjectItem(pJsonRoot, "item");

		if (!pJSON_Item)
		{
			printf("[SY] Task_ParseJSON_Message pJSON_Header not json ... \n");
			goto __cJSON_Delete;
		}

		post_data_to_clouds(client, MqttTopicPub);

	__cJSON_Delete:
		cJSON_Delete(pJsonRoot);
	}
}

static void post_data_to_clouds(esp_mqtt_client_handle_t client, char *topic)
{

	if (!isConnect2Server)
		return;

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
	cJSON_AddStringToObject(pAttrPower, "value", "on");
	cJSON_AddItemToArray(pAttr, pAttrPower);

	char *s = cJSON_Print(pRoot);
	ESP_LOGI(TAG, "post_data_to_clouds topic end : %s", topic);
	ESP_LOGI(TAG, "post_data_to_clouds payload : %s", s);
	//esp_mqtt_client_publish(client, topic, s, strlen(s), 1, 0);
	cJSON_free((void *)s);
	cJSON_Delete(pRoot);
}

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

void TaskXMqttRecieve(void *p)
{
	//连接的配置参数
	esp_mqtt_client_config_t mqtt_cfg = {
		.host = "www.xuhong.com", //连接的域名 ，请务必修改为您的
		.port = 1883,			  //端口，请务必修改为您的
		.username = "admin",	  //用户名，请务必修改为您的
		.password = "xuhong123",  //密码，请务必修改为您的
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

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	switch (event->event_id)
	{
	case SYSTEM_EVENT_STA_START:
		xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 1024 * 2, NULL, 3, NULL);
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

void smartconfig_example_task(void *parm)
{
	EventBits_t uxBits;

	bool isAutoConnect = routerStartConnect();

	if (isAutoConnect)
	{
		vTaskDelete(NULL);
	}
	else
	{
		ESP_LOGI(TAG, "into smartconfig mode");
		ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS));
		ESP_ERROR_CHECK(esp_smartconfig_start(sc_callback));
	}

	while (1)
	{
		uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT | AIRKISS_DONE_BIT, true, false, portMAX_DELAY);

		if (uxBits & AIRKISS_DONE_BIT)
		{
			ESP_LOGI(TAG, "smartconfig over , start find device");
			esp_smartconfig_stop();

			if (handleLlocalFind == NULL)
				xTaskCreate(TaskCreatSocket, "TaskCreatSocket", 1024 * 2, NULL, 6, &handleLlocalFind);

			ESP_LOGI(TAG, "getAirkissVersion %s", airkiss_version());

			vTaskDelete(NULL);
		}

		if (uxBits & ESPTOUCH_DONE_BIT)
		{
			ESP_LOGI(TAG, "smartconfig over , but don't find device by airkiss...");
			esp_smartconfig_stop();
			vTaskDelete(NULL);
		}
	}
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
	printf("     system_get_flash_size_map(): %d \n", system_get_flash_size_map());
	//获取mac地址（station模式）
	uint8_t mac[6];
	esp_read_mac(mac, ESP_MAC_WIFI_STA);
	sprintf(deviceUUID, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	sprintf((char *)deviceInfo, "{\"device\":\"%s\",\"mac\":\"%s\"}", DEVICE_TYPE, deviceUUID);
	sprintf(MqttTopicSub, "/%s/%s/devSub", DEVICE_TYPE, deviceUUID);
	sprintf(MqttTopicPub, "/%s/%s/devPub", DEVICE_TYPE, deviceUUID);

	ESP_LOGI(TAG, "flagNet: %d", flagNet);
	ESP_LOGI(TAG, "FW_VERSION: %s", FW_VERSION);
	ESP_LOGI(TAG, "deviceUUID: %s", deviceUUID);
	ESP_LOGI(TAG, "deviceInfo: %s", deviceInfo);
	ESP_LOGI(TAG, "MqttTopicSub: %s", MqttTopicSub);
	ESP_LOGI(TAG, "MqttTopicPub: %s", MqttTopicPub);

	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
}