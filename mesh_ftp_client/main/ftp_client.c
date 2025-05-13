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
//#include "esp_mac.h" // MACSTR
//#include "esp_wifi.h" // esp_wifi_get_mac

#include "esp_mesh_lite.h"
#include "FtpClient.h"

static const char *TAG = "FTP-CLIENT";

//extern char *MOUNT_POINT;
extern QueueHandle_t xQueueFTP;

void ftp_client(void *pvParameters)
{
	char *srcFileName = (char *)pvParameters;
	ESP_LOGI(TAG, "Start FTP_SERVER:%s FTP_PORT:%d FTP_USER:%s FTP_PASSWORD:%s",
		CONFIG_FTP_SERVER, CONFIG_FTP_PORT, CONFIG_FTP_USER, CONFIG_FTP_PASSWORD);
	ESP_LOGI(TAG, "srcFileName=[%s]", srcFileName);

	int16_t layer;
	//char srcFileName[64];
	char dstFileName[64];
	//sprintf(srcFileName, "%s/mesh-lite.txt", MOUNT_POINT);
	sprintf(dstFileName, "mesh-lite.txt");
	while(1) {
		if(xQueueReceive(xQueueFTP, &layer, portMAX_DELAY)) {
			ESP_LOGI(TAG, "layer=%d", layer);
			if (layer != 1) continue;

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

			// Put file to FTP server
			ftpClient->ftpClientPut(srcFileName, dstFileName, FTP_CLIENT_TEXT, ftpClientNetBuf);
			ESP_LOGI(TAG, "ftpClientPut %s ---> %s", srcFileName, dstFileName);

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
