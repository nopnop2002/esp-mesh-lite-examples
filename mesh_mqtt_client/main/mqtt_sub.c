/*	
	MQTT subscribe Example

	This example code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h" // esp_wifi_get_mac

#include "esp_mesh_lite.h"
#include "mqtt_client.h"
#include "mqtt.h"

static const char *TAG = "SUB";

extern const uint8_t root_cert_pem_start[] asm("_binary_root_cert_pem_start");
extern const uint8_t root_cert_pem_end[] asm("_binary_root_cert_pem_end");

static void log_error_if_nonzero(const char *message, int error_code)
{
	if (error_code != 0) {
		ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
	}
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	esp_mqtt_event_handle_t event = event_data;
	MQTT_t *mqttBuf = handler_args;
	ESP_LOGI(TAG, "taskHandle=0x%x", (unsigned int)mqttBuf->taskHandle);
	mqttBuf->event_id = event->event_id;
	switch (event->event_id) {
	case MQTT_EVENT_CONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
		xTaskNotifyGive( mqttBuf->taskHandle );
		break;
	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
		xTaskNotifyGive( mqttBuf->taskHandle );
		break;
	case MQTT_EVENT_SUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_DATA:
		ESP_LOGI(TAG, "MQTT_EVENT_DATA");
		ESP_LOGI(TAG, "TOPIC=[%.*s] DATA=[%.*s]\r", event->topic_len, event->topic, event->data_len, event->data);

		mqttBuf->topic_len = event->topic_len;
		for(int i=0;i<event->topic_len;i++) {
			mqttBuf->topic[i] = event->topic[i];
			mqttBuf->topic[i+1] = 0;
		}
		mqttBuf->data_len = event->data_len;
		for(int i=0;i<event->data_len;i++) {
			mqttBuf->data[i] = event->data[i];
			mqttBuf->data[i+1] = 0;
		}
		xTaskNotifyGive( mqttBuf->taskHandle );
		break;
	case MQTT_EVENT_ERROR:
		ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
		if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
			log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
			log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
			log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
			ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

		}
		xTaskNotifyGive( mqttBuf->taskHandle );
		break;
	default:
		ESP_LOGI(TAG, "Other event id:%d", event->event_id);
		break;
	}
}

esp_err_t query_mdns_host(const char * host_name, char *ip);
void convert_mdns_host(char * from, char * to);

void mqtt_sub(void *pvParameters)
{
	ESP_LOGI(TAG, "Start CONFIG_MQTT_BROKER=[%s]", CONFIG_MQTT_BROKER);

	// Set client id from mac
	uint8_t mac[8];
	ESP_ERROR_CHECK(esp_base_mac_addr_get(mac));
	for(int i=0;i<8;i++) {
		ESP_LOGI(TAG, "mac[%d]=%x", i, mac[i]);
	}
	char client_id[64];
	sprintf(client_id, "esp32-%02x%02x%02x%02x%02x%02x", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	ESP_LOGI(TAG, "client_id=[%s]", client_id);

	// Resolve mDNS host name
	char ip[128];
	char uri[138];
	ESP_LOGI(TAG, "CONFIG_MQTT_BROKER=[%s]", CONFIG_MQTT_BROKER);
	convert_mdns_host(CONFIG_MQTT_BROKER, ip);
	ESP_LOGI(TAG, "ip=[%s]", ip);
#if CONFIG_MQTT_TRANSPORT_OVER_TCP
	ESP_LOGI(TAG, "MQTT_TRANSPORT_OVER_TCP");
	sprintf(uri, "mqtt://%.60s:%d", ip, CONFIG_MQTT_PORT_TCP);
#elif CONFIG_MQTT_TRANSPORT_OVER_SSL
	ESP_LOGI(TAG, "MQTT_TRANSPORT_OVER_SSL");
	sprintf(uri, "mqtts://%.60s:%d", ip, CONFIG_MQTT_PORT_SSL);
#elif CONFIG_MQTT_TRANSPORT_OVER_WS
	ESP_LOGI(TAG, "MQTT_TRANSPORT_OVER_WS");
	sprintf(uri, "ws://%.60s:%d/mqtt", ip, CONFIG_MQTT_PORT_WS);
#elif CONFIG_MQTT_TRANSPORT_OVER_WSS
	ESP_LOGI(TAG, "MQTT_TRANSPORT_OVER_WSS");
	sprintf(uri, "wss://%.60s:%d/mqtt", ip, CONFIG_MQTT_PORT_WSS);
#endif
	ESP_LOGI(TAG, "uri=[%s]", uri);

	// Initialize MQTT configuration structure
	esp_mqtt_client_config_t mqtt_cfg = {
		.broker.address.uri = uri,
#if CONFIG_MQTT_TRANSPORT_OVER_TCP
#elif CONFIG_MQTT_TRANSPORT_OVER_SSL
		.broker.verification.certificate = (const char *)root_cert_pem_start,
#elif CONFIG_MQTT_TRANSPORT_OVER_WS
#elif CONFIG_MQTT_TRANSPORT_OVER_WSS
		.broker.verification.certificate = (const char *)root_cert_pem_start,
#endif
#if CONFIG_BROKER_AUTHENTICATION
		.credentials.username = CONFIG_AUTHENTICATION_USERNAME,
		.credentials.authentication.password = CONFIG_AUTHENTICATION_PASSWORD,
#endif
		.credentials.client_id = client_id
	};

	// Select protocol
#if CONFIG_MQTT_PROTOCOL_V_3_1_1
	ESP_LOGI(TAG, "MQTT_PROTOCOL_V_3_1_1");
	mqtt_cfg.session.protocol_ver = MQTT_PROTOCOL_V_3_1_1;
#elif CONFIG_MQTT_PROTOCOL_V_5
	ESP_LOGI(TAG, "MQTT_PROTOCOL_V_5");
	mqtt_cfg.session.protocol_ver = MQTT_PROTOCOL_V_5;
#endif

	// Initialize user context
	MQTT_t mqttBuf;
	mqttBuf.taskHandle = xTaskGetCurrentTaskHandle();
	ESP_LOGI(TAG, "taskHandle=0x%x", (unsigned int)mqttBuf.taskHandle);

	// Start mqtt client
	esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, &mqttBuf);
	esp_err_t ret = esp_mqtt_client_start(mqtt_client);
	ESP_LOGI(TAG, "esp_mqtt_client_start ret=%d", ret);

	// Get station mac address
	uint8_t sta_mac[6] = {0};
	esp_wifi_get_mac(WIFI_IF_STA, sta_mac);

	while (1) {
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		ESP_LOGI(TAG, "ulTaskNotifyTake event_id=%"PRIi32, mqttBuf.event_id);

		if (mqttBuf.event_id == MQTT_EVENT_CONNECTED) {
			char topic[64];
			strcpy(topic, "/topic/mesh/broadcast");
			esp_mqtt_client_subscribe(mqtt_client, topic, 0);
			sprintf(topic, "/topic/mesh/%02x%02x%02x%02x%02x%02x", 
				sta_mac[0], sta_mac[1], sta_mac[2], sta_mac[3], sta_mac[4], sta_mac[5]);
			ESP_LOGI(TAG, "topic=[%s]", topic);
			esp_mqtt_client_subscribe(mqtt_client, topic, 0);
		} else if (mqttBuf.event_id == MQTT_EVENT_DISCONNECTED) {
			break;
		} else if (mqttBuf.event_id == MQTT_EVENT_DATA) {
			ESP_LOGI(TAG, "-------------------------------------------------");
			ESP_LOGI(TAG, "TOPIC=[%.*s]\r", mqttBuf.topic_len, mqttBuf.topic);
			ESP_LOGI(TAG, "DATA=[%.*s]\r", mqttBuf.data_len, mqttBuf.data);
			ESP_LOGI(TAG, "-------------------------------------------------");
		} else if (mqttBuf.event_id == MQTT_EVENT_ERROR) {
			//break;
		}
	} // end while

	ESP_LOGI(TAG, "Task Delete");
	esp_mqtt_client_stop(mqtt_client);
	vTaskDelete(NULL);

}
