/*
	TWAI to MESH bridge Example

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

#include "driver/twai.h" // Update from V4.2

static const char *TAG = "MAIN";

MessageBufferHandle_t xMessageBufferTx;
MessageBufferHandle_t xMessageBufferRx;

// The total number of bytes (not messages) the message buffer will be able to hold at any one time.
size_t xBufferSizeBytes = 1024;

#define MAX_RETRY  5

#if CONFIG_MESH_ROOT
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

#if CONFIG_CAN_BITRATE_25
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_25KBITS();
#define BITRATE "Bitrate is 25 Kbit/s"
#elif CONFIG_CAN_BITRATE_50
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_50KBITS();
#define BITRATE "Bitrate is 50 Kbit/s"
#elif CONFIG_CAN_BITRATE_100
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_100KBITS();
#define BITRATE "Bitrate is 100 Kbit/s"
#elif CONFIG_CAN_BITRATE_125
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();
#define BITRATE "Bitrate is 125 Kbit/s"
#elif CONFIG_CAN_BITRATE_250
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
#define BITRATE "Bitrate is 250 Kbit/s"
#elif CONFIG_CAN_BITRATE_500
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
#define BITRATE "Bitrate is 500 Kbit/s"
#elif CONFIG_CAN_BITRATE_800
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_800KBITS();
#define BITRATE "Bitrate is 800 Kbit/s"
#elif CONFIG_CAN_BITRATE_1000
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
#define BITRATE "Bitrate is 1 Mbit/s"
#endif

static const twai_general_config_t g_config =
	TWAI_GENERAL_CONFIG_DEFAULT(CONFIG_CTX_GPIO, CONFIG_CRX_GPIO, TWAI_MODE_NORMAL);
#endif


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
	esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
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

#if !CONFIG_MESH_ROOT
	} else {

		// Sending messages from all leaves to the root
		// esp_mesh_lite_try_sending_msg("report_info_to_root")
		//	 -->report_info_to_root_process()
		//	 -->report_info_to_root_ack()
		//	 When a message of the expected type is received, stop retransmitting.
		cJSON *item = cJSON_CreateObject();
		if (item) {
			printf("[send to root] level: %d, number: %"PRIu32", payload: [%s]\r\n", esp_mesh_lite_get_level(), sequence_number, mac_str);
			cJSON_AddNumberToObject(item, "level", esp_mesh_lite_get_level());
			cJSON_AddNumberToObject(item, "number", sequence_number);
#if CONFIG_CAN_STD_FRAME
			cJSON_AddStringToObject(item, "frame", "std");
#else
			cJSON_AddStringToObject(item, "frame", "ext");
#endif
			cJSON_AddNumberToObject(item, "canid", CONFIG_CAN_ID);
			cJSON_AddStringToObject(item, "payload", mac_str);
			cJSON_AddStringToObject(item, "target", CONFIG_IDF_TARGET);
			esp_mesh_lite_try_sending_msg("report_info_to_root", "report_info_to_root_ack", MAX_RETRY, item, &esp_mesh_lite_send_msg_to_root);
#if 0
			// esp_mesh_lite_try_sending_msg will be updated to esp_mesh_lite_send_msg
			esp_mesh_lite_msg_config_t config = {
				.json_msg = {
					.send_msg = "report_info_to_root",
					.expect_msg = "report_info_to_root_ack",
					.max_retry = MAX_RETRY,
					.retry_interval = 1000,
					.req_payload = item,
					.resend = &esp_mesh_lite_send_msg_to_root,
					.send_fail = NULL,
				}
			};
			esp_mesh_lite_send_msg(ESP_MESH_LITE_JSON_MSG, &config);
#endif
			sequence_number++;
			cJSON_Delete(item);
		}
#endif
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
static cJSON* report_info_to_root_process(cJSON *payload, uint32_t seq)
{
	cJSON *found = NULL;

	found = cJSON_GetObjectItem(payload, "level");
	uint8_t level = found->valueint;
	found = cJSON_GetObjectItem(payload, "number");
	uint32_t number = found->valueint;
	found = cJSON_GetObjectItem(payload, "payload");
	char *_payload = found->valuestring;
	printf("[recv from child] level: %d, number=%"PRIu32", payload: [%s]\r\n", level, number,  _payload);

#if CONFIG_PRINT_FORMATTED
	char *my_json_string = cJSON_Print(payload);
#else
	char *my_json_string = cJSON_PrintUnformatted(payload);
#endif
	ESP_LOGI(TAG, "my_json_string\n%s",my_json_string);
	int json_length = strlen(my_json_string);
	size_t sended = xMessageBufferSendFromISR(xMessageBufferTx, my_json_string, json_length, NULL);
	if (sended != json_length) {
		ESP_LOGE(TAG, "xMessageBufferSendFromISR fail json_length=%d sended=%d", json_length, sended);
	}
	cJSON_free(my_json_string);

	//esp_mesh_lite_node_info_add was updated to esp_mesh_lite_node_info_update
	//esp_mesh_lite_node_info_add(level, found->valuestring);
	return NULL;
}

static cJSON* report_info_to_root_ack_process(cJSON *payload, uint32_t seq)
{
	return NULL;
}

static cJSON* report_info_to_parent_process(cJSON *payload, uint32_t seq)
{
	return NULL;
}

static cJSON* report_info_to_parent_ack_process(cJSON *payload, uint32_t seq)
{
	return NULL;
}

static cJSON* report_info_to_sibling_process(cJSON *payload, uint32_t seq)
{
	return NULL;
}

static cJSON* broadcast_process(cJSON *payload, uint32_t seq)
{
	static uint32_t last_recv_seq;
	ESP_LOGD(__FUNCTION__, "last_recv_seq=%"PRIu32" seq=%"PRIu32,last_recv_seq, seq);

	// Receive the same message MAX_RETRY times
	// Discard duplicate messages
	if (last_recv_seq != seq) {
		cJSON *found = NULL;
		found = cJSON_GetObjectItem(payload, "ext");
		int ext = found->valueint;
		found = cJSON_GetObjectItem(payload, "rtr");
		int rtr = found->valueint;
		found = cJSON_GetObjectItem(payload, "id");
		uint32_t id = found->valueint;
		found = cJSON_GetObjectItem(payload, "length");
		int length = found->valueint;
		cJSON *array = cJSON_GetObjectItem(payload,"data");
		int array_size = cJSON_GetArraySize(array); 
		cJSON *element[array_size];
		int data[array_size];
		for (int i=0;i<array_size;i++) {
			element[i] = cJSON_GetArrayItem(array, i);
			data[i] = element[i]->valueint;
		}
		
		printf("[recv from root] ext: [%d] rtr: [%d] id: %"PRIx32" length: %d data: ", ext, rtr, id, length);
		for (int i=0;i<array_size;i++) {
			printf("0x%02x ", data[i]);
		}
		printf("\n");

		cJSON *item = cJSON_Duplicate(payload, true);
		if (item) {
			esp_mesh_lite_try_sending_msg("broadcast", NULL, MAX_RETRY, payload, &esp_mesh_lite_send_broadcast_msg_to_child);
			cJSON_Delete(item);
		}
		last_recv_seq = seq;
	}
	return NULL;
}

#if 0
typedef struct esp_mesh_lite_msg_action {
	const char* type;		  /**< The type of message sent */
	const char* rsp_type;	  /**< The message type expected to be received. When a message of the expected type is received, stop retransmitting.
								If set to NULL, it will be sent until the maximum number of retransmissions is reached. */
	msg_process_cb_t process; /**< The callback function when receiving the 'type' message. The cjson information in the type message can be processed in this cb. */
} esp_mesh_lite_msg_action_t;
#endif

