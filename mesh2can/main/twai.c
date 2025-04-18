/*	TWAI Network Example

	This example code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/message_buffer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h" // Update from V4.2
#include "cJSON.h"

extern MessageBufferHandle_t xMessageBufferTx;
extern MessageBufferHandle_t xMessageBufferRx;

#define BUF_SIZE 128

void twai_tx(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL),"task start");

	twai_message_t tx_msg;
	twai_status_info_t status_info;
	tx_msg.extd = 0;
	tx_msg.ss = 1;
	tx_msg.self = 0;
	tx_msg.dlc_non_comp = 0;
	tx_msg.identifier = 0;

	char tx_buffer[BUF_SIZE+1];
	while (1) {
		size_t received = xMessageBufferReceive(xMessageBufferTx, tx_buffer, BUF_SIZE, portMAX_DELAY);
		ESP_LOGI(pcTaskGetName(NULL), "xMessageBufferReceive received=%d", received);
		if (received > 0) {
			tx_buffer[received] = 0;
			ESP_LOGI(pcTaskGetName(NULL), "xMessageBufferReceive tx_buffer=[%s]",tx_buffer);
			cJSON *root = cJSON_Parse(tx_buffer);
			cJSON *found = NULL;
			found = cJSON_GetObjectItem(root, "level");
			uint8_t level = found->valueint;
			found = cJSON_GetObjectItem(root, "number");
			uint32_t number = found->valueint;
			found = cJSON_GetObjectItem(root, "frame");
			char *frame = found->valuestring;
			found = cJSON_GetObjectItem(root, "canid");
			uint32_t canid = found->valueint;
			found = cJSON_GetObjectItem(root, "payload");
			char *payload = found->valuestring;
			ESP_LOGI(pcTaskGetName(NULL), "level=%d number=%"PRIu32" frame=[%s] canid=%"PRIx32" payload=[%s]",
				level, number, frame, canid, payload);
			if (strcmp(frame, "std") == 0) {
				tx_msg.extd = 0;
			} else {
				tx_msg.extd = 1;
			}
			tx_msg.identifier = canid;
			tx_msg.data[0] = level;
			tx_msg.data[1] = number;
			char temp[3];
			for (int i=0;i<6;i++) {
				temp[0] = payload[i * 3 + 0];
				temp[1] = payload[i * 3 + 1];
				temp[2] = 0x00;
				tx_msg.data[i+2] = strtol(temp, NULL, 16);;
				ESP_LOGI(pcTaskGetName(NULL), "tx_msg.data[%d]=0x%x", i, tx_msg.data[i]);

			}
			cJSON_Delete(root);
			tx_msg.data_length_code = 8;
#if 0
			for (int i=0;i<tx_msg.data_length_code;i++) {
				tx_msg.data[i] = i;
			}
#endif

			twai_get_status_info(&status_info);
			if (status_info.state != TWAI_STATE_RUNNING) {
				ESP_LOGE(pcTaskGetName(NULL), "TWAI driver not running %d", status_info.state);
				continue;
			}
	
			esp_err_t ret = twai_transmit(&tx_msg, 0);
			if (ret == ESP_OK) {
				ESP_LOGI(pcTaskGetName(NULL), "twai_transmit success");
			} else {
				ESP_LOGE(pcTaskGetName(NULL), "twai_transmit Fail %s", esp_err_to_name(ret));
			}
			vTaskDelay(100);
		}
	}

	// Never reach here
	vTaskDelete(NULL);
}

void twai_rx(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL),"task start");

	twai_message_t rx_msg;
	while (1) {
		twai_receive(&rx_msg, portMAX_DELAY);
		ESP_LOGD(pcTaskGetName(NULL),"twai_receive identifier=0x%"PRIx32" flags=0x%"PRIx32" extd=0x%x rtr=0x%x data_length_code=%d",
			rx_msg.identifier, rx_msg.flags, rx_msg.extd, rx_msg.rtr, rx_msg.data_length_code);

		//int ext = rx_msg.flags & 0x01; // flags is Deprecated
		int ext = rx_msg.extd;
		//int rtr = rx_msg.flags & 0x02; // flags is Deprecated
		int rtr = rx_msg.rtr;

#if 0
		if (ext == 0) {
			printf("Standard ID: 0x%03"PRIx32"	   ", rx_msg.identifier);
		} else {
			printf("Extended ID: 0x%08"PRIx32, rx_msg.identifier);
		}
		printf("  DLC: %d  Data: ", rx_msg.data_length_code);

		if (rtr == 0) {
			for (int i = 0; i < rx_msg.data_length_code; i++) {
				printf("0x%02x ", rx_msg.data[i]);
			}
		} else {
			printf("REMOTE REQUEST FRAME");

		}
		printf("\n");
#endif

		cJSON *root;
		root = cJSON_CreateObject();
		cJSON_AddNumberToObject(root, "ext", ext);
		cJSON_AddNumberToObject(root, "rtr", rtr);
		cJSON_AddNumberToObject(root, "id", rx_msg.identifier);
		cJSON_AddNumberToObject(root, "length", rx_msg.data_length_code);
		cJSON *array;
		array = cJSON_AddArrayToObject(root, "data");
		cJSON *element[6];
		for (int i = 0; i < rx_msg.data_length_code; i++) {
			element[i] = cJSON_CreateNumber(rx_msg.data[i]);
			cJSON_AddItemToArray(array, element[i]);
		}
		//char *my_json_string = cJSON_Print(root);
		char *my_json_string = cJSON_PrintUnformatted(root);
		ESP_LOGD(pcTaskGetName(NULL), "my_json_string\n%s",my_json_string);
		int json_length = strlen(my_json_string);
		size_t sended = xMessageBufferSend(xMessageBufferRx, my_json_string, json_length, 100);
		if (sended != json_length) {
			ESP_LOGE(pcTaskGetName(NULL), "xMessageBufferSendFromISR fail json_length=%d sended=%d", json_length, sended);
			break;
		}
		cJSON_free(my_json_string);
		cJSON_Delete(root);
	} // end while

	// Never reach here
	vTaskDelete(NULL);
}
