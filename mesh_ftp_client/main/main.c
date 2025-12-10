/*
	FTP over MESH Example

	This example code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_spiffs.h"

#include "esp_wifi.h"
#include "nvs_flash.h"
#include <sys/socket.h> // ip_struct/inet_ntoa

#include "esp_bridge.h"
#include "esp_mesh_lite.h"

char *MOUNT_POINT = "/root";
QueueHandle_t xQueueFTP;

static bool wifi_connected = false;
static bool ftp_client_task = false;
static char srcFileName[64];

static const char *TAG = "MAIN";

void ftp_client(void *pvParameters);

#define MAX_RETRY 5
#define RAW_MSG_ID_BROADCAST 1
#define RAW_MSG_ID_TO_SIBLING 2
#define RAW_MSG_ID_TO_ROOT 3
#define RAW_MSG_ID_TO_ROOT_RESP 4
#define RAW_MSG_ID_TO_PARENT 5
#define RAW_MSG_ID_TO_PARENT_RESP 6

/**
 * @brief Timed printing system information
 */
static void print_system_info_timercb(TimerHandle_t timer)
{
	uint8_t primary					= 0;
	uint8_t sta_mac[6]				= {0};
	uint8_t ap_mac[6]				= {0};
	wifi_ap_record_t ap_info		= {0};
	wifi_second_chan_t second		= 0;
	wifi_sta_list_t wifi_sta_list	= {0x0};

	esp_wifi_sta_get_ap_info(&ap_info);
	esp_wifi_get_mac(WIFI_IF_STA, sta_mac);
	esp_wifi_get_mac(WIFI_IF_AP, ap_mac);
	esp_wifi_ap_get_sta_list(&wifi_sta_list);
	esp_wifi_get_channel(&primary, &second);

	char buffer[256];
	int buffer_len = snprintf(buffer, sizeof(buffer), 
		"System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
		", parent rssi: %d, free heap: %"PRIu32"", primary,
		esp_mesh_lite_get_level(), MAC2STR(ap_mac), MAC2STR(ap_info.bssid),
		(ap_info.rssi != 0 ? ap_info.rssi : -120), esp_get_free_heap_size());
	ESP_LOGI(TAG, "%s", buffer);
#if 0
	ESP_LOGI(TAG, "System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
		", parent rssi: %d, free heap: %"PRIu32"", primary,
		esp_mesh_lite_get_level(), MAC2STR(ap_mac), MAC2STR(ap_info.bssid),
		(ap_info.rssi != 0 ? ap_info.rssi : -120), esp_get_free_heap_size());
#endif
	for (int i = 0; i < wifi_sta_list.num; i++) {
		ESP_LOGI(TAG, "Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
	}

#if 0
	ESP_LOGI(TAG, "All node number: %"PRIu32"", esp_mesh_lite_get_mesh_node_number());
	uint32_t size = 0;
	const node_info_list_t *node = esp_mesh_lite_get_nodes_list(&size);
	for (uint32_t loop = 0; (loop < size) && (node != NULL); loop++) {
		struct in_addr ip_struct;
		ip_struct.s_addr = node->node->ip_addr;
		printf("%ld: %d, "MACSTR", %s\r\n" , loop + 1, node->node->level, MAC2STR(node->node->mac_addr), inet_ntoa(ip_struct));
		node = node->next;
	}
#endif

	char mac_str[MAC_MAX_LEN];
	snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(sta_mac));
	int16_t layer = esp_mesh_lite_get_level();
	ESP_LOGD(TAG, "layer: %d, mac_str: [%s]", layer, mac_str);
	if (layer == 1) {
		sprintf(srcFileName, "%s/mesh-lite.txt", MOUNT_POINT);
		if (ftp_client_task == false) {
			ESP_LOGI(TAG, "Start ftp_client task");
			xTaskCreate(ftp_client, "ftp_client", 6 * 1024, (void *)srcFileName, 5, NULL);

			struct stat stat_buf;
			if (stat(srcFileName, &stat_buf) == 0) {
				unlink(srcFileName);
			}
			ftp_client_task = true;
		}

		// Append file
		FILE* fp = fopen(srcFileName, "a+");
		if (fp != NULL) {
			fprintf(fp, "%s\n", buffer);
			fclose(fp);
		} else {
			ESP_LOGE(TAG, "Failed to open file for writing");
		}
		xQueueSendFromISR(xQueueFTP, &layer, NULL);
	} else {
		esp_mesh_lite_msg_config_t config = {
			.raw_msg = {
				.msg_id = RAW_MSG_ID_TO_ROOT,
				.expect_resp_msg_id = RAW_MSG_ID_TO_ROOT_RESP,
				.max_retry = MAX_RETRY,
				.retry_interval = 1000,
				.data = (uint8_t *)buffer,
				.size = buffer_len,
				.raw_resend = &esp_mesh_lite_send_raw_msg_to_root,
				.raw_send_fail = NULL,
			}
		};
		esp_err_t err = esp_mesh_lite_send_msg(ESP_MESH_LITE_RAW_MSG, &config);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "esp_mesh_lite_send_msg fail");
		}
	}
}

