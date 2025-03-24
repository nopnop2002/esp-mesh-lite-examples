/*
	WebSocket Client Example

	This example code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.

	https://qiita.com/zakkied/items/b01945c7b0f78733ad26
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_mac.h" // MACSTR
#include "esp_wifi.h" // esp_wifi_get_mac

#include "esp_mesh_lite.h"
#include "esp_websocket_client.h"

static const char *TAG = "CLIENT";

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	switch (event_id) {
	case WEBSOCKET_EVENT_CONNECTED:
		//ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
		break;
	case WEBSOCKET_EVENT_DISCONNECTED:
		//ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
		break;
	case WEBSOCKET_EVENT_DATA:
		//ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
		//ESP_LOGI(TAG, "Received opcode=0x%x", data->op_code);
		break;
	case WEBSOCKET_EVENT_ERROR:
		//ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
		break;
	}
}

void ws_client(void *pvParameters)
{
	ESP_LOGI(TAG, "Start SERVER_IP:%s SERVER_PORT:%d", CONFIG_SERVER_IP, CONFIG_SERVER_PORT);

	// Set server url
	char url[142];
	sprintf(url, "ws://%s:%d", CONFIG_SERVER_IP, CONFIG_SERVER_PORT);
	ESP_LOGI(TAG, "url=[%s]", url);

	esp_websocket_client_config_t websocket_cfg = {};
	websocket_cfg.uri = url;
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0))
	websocket_cfg.reconnect_timeout_ms = 10000;
	websocket_cfg.network_timeout_ms = 10000;
	websocket_cfg.ping_interval_sec = 60;
#endif

	ESP_LOGI(TAG, "Connecting to %s...", websocket_cfg.uri);
	esp_websocket_client_handle_t client = esp_websocket_client_init(&websocket_cfg);
	esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);

	// Wait for server connection
	esp_websocket_client_start(client);
	while(1) {
		if (esp_websocket_client_is_connected(client)) break;
		ESP_LOGW(TAG, "waiting for server to start");
		vTaskDelay(1000);
	}
	ESP_LOGI(TAG, "Connected to %s...", websocket_cfg.uri);

	// Get station mac address
	uint8_t sta_mac[6] = {0};
	esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);

	char buffer[128];
	while (1) {
		TickType_t nowTick = xTaskGetTickCount();
		int buflen = sprintf(buffer, "(%"PRIu32") data from "MACSTR, nowTick, MAC2STR(sta_mac));
		ESP_LOGI(TAG, "buffer=[%s]",buffer);
		if (esp_websocket_client_is_connected(client)) {
			esp_websocket_client_send_text(client, buffer, buflen, portMAX_DELAY);
		} else {
			ESP_LOGW(TAG, "Not connected server");
		}
		vTaskDelay(1000);
	}

	// Close the WebSocket connection in a clean way.
	ESP_LOGI(TAG, "esp_websocket_client_close");
	esp_websocket_client_close(client, 0);

	// Stops the WebSocket connection without websocket closing handshake.
	ESP_LOGI(TAG, "esp_websocket_client_stop");
	esp_websocket_client_stop(client);

	ESP_LOGI(TAG, "Websocket client destory");
	esp_websocket_client_destroy(client);
	vTaskDelete(NULL);
}
