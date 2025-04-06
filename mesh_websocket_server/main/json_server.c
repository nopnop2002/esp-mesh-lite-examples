/*	JSON message server
	Stores and serves up the latest JSON data.

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
#include "freertos/message_buffer.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "JSON_SERVER";

extern MessageBufferHandle_t xMessageBufferJsonServer;
extern MessageBufferHandle_t xMessageBufferToClient;

#define BUF_SIZE 1024
#define JSON_SIZE 512

void json_server(void *pvParameters)
{
	ESP_LOGI(TAG, "Start");
	bool active_client = false;
	bool valid_json = false;
	char buffer[BUF_SIZE+1];
	char json_string[JSON_SIZE+1] = {0};
	while(1) {
		size_t received = xMessageBufferReceive(xMessageBufferJsonServer, buffer, BUF_SIZE, portMAX_DELAY);
		ESP_LOGI(TAG, "xMessageBufferReceive received=%d", received);
		ESP_LOGI(TAG, "buffer=[%.*s]", received, buffer);
		if (received == BUF_SIZE) {
			ESP_LOGE(TAG, "BUF_SIZE [%d] may be small", BUF_SIZE);
		}

		cJSON *root = cJSON_Parse(buffer);
		cJSON *found = NULL;

		found = cJSON_GetObjectItem(root, "id");
		char *id = found->valuestring;
		ESP_LOGI(TAG, "id=[%s]", id);

		// Save current json message
		if (strcmp(id, "save") == 0) {
			valid_json = true;
			found = cJSON_GetObjectItem(root, "json");
			char *json = found->valuestring;
			if (strlen(json) > JSON_SIZE) {
				ESP_LOGE(TAG, "JSON_SIZE [%d] may be small", JSON_SIZE);
				break;
			}
			strcpy(json_string, json);
			if (active_client) {

				cJSON *request;
				request = cJSON_CreateObject();
				cJSON_AddStringToObject(request, "id", "json-request");
				cJSON_AddStringToObject(request, "json", json_string);
				char *my_json_string = cJSON_Print(request);
				ESP_LOGI(TAG, "my_json_string\n%s",my_json_string);

				size_t xBytesSent = xMessageBufferSend(xMessageBufferToClient, my_json_string, strlen(my_json_string), portMAX_DELAY);
				if (xBytesSent != strlen(my_json_string)) {
					ESP_LOGE(TAG, "xMessageBufferSend fail. xBytesSent=%d strlen(my_json_string)=%d", xBytesSent, strlen(my_json_string));
					break;
				}
				cJSON_Delete(request);
				cJSON_free(my_json_string);
				
			}
		}

		// Post current json message
		if (strcmp(id, "load") == 0) {
			active_client = true;
			if (valid_json) {

				cJSON *request;
				request = cJSON_CreateObject();
				cJSON_AddStringToObject(request, "id", "json-request");
				cJSON_AddStringToObject(request, "json", json_string);
				char *my_json_string = cJSON_Print(request);
				ESP_LOGI(TAG, "my_json_string\n%s",my_json_string);

				size_t xBytesSent = xMessageBufferSend(xMessageBufferToClient, my_json_string, strlen(my_json_string), portMAX_DELAY);
				if (xBytesSent != strlen(my_json_string)) {
					ESP_LOGE(TAG, "xMessageBufferSend fail. xBytesSent=%d strlen(my_json_string)=%d", xBytesSent, strlen(my_json_string));
					break;
				}
				cJSON_Delete(request);
				cJSON_free(my_json_string);
			}
		}
		cJSON_Delete(root);

	}

	// Some error occurred
	ESP_LOGE(TAG, "finish");
	vTaskDelete(NULL);
}
