/*
	UART/USB to MESH bridge Example

	This example code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/message_buffer.h"
#include "freertos/timers.h"

#include "esp_wifi.h"
#include "nvs_flash.h"
#include <sys/socket.h>

#include "esp_mac.h"
#include "esp_bridge.h"
#include "esp_mesh_lite.h"

static const char *TAG = "MAIN";

MessageBufferHandle_t xMessageBufferTx;
MessageBufferHandle_t xMessageBufferRx;

// The total number of bytes (not messages) the message buffer will be able to hold at any one time.
size_t xBufferSizeBytes = 1024;

#define MAX_RETRY  5

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

	if (esp_mesh_lite_get_level() > 1) {
		esp_wifi_sta_get_ap_info(&ap_info);
	}
	esp_wifi_get_mac(WIFI_IF_STA, sta_mac);
	esp_wifi_ap_get_sta_list(&wifi_sta_list);
	esp_wifi_get_channel(&primary, &second);

	ESP_LOGI(TAG, "System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
		", parent rssi: %d, free heap: %"PRIu32"", primary,
		esp_mesh_lite_get_level(), MAC2STR(sta_mac), MAC2STR(ap_info.bssid),
		(ap_info.rssi != 0 ? ap_info.rssi : -120), esp_get_free_heap_size());

	for (int i = 0; i < wifi_sta_list.num; i++) {
		ESP_LOGI(TAG, "Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
	}

	uint32_t size = 0;
	const node_info_list_t *node = esp_mesh_lite_get_nodes_list(&size);
	for (uint32_t loop = 0; (loop < size) && (node != NULL); loop++) {
		struct in_addr ip_struct;
		ip_struct.s_addr = node->node->ip_addr;
		printf("%ld: %d, "MACSTR", %s\r\n" , loop + 1, node->node->level, MAC2STR(node->node->mac_addr), inet_ntoa(ip_struct));
		node = node->next;
	}

	static uint32_t sequence_number = 0;
	char mac_str[MAC_MAX_LEN];
	snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(sta_mac));

	if (esp_mesh_lite_get_level() == 0) {

	} else if (esp_mesh_lite_get_level() == 1) {
		cJSON *root = cJSON_CreateObject();
		if (root) {
			cJSON_AddNumberToObject(root, "level", esp_mesh_lite_get_level());
			cJSON_AddNumberToObject(root, "number", sequence_number);
			cJSON_AddStringToObject(root, "payload", mac_str);
			cJSON_AddStringToObject(root, "target", CONFIG_IDF_TARGET);
#if CONFIG_PRINT_FORMATTED
			char *my_json_string = cJSON_Print(root);
#else
			char *my_json_string = cJSON_PrintUnformatted(root);
#endif
			int json_length = strlen(my_json_string);
			printf("[send to host] %s\n", my_json_string);
			size_t sended = xMessageBufferSendFromISR(xMessageBufferTx, my_json_string, json_length, NULL);
			if (sended != json_length) {
				ESP_LOGE(TAG, "xMessageBufferSendFromISR fail json_length=%d sended=%d", json_length, sended);
			}
			cJSON_free(my_json_string);
			cJSON_Delete(root);
			sequence_number++;
		}

	} else {

		cJSON *root = cJSON_CreateObject();
		if (root) {
			cJSON_AddNumberToObject(root, "level", esp_mesh_lite_get_level());
			cJSON_AddNumberToObject(root, "number", sequence_number);
			cJSON_AddStringToObject(root, "payload", mac_str);
			cJSON_AddStringToObject(root, "target", CONFIG_IDF_TARGET);
			char *my_json_string = cJSON_PrintUnformatted(root);
			printf("[send to root] %s\n", my_json_string);
			cJSON_free(my_json_string);
			// esp_mesh_lite_try_sending_msg will be updated to esp_mesh_lite_send_msg
			esp_mesh_lite_msg_config_t config = {
				.json_msg = {
					.send_msg = "json_id_to_root",
					.expect_msg = "json_id_to_root_ack",
					.max_retry = MAX_RETRY,
					.retry_interval = 1000,
					.req_payload = root,
					.resend = &esp_mesh_lite_send_msg_to_root,
					.send_fail = NULL,
				}
			};
			esp_err_t err = esp_mesh_lite_send_msg(ESP_MESH_LITE_JSON_MSG, &config);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "esp_mesh_lite_send_msg fail");
			}
			cJSON_Delete(root);
			sequence_number++;
		}
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
	wifi_config_t wifi_config;
	memset(&wifi_config, 0x0, sizeof(wifi_config_t));
	esp_bridge_wifi_set_config(WIFI_IF_STA, &wifi_config);

	// Softap
	snprintf((char *)wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), "%s", CONFIG_BRIDGE_SOFTAP_SSID);
	strlcpy((char *)wifi_config.ap.password, CONFIG_BRIDGE_SOFTAP_PASSWORD, sizeof(wifi_config.ap.password));
	wifi_config.ap.channel = CONFIG_MESH_CHANNEL;
	esp_bridge_wifi_set_config(WIFI_IF_AP, &wifi_config);
}

void app_wifi_set_softap_info(void)
{
	char softap_ssid[32];
	uint8_t softap_mac[6];
	esp_wifi_get_mac(WIFI_IF_AP, softap_mac);
	memset(softap_ssid, 0x0, sizeof(softap_ssid));

#ifdef CONFIG_BRIDGE_SOFTAP_SSID_END_WITH_THE_MAC
	snprintf(softap_ssid, sizeof(softap_ssid), "%.25s_%02x%02x%02x", CONFIG_BRIDGE_SOFTAP_SSID, softap_mac[3], softap_mac[4], softap_mac[5]);
#else
	snprintf(softap_ssid, sizeof(softap_ssid), "%.32s", CONFIG_BRIDGE_SOFTAP_SSID);
#endif
	esp_mesh_lite_set_softap_ssid_to_nvs(softap_ssid);
	esp_mesh_lite_set_softap_psw_to_nvs(CONFIG_BRIDGE_SOFTAP_PASSWORD);
	esp_mesh_lite_set_softap_info(softap_ssid, CONFIG_BRIDGE_SOFTAP_PASSWORD);
}

// https://gist.github.com/tswen/25d2054e868ef2b6c798a3ca05f77c7f
static cJSON* json_broadcast_handler(cJSON *payload, uint32_t seq)
{
	ESP_LOGD(__FUNCTION__, "seq=%"PRIi32, seq);
	static uint32_t last_recv_seq = 0;
	// The same message will be received MAX_RETRY times, so if it is the same message, it will be discarded.
	if (last_recv_seq != seq) {
		char *my_json_string = cJSON_PrintUnformatted(payload);
		printf("[recv from root] %s\n", my_json_string);
		cJSON_free(my_json_string);
		// esp_mesh_lite_try_sending_msg will be updated to esp_mesh_lite_send_msg
		esp_mesh_lite_msg_config_t config = {
			.json_msg = {
				.send_msg = "json_id_broadcast",
				.expect_msg = NULL,
				.max_retry = MAX_RETRY,
				.retry_interval = 1000,
				.req_payload = payload,
				.resend = &esp_mesh_lite_send_broadcast_msg_to_child,
				.send_fail = NULL,
			}
		};
		esp_err_t err = esp_mesh_lite_send_msg(ESP_MESH_LITE_JSON_MSG, &config);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "esp_mesh_lite_send_msg fail");
		}
		last_recv_seq = seq;
	}
	return NULL;
}

static cJSON* json_to_root_handler(cJSON *payload, uint32_t seq)
{
	ESP_LOGD(__FUNCTION__, "seq=%"PRIi32, seq);
	char *my_json_string = cJSON_PrintUnformatted(payload);
	printf("[recv from child] %s\n", my_json_string);
	cJSON_free(my_json_string);

#if CONFIG_PRINT_FORMATTED
	my_json_string = cJSON_Print(payload);
#else
	my_json_string = cJSON_PrintUnformatted(payload);
#endif
	int json_length = strlen(my_json_string);
	printf("[send to host] %s\n", my_json_string);
	size_t sended = xMessageBufferSendFromISR(xMessageBufferTx, my_json_string, json_length, NULL);
	if (sended != json_length) {
		ESP_LOGE(TAG, "xMessageBufferSendFromISR fail json_length=%d sended=%d", json_length, sended);
	}
	cJSON_free(my_json_string);
	return NULL;
}

static cJSON* json_to_root_ack_handler(cJSON *payload, uint32_t seq)
{
	return NULL;
}

static cJSON* json_to_parent_handler(cJSON *payload, uint32_t seq)
{
	return NULL;
}

static cJSON* json_to_parent_ack_handler(cJSON *payload, uint32_t seq)
{
	return NULL;
}

static cJSON* json_to_sibling_handler(cJSON *payload, uint32_t seq)
{
	return NULL;
}

static const esp_mesh_lite_msg_action_t json_msgs_action[] = {
	/* Send JSON to the all node */
	{"json_id_broadcast", NULL, json_broadcast_handler},

	/* Send JSON to the sibling node */
	{"json_id_to_sibling", NULL, json_to_sibling_handler},

	/* Send JSON to the root node */
	{"json_id_to_root", "json_id_to_root_ack", json_to_root_handler},
	{"json_id_to_root_ack", NULL, json_to_root_ack_handler},

	/* Send JSON to the parent node */
	{"json_id_to_parent", "json_id_to_parent_ack", json_to_parent_handler},
	{"json_id_to_parent_ack", NULL, json_to_parent_ack_handler},

	{NULL, NULL, NULL} /* Must be NULL terminated */
};