static const esp_mesh_lite_msg_action_t node_report_action[] = {
	{"broadcast", NULL, broadcast_process},

	/* Report information to the sibling node */
	{"report_info_to_sibling", NULL, report_info_to_sibling_process},

	/* Report information to the root node */
	{"report_info_to_root", "report_info_to_root_ack", report_info_to_root_process},
	{"report_info_to_root_ack", NULL, report_info_to_root_ack_process},

	/* Report information to the root node */
	{"report_info_to_parent", "report_info_to_parent_ack", report_info_to_parent_process},
	{"report_info_to_parent_ack", NULL, report_info_to_parent_ack_process},

	{NULL, NULL, NULL} /* Must be NULL terminated */
};

#define BUF_SIZE 128

void twai_tx(void *pvParameters);
void twai_rx(void *pvParameters);

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
	esp_mesh_lite_msg_action_list_register(node_report_action);

	app_wifi_set_softap_info();

#if CONFIG_MESH_ROOT
	ESP_LOGI(TAG, "Root node");
	esp_mesh_lite_set_allowed_level(1);
#else
	ESP_LOGI(TAG, "Child node");
	esp_mesh_lite_set_disallowed_level(1);
#endif

	esp_mesh_lite_start();

	TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_PERIOD_MS,
									   true, NULL, print_system_info_timercb);
	xTimerStart(timer, 0);

