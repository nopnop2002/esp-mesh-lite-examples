/*	
	HTTP Client Example

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
#include "esp_log.h"
#include "esp_mac.h" // MACSTR
#include "esp_wifi.h" // esp_wifi_get_mac

#include "esp_mesh_lite.h"
#include "esp_http_client.h"
#include "esp_chip_info.h"
#include "cJSON.h"

static const char *TAG = "CLIENT";

esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
	switch(evt->event_id) {
		case HTTP_EVENT_ERROR:
			ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
			break;
		case HTTP_EVENT_ON_CONNECTED:
			ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
			break;
		case HTTP_EVENT_HEADER_SENT:
			ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
			break;
		case HTTP_EVENT_ON_HEADER:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
			break;
		case HTTP_EVENT_ON_DATA:
			ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
			break;
		case HTTP_EVENT_ON_FINISH:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
			break;
		case HTTP_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
			break;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
		case HTTP_EVENT_REDIRECT:
			ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
			esp_http_client_set_header(evt->client, "From", "user@example.com");
			esp_http_client_set_header(evt->client, "Accept", "text/html");
			esp_http_client_set_redirection(evt->client);
			break;
#endif
	}
	return ESP_OK;
}


esp_err_t http_post_with_url(char *url, char * post_data, size_t post_len)
{
	ESP_LOGI(TAG, "http_post_with_url url=[%s]", url);

	esp_http_client_config_t config = {
		.url = url,
		.path = "/post",
		.event_handler = http_event_handler,
		.user_data = NULL,
		.disable_auto_redirect = true,
	};

	esp_http_client_handle_t client = esp_http_client_init(&config);

	// POST
	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_header(client, "Content-Type", "application/json");
	//esp_http_client_set_post_field(client, post_data, strlen(post_data));
	esp_http_client_set_post_field(client, post_data, post_len);
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
			esp_http_client_get_status_code(client),
			(int)esp_http_client_get_content_length(client));
	} else {
		ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
	}

	esp_http_client_cleanup(client);
	return err;
}

void http_client(void *pvParameters)
{
	ESP_LOGI(TAG, "Start SERVER_IP:%s SERVER_PORT:%d", CONFIG_SERVER_IP, CONFIG_SERVER_PORT);

	// Set server url
	char url[142];
	sprintf(url, "http://%s:%d", CONFIG_SERVER_IP, CONFIG_SERVER_PORT);
	ESP_LOGI(TAG, "url=[%s]", url);

	// Get station mac address
	uint8_t sta_mac[6] = {0};
	esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
	char mac_str[MAC_MAX_LEN];
	snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(sta_mac));

	while (1) {
		TickType_t nowTick = xTaskGetTickCount();
		cJSON *root;
		root = cJSON_CreateObject();
		esp_chip_info_t chip_info;
		esp_chip_info(&chip_info);
		cJSON_AddNumberToObject(root, "level", esp_mesh_lite_get_level());
		cJSON_AddStringToObject(root, "mac", mac_str);
		cJSON_AddNumberToObject(root, "now", nowTick);
		cJSON_AddNumberToObject(root, "cores", chip_info.cores);
		cJSON_AddStringToObject(root, "target", CONFIG_IDF_TARGET);
		//char *json_string = cJSON_Print(root);
		char *json_string = cJSON_PrintUnformatted(root);
		int json_length = strlen(json_string);
		ESP_LOGI(TAG, "json_string\n%s",json_string);
		if (http_post_with_url(url, json_string, json_length) != ESP_OK) {
			ESP_LOGE(TAG, "http_post_with_url fail");
		}
		cJSON_Delete(root);
		cJSON_free(json_string);
		vTaskDelay(1000);
	}

	// Stop connection
	vTaskDelete(NULL);
}