static void ip_event_sta_got_ip_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	ESP_LOGI(TAG, "ip_event_sta_got_ip_handler");
	wifi_connected = true;
}

static esp_err_t esp_storage_init(void)
{
	esp_err_t ret = nvs_flash_init();

	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}

	return ret;
}

static void wifi_init(void)
{
	// Station
	wifi_config_t wifi_config = {
		.sta = {
			.ssid = CONFIG_ROUTER_SSID,
			.password = CONFIG_ROUTER_PASSWORD,
		},
	};
	esp_bridge_wifi_set_config(WIFI_IF_STA, &wifi_config);

	// Softap
	snprintf((char *)wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), "%s", CONFIG_BRIDGE_SOFTAP_SSID);
	strlcpy((char *)wifi_config.ap.password, CONFIG_BRIDGE_SOFTAP_PASSWORD, sizeof(wifi_config.ap.password));
	esp_bridge_wifi_set_config(WIFI_IF_AP, &wifi_config);
}

void app_wifi_set_softap_info(void)
{
	char softap_ssid[32];
	char softap_psw[64];
	uint8_t softap_mac[6];
	size_t size = sizeof(softap_psw);
	esp_wifi_get_mac(WIFI_IF_AP, softap_mac);
	memset(softap_ssid, 0x0, sizeof(softap_ssid));

#ifdef CONFIG_BRIDGE_SOFTAP_SSID_END_WITH_THE_MAC
	snprintf(softap_ssid, sizeof(softap_ssid), "%.25s_%02x%02x%02x", CONFIG_BRIDGE_SOFTAP_SSID, softap_mac[3], softap_mac[4], softap_mac[5]);
#else
	snprintf(softap_ssid, sizeof(softap_ssid), "%.32s", CONFIG_BRIDGE_SOFTAP_SSID);
#endif
	ESP_LOGI(TAG, "softap_ssid=[%s]", softap_ssid);
	if (esp_mesh_lite_get_softap_ssid_from_nvs(softap_ssid, &size) != ESP_OK) {
		esp_mesh_lite_set_softap_ssid_to_nvs(softap_ssid);
	}
	if (esp_mesh_lite_get_softap_psw_from_nvs(softap_psw, &size) != ESP_OK) {
		esp_mesh_lite_set_softap_psw_to_nvs(CONFIG_BRIDGE_SOFTAP_PASSWORD);
	}
	esp_mesh_lite_set_softap_info(softap_ssid, CONFIG_BRIDGE_SOFTAP_PASSWORD);
}

static esp_err_t raw_broadcast_handler(uint8_t *data, uint32_t len, uint8_t **out_data, uint32_t* out_len, uint32_t seq)
{
	return ESP_OK;
}

static esp_err_t raw_to_sibling_handler(uint8_t *data, uint32_t len, uint8_t **out_data, uint32_t* out_len, uint32_t seq)
{
	return ESP_OK;
}

static esp_err_t raw_to_root_handler(uint8_t *data, uint32_t len, uint8_t **out_data, uint32_t* out_len, uint32_t seq)
{
	ESP_LOGI(__FUNCTION__, "seq=%"PRIi32, seq);
	static uint32_t last_recv_seq = 0;
	// The same message will be received MAX_RETRY times, so if it is the same message, it will be discarded.
	if (last_recv_seq != seq) {
		printf("[recv from child] %.*s\n", (int)len, data);
		if (strncmp((char *)data, "System information", 18) == 0) {
			// Append file
			FILE* fp = fopen(srcFileName, "a+");
			if (fp != NULL) {
				fprintf(fp, "%.*s\n", (int)len, data);
				fclose(fp);
			} else {
				ESP_LOGE(TAG, "Failed to open file for writing");
			}
		}
		last_recv_seq = seq;
	}
	*out_len = 0;
	return ESP_OK;
}

