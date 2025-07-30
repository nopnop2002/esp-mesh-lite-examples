/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_wifi.h"
#include "nvs_flash.h"
#include <sys/socket.h>

#include "esp_mac.h"
#include "esp_bridge.h"
#include "esp_mesh_lite.h"

static const char *TAG = "no_router";

#define MAX_RETRY  5
#define RAW_MSG_ID_TO_ROOT 1
#define RAW_MSG_ID_TO_SIBLING 2
#define RAW_MSG_ID_TO_ROOT_RESP 3
#define RAW_MSG_ID_TO_PARENT 4
#define RAW_MSG_ID_TO_PARENT_RESP 5
#define RAW_MSG_ID_BROADCAST 6


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

#if 0
	uint32_t size = 0;
	const node_info_list_t *node = esp_mesh_lite_get_nodes_list(&size);
	for (uint32_t loop = 0; (loop < size) && (node != NULL); loop++) {
		struct in_addr ip_struct;
		ip_struct.s_addr = node->node->ip_addr;
		printf("%ld: %d, "MACSTR", %s\r\n" , loop + 1, node->node->level, MAC2STR(node->node->mac_addr), inet_ntoa(ip_struct));
		node = node->next;
	}
#endif

	static uint32_t sequence_number = 0;
	char mac_str[MAC_MAX_LEN];
	snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(sta_mac));

	if (esp_mesh_lite_get_level() == 0) {

	} else if (esp_mesh_lite_get_level() == 1) {

		// How to send a messages from the root to all leaves.
		// Use esp_mesh_lite_send_msg() to send a message from level 1 to a node on level 2.
		// Level 2 node receives message with json_broadcast_handler().
		// Use esp_mesh_lite_send_msg() to send a message from level 2 to a node on level 3.
		// Level 3 node receives message with json_broadcast_handler().
		// Repeat until there are no more children.

#if CONFIG_JSON_FORMAT
		cJSON *root = cJSON_CreateObject();
		if (root) {
			cJSON_AddNumberToObject(root, "number", sequence_number);
			cJSON_AddStringToObject(root, "mac", mac_str);
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
		}
#endif // CONFIG_JSON_FORMAT

#if CONFIG_RAW_FORMAT
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "number:%ld mac:%s", sequence_number, mac_str);
		printf("[send to all child] %s\n", buffer);
		// esp_mesh_lite_try_sending_msg will be updated to esp_mesh_lite_send_msg
		esp_mesh_lite_msg_config_t config = {
			.raw_msg = {
				.msg_id = RAW_MSG_ID_BROADCAST,
				.expect_resp_msg_id = 0,
				.max_retry = MAX_RETRY,
				.retry_interval = 1000,
				.data = (uint8_t *)buffer,
				.size = strlen(buffer),
				.raw_resend = &esp_mesh_lite_send_broadcast_raw_msg_to_child,
				.raw_send_fail = NULL,
			}
		};
		esp_err_t err = esp_mesh_lite_send_msg(ESP_MESH_LITE_RAW_MSG, &config);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "esp_mesh_lite_send_msg fail");
		}
#endif // CONFIG_RAW_FORMAT
		sequence_number++;

	} else {

#if CONFIG_JSON_FORMAT
		cJSON *root = cJSON_CreateObject();
		if (root) {
			cJSON_AddNumberToObject(root, "level", esp_mesh_lite_get_level());
			cJSON_AddNumberToObject(root, "number", sequence_number);
			cJSON_AddStringToObject(root, "mac", mac_str);
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
		}
#endif // CONFIG_JSON_FORMAT

#if CONFIG_RAW_FORMAT
		char buffer[128];
		sprintf(buffer, "level:%d number:%ld mac:%s", esp_mesh_lite_get_level(), sequence_number, mac_str);
		printf("[send to root] %s\n", buffer);
		// esp_mesh_lite_try_sending_msg will be updated to esp_mesh_lite_send_msg
		esp_mesh_lite_msg_config_t config = {
			.raw_msg = {
				.msg_id = RAW_MSG_ID_TO_ROOT,
				.expect_resp_msg_id = 0,
				.max_retry = MAX_RETRY,
				.retry_interval = 1000,
				.data = (uint8_t *)buffer,
				.size = strlen(buffer),
				.raw_resend = &esp_mesh_lite_send_raw_msg_to_root,
				.raw_send_fail = NULL,
			}
		};
		esp_err_t err = esp_mesh_lite_send_msg(ESP_MESH_LITE_RAW_MSG, &config);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "esp_mesh_lite_send_msg fail");
		}