#define BUF_SIZE 128

#if CONFIG_UART_INTERFACE
void uart_init(void);
void uart_tx(void *pvParameters);
void uart_rx(void *pvParameters);
#endif

#if CONFIG_USB_INTERFACE
void usb_init(void);
void tusb_tx(void *pvParameters);
#endif

void app_main()
{
	// Set the log level for serial port printing.
	esp_log_level_set("*", ESP_LOG_INFO);

	esp_storage_init();

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	esp_bridge_create_all_netif();

	wifi_init();

	esp_mesh_lite_config_t mesh_lite_config = ESP_MESH_LITE_DEFAULT_INIT();
	mesh_lite_config.join_mesh_ignore_router_status = true;
#if CONFIG_MESH_ROOT
	mesh_lite_config.join_mesh_without_configured_wifi = false;
#else
	mesh_lite_config.join_mesh_without_configured_wifi = true;
#endif
	esp_mesh_lite_init(&mesh_lite_config);

	// Register custom message reception and recovery logic
	esp_mesh_lite_msg_action_list_register(json_msgs_action);

	app_wifi_set_softap_info();

#if CONFIG_MESH_ROOT
	ESP_LOGI(TAG, "Root node");
	esp_mesh_lite_set_allowed_level(1);
#else
	ESP_LOGI(TAG, "Child node");
	esp_mesh_lite_set_disallowed_level(1);
#endif

	esp_mesh_lite_start();

	TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_PERIOD_MS, true, NULL, print_system_info_timercb);
	xTimerStart(timer, 0);

