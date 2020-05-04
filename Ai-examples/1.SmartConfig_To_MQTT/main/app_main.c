#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "mqtt_client.h"

static const char *TAG = "MQTT_EXAMPLE";




/**
 *    基于 esp-idf esp8266芯片 rtos3.0 sdk 乐鑫没做微信近场发现的功能，于是动动手指做起来！
 *    这是微信airkiss配网以及近场发现的功能的demo示范，亲测可以配网成功以及近场发现！
 *    有任何技术问题邮箱： 870189248@qq.com
 *    @team: Ai-Thinker Open Team 安信可开源团队-半颗心脏
 *
 **/
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {
	esp_mqtt_client_handle_t client = event->client;
	int msg_id;
	// your_context_t *context = event->context;
	switch (event->event_id) {
	case MQTT_EVENT_CONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
		msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1,
				0);
		ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

		msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
		ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

		msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
		ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

		msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
		ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
		break;
	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
		break;

	case MQTT_EVENT_SUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
		msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0,
				0);
		ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_DATA:
		ESP_LOGI(TAG, "MQTT_EVENT_DATA");
		printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
		printf("DATA=%.*s\r\n", event->data_len, event->data);
		break;
	case MQTT_EVENT_ERROR:
		ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
		break;
	}
	return ESP_OK;
}

static void mqtt_app_start(void) {
	esp_mqtt_client_config_t mqtt_cfg = { .uri = CONFIG_BROKER_URL,
			.event_handle = mqtt_event_handler,
	// .user_context = (void *)your_context
			};

	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_start(client);
}

void app_main() {

	ESP_ERROR_CHECK(nvs_flash_init());

	ESP_LOGI(TAG, "[APP] Startup..");
	ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
	ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	ESP_ERROR_CHECK(example_connect());

	mqtt_app_start();
}
