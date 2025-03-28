/*
	SNTP over MESH Example

	This example code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"

#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_sntp.h"

#include "esp_bridge.h"
#include "esp_mesh_lite.h"

static const char *TAG = "MAIN";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0

/**
 * @brief Timed printing system information
 */
static void print_system_info_timercb(TimerHandle_t timer)
{
	uint8_t primary					= 0;
	uint8_t sta_mac[6]				= {0};
	wifi_ap_record_t ap_info		= {0};
	wifi_second_chan_t second		= 0;
	wifi_sta_list_t wifi_sta_list	= {0x0};

	esp_wifi_sta_get_ap_info(&ap_info);
	esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
	esp_wifi_ap_get_sta_list(&wifi_sta_list);
	esp_wifi_get_channel(&primary, &second);

	ESP_LOGI(TAG, "System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
			 ", parent rssi: %d, free heap: %"PRIu32"", primary,
			 esp_mesh_lite_get_level(), MAC2STR(sta_mac), MAC2STR(ap_info.bssid),
			 (ap_info.rssi != 0 ? ap_info.rssi : -120), esp_get_free_heap_size());
#if CONFIG_MESH_LITE_NODE_INFO_REPORT
	ESP_LOGI(TAG, "All node number: %"PRIu32"", esp_mesh_lite_get_mesh_node_number());
#endif /* MESH_LITE_NODE_INFO_REPORT */
	for (int i = 0; i < wifi_sta_list.num; i++) {
		ESP_LOGI(TAG, "Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
	}
}

static void ip_event_sta_got_ip_handler(void *arg, esp_event_base_t event_base,
										int32_t event_id, void *event_data)
{
	static bool tcp_task = false;
	ESP_LOGI(TAG, "ip_event_sta_got_ip_handler");
	if (!tcp_task) {
		ESP_LOGI(TAG, "Connect to MESH");
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
		tcp_task = true;
	}
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

void time_sync_notification_cb(struct timeval *tv)
{
	ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void initialize_sntp(void)
{
	ESP_LOGI(TAG, "Initializing SNTP");
	esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
	//sntp_setservername(0, "pool.ntp.org");
	ESP_LOGI(TAG, "Your NTP Server is %s", CONFIG_NTP_SERVER);
	esp_sntp_setservername(0, CONFIG_NTP_SERVER);
	sntp_set_time_sync_notification_cb(time_sync_notification_cb);
	esp_sntp_init();
}

static esp_err_t obtain_time(void)
{
	initialize_sntp();
	// wait for time to be set
	int retry = 0;
	const int retry_count = 10;
	while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
		ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}

	if (retry == retry_count) return ESP_FAIL;
	return ESP_OK;
}

void app_main()
{
	/**
	 * @brief Set the log level for serial port printing.
	 */
	esp_log_level_set("*", ESP_LOG_INFO);

	ESP_ERROR_CHECK(esp_storage_init());

	s_wifi_event_group = xEventGroupCreate();
	xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	esp_bridge_create_all_netif();
	
	ESP_LOGI(TAG, "wifi_init");
	wifi_init();

	ESP_LOGI(TAG, "esp_mesh_lite_init");
	esp_mesh_lite_config_t mesh_lite_config = ESP_MESH_LITE_DEFAULT_INIT();
	esp_mesh_lite_init(&mesh_lite_config);

	ESP_LOGI(TAG, "app_wifi_set_softap_info");
	app_wifi_set_softap_info();

	ESP_LOGI(TAG, "esp_mesh_lite_start");
	esp_mesh_lite_start();

	/**
	 * @breif Create handler
	 */
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_sta_got_ip_handler, NULL, NULL));

	TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_PERIOD_MS,
									   true, NULL, print_system_info_timercb);
	xTimerStart(timer, 0);

	// Waiting until the connection is established (WIFI_CONNECTED_BIT) 
	xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

	// Obtain time over NTP
	ESP_ERROR_CHECK(obtain_time());

	// Obtain local current time
	time_t now;
	struct tm timeinfo;
	char strftime_buf[64];
	while (1) {
		time(&now);
		now = now + (CONFIG_LOCAL_TIMEZONE*60*60);
		localtime_r(&now, &timeinfo);
		strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
		ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);
		vTaskDelay(10000);
	}
}