#if CONFIG_MESH_ROOT
	// Create MessageBuffer
	xMessageBufferTx = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferTx );
	xMessageBufferRx = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferRx );

	// Initialize TWAI
	ESP_LOGI(TAG, "%s",BITRATE);
	ESP_LOGI(TAG, "CTX_GPIO=%d",CONFIG_CTX_GPIO);
	ESP_LOGI(TAG, "CRX_GPIO=%d",CONFIG_CRX_GPIO);

	ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
	ESP_LOGI(TAG, "Driver installed");
	ESP_ERROR_CHECK(twai_start());
	ESP_LOGI(TAG, "Driver started");

	// Start task
	xTaskCreate(twai_tx, "TWAI-TX", 1024*3, NULL, 2, NULL);
	xTaskCreate(twai_rx, "TWAI-RX", 1024*3, NULL, 2, NULL);

	// Receive from TWAI
	char rx_buffer[BUF_SIZE+1];
	while(1) {
		size_t received = xMessageBufferReceive(xMessageBufferRx, rx_buffer, BUF_SIZE, portMAX_DELAY);
		ESP_LOGI(TAG, "xMessageBufferReceive received=%d", received);
		if (received > 0) {
			rx_buffer[received] = 0;
			ESP_LOGI(TAG, "rx_buffer=[%s]", rx_buffer);

			cJSON *item = cJSON_Parse(rx_buffer);
			// Sending messages from the root to all leaves
			// esp_mesh_lite_try_sending_msg("broadcast")
			//	 --> broadcast_process()
			//	 --> esp_mesh_lite_try_sending_msg("broadcast")
			//	 --> broadcast_process()
			//	 --> esp_mesh_lite_try_sending_msg("broadcast")
			//	 it will be sent until the maximum number of retransmissions is reached
			if (item) {
				esp_mesh_lite_try_sending_msg("broadcast", NULL, MAX_RETRY, item, &esp_mesh_lite_send_broadcast_msg_to_child);
#if 0
				// esp_mesh_lite_try_sending_msg will be updated to esp_mesh_lite_send_msg
				esp_mesh_lite_msg_config_t config = {
					.json_msg = {
						.send_msg = "broadcast",
						.expect_msg = NULL,
						.max_retry = MAX_RETRY,
						.retry_interval = 1000,
						.req_payload = item,
						.resend = &esp_mesh_lite_send_broadcast_msg_to_child,
						.send_fail = NULL,
					}
				};
				esp_mesh_lite_send_msg(ESP_MESH_LITE_JSON_MSG, &config);
#endif
				cJSON_Delete(item);
			}
		} else {
			ESP_LOGE(TAG, "xMessageBufferReceive faul");
		}
	} // end while
#endif
}
