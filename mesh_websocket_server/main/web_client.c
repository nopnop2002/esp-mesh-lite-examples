/*
	Example using WEB Socket.
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
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/message_buffer.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "web_client";

#include "websocket_server.h"

extern MessageBufferHandle_t xMessageBufferToClient;
extern MessageBufferHandle_t xMessageBufferJsonServer;

// Timer callback
static void timer_cb(TimerHandle_t xTimer)
{
	TickType_t nowTick;
	nowTick = xTaskGetTickCount();
	ESP_LOGD(TAG, "timer is called, now=%"PRIu32, nowTick);

	cJSON *request;
	request = cJSON_CreateObject();
	cJSON_AddStringToObject(request, "id", "timer-request");
	//char *my_json_string = cJSON_Print(request);
	char *my_json_string = cJSON_PrintUnformatted(request);
	ESP_LOGD(TAG, "my_json_string\n%s",my_json_string);
	size_t xBytesSent = xMessageBufferSendFromISR(xMessageBufferToClient, my_json_string, strlen(my_json_string), NULL);
	if (xBytesSent != strlen(my_json_string)) {
		ESP_LOGE(TAG, "xMessageBufferSend fail");
	}
	cJSON_Delete(request);
	cJSON_free(my_json_string);
}

#define BUF_SIZE 1024

void client_task(void* pvParameters) {
	TaskHandle_t NotifyTaskHandle = (TaskHandle_t)pvParameters;
	ESP_LOGI(TAG, "Start NotifyTaskHandle=%"PRIx32, (uint32_t)NotifyTaskHandle);

	// Create Timer (Trigger a measurement every second)
	TimerHandle_t timerHandle = xTimerCreate("MY Trigger", 500, pdTRUE, NULL, timer_cb);
	if (timerHandle != NULL) {
		if (xTimerStart(timerHandle, 0) != pdPASS) {
			ESP_LOGE(TAG, "Unable to start Timer");
			vTaskDelete(NULL);
		} else {
			ESP_LOGI(TAG, "Success to start Timer");
		}
	} else {
		ESP_LOGE(TAG, "Unable to create Timer");
		vTaskDelete(NULL);
	}

	char inBuffer[BUF_SIZE+1];
	char DEL = 0x04;
	char outBuffer[BUF_SIZE+1];

	while (1) {
		size_t readBytes = xMessageBufferReceive(xMessageBufferToClient, inBuffer, sizeof(inBuffer), portMAX_DELAY );
		ESP_LOGI(TAG, "readBytes=%d", readBytes);
		ESP_LOGI(TAG, "inBuffer=[%.*s]", readBytes, inBuffer);
		cJSON *root = cJSON_Parse(inBuffer);
		if (cJSON_GetObjectItem(root, "id")) {
			char *id = cJSON_GetObjectItem(root,"id")->valuestring;
			ESP_LOGI(TAG, "id=[%s]",id);

			if ( strcmp (id, "init") == 0) {
				cJSON *request;
				request = cJSON_CreateObject();
				cJSON_AddStringToObject(request, "id", "load");
				char *my_json_string = cJSON_Print(request);
				ESP_LOGD(TAG, "my_json_string\n%s",my_json_string);

				// Load current json string
				size_t xBytesSent = xMessageBufferSend(xMessageBufferJsonServer, my_json_string, strlen(my_json_string), portMAX_DELAY);
				if (xBytesSent != strlen(my_json_string)) {
					ESP_LOGE(TAG, "xMessageBufferSend fail. xBytesSent=%d strlen(my_json_string)=%d", xBytesSent, strlen(my_json_string));
					break;
				}
				cJSON_Delete(request);
				cJSON_free(my_json_string);
			} // end init

			if ( strcmp (id, "json-request") == 0) {
				char *json = cJSON_GetObjectItem(root,"json")->valuestring;
				ESP_LOGI(TAG, "json=[%s]",json);
				sprintf(outBuffer,"TREE%c%s",DEL, json);
				ESP_LOGD(TAG, "outBuffer=[%s]", outBuffer);
				ws_server_send_text_all(outBuffer,strlen(outBuffer));
			} // end json-request

			if ( strcmp (id, "timer-request") == 0) {
				TickType_t nowTick = xTaskGetTickCount();
				char wk[64];
				cJSON *root2 = cJSON_CreateObject();
				sprintf(wk, "%"PRIi32, nowTick);
				cJSON_AddStringToObject(root2, "elasped", wk);
				sprintf(wk, "%"PRIu32, esp_get_free_heap_size());
				cJSON_AddStringToObject(root2, "freeHeep", wk);
				//char *json = cJSON_Print(root2);
				char *json = cJSON_PrintUnformatted(root2);
				ESP_LOGI(TAG, "json=[%s]",json);
				sprintf(outBuffer,"ELASPED%c%s",DEL, json);
				ESP_LOGD(TAG, "outBuffer=[%s]", outBuffer);
				ws_server_send_text_all(outBuffer,strlen(outBuffer));
				cJSON_Delete(root2);
				cJSON_free(json);
			} // end timer-request

		} // end if

		// Delete a cJSON structure
		cJSON_Delete(root);

	} // end while

	// Never reach here
	vTaskDelete(NULL);
}