#if CONFIG_MESH_ROOT
	// Create MessageBuffer
	xMessageBufferTx = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferTx );
	xMessageBufferRx = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferRx );

#if CONFIG_UART_INTERFACE
	uart_init();
	xTaskCreate(uart_tx, "UART-TX", 1024*3, NULL, 2, NULL);
	xTaskCreate(uart_rx, "UART-RX", 1024*3, NULL, 2, NULL);
#else
	usb_init();
	xTaskCreate(tusb_tx, "TUSB-TX", 1024*3, NULL, 2, NULL);
#endif

	// Receive from UART
	char rx_buffer[BUF_SIZE+1];
	uint32_t sequence_number = 0;
	while(1) {
		size_t received = xMessageBufferReceive(xMessageBufferRx, rx_buffer, BUF_SIZE, portMAX_DELAY);
		ESP_LOGI(TAG, "xMessageBufferReceive received=%d", received);
		if (received > 0) {
			rx_buffer[received] = 0;
			ESP_LOGD(TAG, "rx_buffer=[%s]", rx_buffer);
			cJSON *root = cJSON_CreateObject();
			if (root) {
				cJSON_AddNumberToObject(root, "number", sequence_number);
				cJSON_AddStringToObject(root, "payload", rx_buffer);
				char *my_json_string = cJSON_PrintUnformatted(root);
				printf("[send to all child] %s\n", my_json_string);
				cJSON_free(my_json_string);
				// esp_mesh_lite_try_sending_msg will be updated to esp_mesh_lite_send_msg
				esp_mesh_lite_msg_config_t config = {
					.json_msg = {
						.send_msg = "json_id_broadcast",
						.expect_msg = NULL,
						.max_retry = MAX_RETRY,
						.retry_interval = 1000,
						.req_payload = root,
						.resend = &esp_mesh_lite_send_broadcast_msg_to_child,
						.send_fail = NULL,
					}
				};
				esp_err_t err = esp_mesh_lite_send_msg(ESP_MESH_LITE_JSON_MSG, &config);
				if (err != ESP_OK) {
					ESP_LOGE(TAG, "esp_mesh_lite_send_msg fail");
				}
				cJSON_Delete(root);
				sequence_number++;
			}
		}
	} // end while
#endif
}