#endif // CONFIG_RAW_FORMAT
		sequence_number++;
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
	return NULL;
}

static cJSON* json_to_root_ack_handler(cJSON *payload, uint32_t seq)
{
	ESP_LOGD(__FUNCTION__, "seq=%"PRIi32, seq);
	return NULL;
}

static cJSON* json_to_parent_handler(cJSON *payload, uint32_t seq)
{
	ESP_LOGD(__FUNCTION__, "seq=%"PRIi32, seq);
	return NULL;
}

static cJSON* json_to_parent_ack_handler(cJSON *payload, uint32_t seq)
{
	ESP_LOGD(__FUNCTION__, "seq=%"PRIi32, seq);
	return NULL;
}

static cJSON* json_to_sibling_handler(cJSON *payload, uint32_t seq)
{
	ESP_LOGD(__FUNCTION__, "seq=%"PRIi32, seq);
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

static esp_err_t raw_broadcast_handler(uint8_t *data, uint32_t len, uint8_t **out_data, uint32_t* out_len, uint32_t seq)
{
	ESP_LOGD(__FUNCTION__, "seq=%"PRIi32" len=%"PRIi32, seq, len);
	static uint32_t last_recv_seq = 0;
	// The same message will be received MAX_RETRY times, so if it is the same message, it will be discarded.
	if (last_recv_seq != seq) {
		printf("[recv from root] %.*s\n", (int)len, (char *)data);
		// esp_mesh_lite_try_sending_msg will be updated to esp_mesh_lite_send_msg
		esp_mesh_lite_msg_config_t config = {
			.raw_msg = {
				.msg_id = RAW_MSG_ID_BROADCAST,
				.expect_resp_msg_id = 0,
				.max_retry = MAX_RETRY,
				.retry_interval = 1000,
				.data = data,
				.size = len,
				.raw_resend = &esp_mesh_lite_send_broadcast_raw_msg_to_child,
				.raw_send_fail = NULL,
			}
		};
		esp_err_t err = esp_mesh_lite_send_msg(ESP_MESH_LITE_RAW_MSG, &config);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "esp_mesh_lite_send_msg fail");
		}
		last_recv_seq = seq;
	}
	*out_len = 0;
	return ESP_OK;
}

static esp_err_t raw_to_sibling_handler(uint8_t *data, uint32_t len, uint8_t **out_data, uint32_t* out_len, uint32_t seq)
{
	ESP_LOGD(__FUNCTION__, "seq=%"PRIi32" len=%"PRIi32, seq, len);
	return ESP_OK;
}

static esp_err_t raw_to_root_handler(uint8_t *data, uint32_t len, uint8_t **out_data, uint32_t* out_len, uint32_t seq)
{
	ESP_LOGD(__FUNCTION__, "seq=%"PRIi32" len=%"PRIi32, seq, len);
	static uint32_t last_recv_seq = 0;
	// The same message will be received MAX_RETRY times, so if it is the same message, it will be discarded.
	if (last_recv_seq != seq) {
		printf("[recv from child] %.*s\n", (int)len, (char *)data);
		last_recv_seq = seq;
	}
	*out_len = 0;
	return ESP_OK;
}

static esp_err_t raw_to_root_resp_handler(uint8_t *data, uint32_t len, uint8_t **out_data, uint32_t* out_len, uint32_t seq)
{
	ESP_LOGD(__FUNCTION__, "seq=%"PRIi32" len=%"PRIi32, seq, len);
	return ESP_OK;
}

static esp_err_t raw_to_parent_handler(uint8_t *data, uint32_t len, uint8_t **out_data, uint32_t* out_len, uint32_t seq)
{
	ESP_LOGD(__FUNCTION__, "seq=%"PRIi32" len=%"PRIi32, seq, len);
	return ESP_OK;
}

static esp_err_t raw_to_parent_resp_handler(uint8_t *data, uint32_t len, uint8_t **out_data, uint32_t* out_len, uint32_t seq)
{
	ESP_LOGD(__FUNCTION__, "seq=%"PRIi32" len=%"PRIi32, seq, len);
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
	esp_mesh_lite_raw_msg_action_list_register(raw_msgs_action);

	app_wifi_set_softap_info();

#if CONFIG_ENCRYPTION
	ESP_LOGW(TAG, "Enable AES encryption");
	unsigned char key[32];
	for (int i=0;i<sizeof(key);i++) key[i] = i;
	esp_mesh_lite_aes_set_key(key, 256);
#endif

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
}
