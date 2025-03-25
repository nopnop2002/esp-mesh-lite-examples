/*
	FTP Client Example

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
#include "esp_log.h"
#include "esp_mac.h" // MACSTR
#include "esp_wifi.h" // esp_wifi_get_mac

#include "esp_mesh_lite.h"
#include "FtpClient.h"

static const char *TAG = "CLIENT";

extern char *MOUNT_POINT;
extern QueueHandle_t xQueue;

void ftp_client(void *pvParameters)
{
	ESP_LOGI(TAG, "Start FTP_SERVER:%s FTP_PORT:%d FTP_USER:%s FTP_PASSWORD:%s",
		CONFIG_FTP_SERVER, CONFIG_FTP_PORT, CONFIG_FTP_USER, CONFIG_FTP_PASSWORD);

	// Get station mac address
	uint8_t sta_mac[6] = {0};
	esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);

	int16_t layer;
	char srcFileName[64];
	char dstFileName[64];
	sprintf(srcFileName, "%s/mesh-lite.txt", MOUNT_POINT);
	sprintf(dstFileName, "mesh-lite.txt");
	while(1) {
		if(xQueueReceive(xQueue, &layer, portMAX_DELAY)) {
			ESP_LOGI(TAG, "layer=%d", layer);
			if (layer == 0) continue;

			// Open FTP server
			static NetBuf_t* ftpClientNetBuf = NULL;
			FtpClient* ftpClient = getFtpClient();
			int connect = ftpClient->ftpClientConnect(CONFIG_FTP_SERVER, CONFIG_FTP_PORT, &ftpClientNetBuf);
			ESP_LOGD(TAG, "connect=%d", connect);
			if (connect == 0) {
				ESP_LOGE(TAG, "FTP server connect fail");
				break;
			}

			// Login FTP server
			int login = ftpClient->ftpClientLogin(CONFIG_FTP_USER, CONFIG_FTP_PASSWORD, ftpClientNetBuf);
			ESP_LOGD(TAG, "login=%d", login);
			if (login == 0) {
				ESP_LOGE(TAG, "FTP server login fail");
				break;
			}

			if (layer == 1) {
				wifi_ap_record_t ap_info;
				wifi_sta_list_t wifi_sta_list;
				uint8_t primary;
				wifi_second_chan_t second;
				esp_wifi_sta_get_ap_info(&ap_info);
				esp_wifi_ap_get_sta_list(&wifi_sta_list);
				esp_wifi_get_channel(&primary, &second);

				ESP_LOGI(TAG, "System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
					", parent rssi: %d, free heap: %"PRIu32"", primary,
					esp_mesh_lite_get_level(), MAC2STR(sta_mac), MAC2STR(ap_info.bssid),
					(ap_info.rssi != 0 ? ap_info.rssi : -120), esp_get_free_heap_size());

				// Create file
				FILE* fp = fopen(srcFileName, "w");
				if (fp == NULL) {
					ESP_LOGE(TAG, "Failed to open file for writing");
					break;
				}

				fprintf(fp, "System information\n");
				fprintf(fp, "channel: %d\n", primary);
				fprintf(fp, "layer: %d\n", esp_mesh_lite_get_level());
				fprintf(fp, "self mac: " MACSTR "\n", MAC2STR(sta_mac));
				fprintf(fp, "parent bssid: " MACSTR "\n", MAC2STR(ap_info.bssid));
				fprintf(fp, "parent rssi: %d", (ap_info.rssi != 0 ? ap_info.rssi : -120));
				fprintf(fp, "free heap: %" PRIu32 "\n", esp_get_free_heap_size());

				for (int i = 0; i < wifi_sta_list.num; i++) {
					fprintf(fp, "Child mac: " MACSTR "\n", MAC2STR(wifi_sta_list.sta[i].mac));
				}

				fclose(fp);
				
				// Put file to FTP server
				ftpClient->ftpClientPut(srcFileName, dstFileName, FTP_CLIENT_TEXT, ftpClientNetBuf);
				ESP_LOGI(TAG, "ftpClientPut %s ---> %s", srcFileName, dstFileName);

			} else {
				// Get file from FTP server
				ftpClient->ftpClientGet(srcFileName, dstFileName, FTP_CLIENT_TEXT, ftpClientNetBuf);
				ESP_LOGI(TAG, "ftpClientGet %s <--- %s", srcFileName, dstFileName);

				// Open file for reading
				FILE* fp = fopen(srcFileName, "r");
				if (fp == NULL) {
					ESP_LOGW(TAG, "Failed to open file for reading");
				} else {
					char str[128];
					ESP_LOGI(TAG, "-----------------------------------------------");
					while (fgets(str, sizeof(str), fp) != NULL) {
						if(*str && str[strlen(str) - 1] == '\n') {
							str[strlen(str) - 1] = 0;
						}
						ESP_LOGI(TAG, "%s", str);
					}
					ESP_LOGI(TAG, "-----------------------------------------------");
					fclose(fp);
				}
			}

			// Close FTP server
			ftpClient->ftpClientQuit(ftpClientNetBuf);
		} else {
			ESP_LOGE(TAG, "xQueueReceive fail");
			break;
		}
	}

	// Stop connection
	vTaskDelete(NULL);
}