static esp_err_t raw_to_root_resp_handler(uint8_t *data, uint32_t len, uint8_t **out_data, uint32_t* out_len, uint32_t seq)
{
	return ESP_OK;
}

static esp_err_t raw_to_parent_handler(uint8_t *data, uint32_t len, uint8_t **out_data, uint32_t* out_len, uint32_t seq)
{
	return ESP_OK;
}

static esp_err_t raw_to_parent_resp_handler(uint8_t *data, uint32_t len, uint8_t **out_data, uint32_t* out_len, uint32_t seq)
{
	return ESP_OK;
}

static const esp_mesh_lite_raw_msg_action_t raw_msgs_action[] = {
	/* Send RAW to the all node */
	{RAW_MSG_ID_BROADCAST, 0, raw_broadcast_handler},

	/* Send RAW to the sibling node */
	{RAW_MSG_ID_TO_SIBLING, 0, raw_to_sibling_handler},

	/* Send RAW to the root node */
	{RAW_MSG_ID_TO_ROOT, RAW_MSG_ID_TO_ROOT_RESP, raw_to_root_handler},
	{RAW_MSG_ID_TO_ROOT_RESP, 0, raw_to_root_resp_handler},

	/* Send RAW to the parent node */
	{RAW_MSG_ID_TO_PARENT, RAW_MSG_ID_TO_PARENT_RESP, raw_to_parent_handler},
	{RAW_MSG_ID_TO_PARENT_RESP, 0, raw_to_parent_resp_handler},

	{0, 0, NULL} /* Must be NULL terminated */
};

esp_err_t mountSPIFFS(char * partition_label, char * mount_point) {
	ESP_LOGI(TAG, "Initializing SPIFFS file system on Builtin SPI Flash Memory");

	esp_vfs_spiffs_conf_t conf = {
		.base_path = mount_point,
		.partition_label = partition_label,
		.max_files = 5,
		.format_if_mount_failed = true
	};

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
		return ret;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(partition_label, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
		return ret;
	}
	ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	ESP_LOGI(TAG, "Mount SPIFFS filesystem on %s", mount_point);
	return ret;
}

void app_main()
{
	/**
	 * @brief Set the log level for serial port printing.
	 */
	esp_log_level_set("*", ESP_LOG_INFO);

	ESP_ERROR_CHECK(esp_storage_init());

	// Initialize SPIFFS
	char *partition_label = "storage";
	ESP_ERROR_CHECK(mountSPIFFS(partition_label, MOUNT_POINT));

	// Create Queue
	xQueueFTP = xQueueCreate( 1, sizeof(int16_t) );
	configASSERT( xQueueFTP );

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	esp_bridge_create_all_netif();
	
	ESP_LOGI(TAG, "wifi_init");
	wifi_init();

	ESP_LOGI(TAG, "esp_mesh_lite_init");
	esp_mesh_lite_config_t mesh_lite_config = ESP_MESH_LITE_DEFAULT_INIT();
	esp_mesh_lite_init(&mesh_lite_config);

	// Register custom message reception and recovery logic
	esp_mesh_lite_raw_msg_action_list_register(raw_msgs_action);

	ESP_LOGI(TAG, "app_wifi_set_softap_info");
	app_wifi_set_softap_info();

	ESP_LOGI(TAG, "esp_mesh_lite_start");
	esp_mesh_lite_start();

	/**
	 * @breif Create handler
	 */
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_sta_got_ip_handler, NULL, NULL));

	ESP_LOGI(TAG, "Wait for wifi connection");
	while(1) {
		ESP_LOGD(TAG, "wifi_connected=%d", wifi_connected);
		vTaskDelay(100);
		if (wifi_connected == true) break;
	}
	ESP_LOGI(TAG, "Connected to wifi");

	TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_PERIOD_MS, true, NULL, print_system_info_timercb);
	xTimerStart(timer, 0);
}
