/*
	WebSocket server over MESH Example

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
#include "freertos/event_groups.h"
#include "freertos/message_buffer.h"
#include "freertos/timers.h"

#include "esp_wifi.h"
#include "nvs_flash.h"
#include "mdns.h"
#include <sys/socket.h>

#include "esp_mac.h"
#include "esp_bridge.h"
#include "esp_mesh_lite.h"

#include "websocket_server.h"

static const char *TAG = "MAIN";

#if CONFIG_MESH_ROOT
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0

MessageBufferHandle_t xMessageBufferNodeManager;
MessageBufferHandle_t xMessageBufferJsonServer;
MessageBufferHandle_t xMessageBufferToClient;

// The total number of bytes (not messages) the message buffer will be able to hold at any one time.
size_t xBufferSizeBytes = 2048;

#endif

#define MAX_RETRY  5

/**
 * @brief Timed printing system information
 */
static void print_system_info_timercb(TimerHandle_t timer)
{
	uint8_t primary					= 0;
	uint8_t ap_mac[6]				= {0};
	uint8_t sta_mac[6]				= {0};
	wifi_ap_record_t ap_info		= {0};
	wifi_second_chan_t second		= 0;
	wifi_sta_list_t wifi_sta_list	= {0x0};

	if (esp_mesh_lite_get_level() > 1) {
		esp_wifi_sta_get_ap_info(&ap_info);
	}
	esp_wifi_get_mac(ESP_IF_WIFI_AP, ap_mac);
	esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
	ESP_LOGI(TAG, "ap_mac=[" MACSTR "] sta_mac=[" MACSTR "]", MAC2STR(ap_mac), MAC2STR(sta_mac));
	esp_wifi_ap_get_sta_list(&wifi_sta_list);
	esp_wifi_get_channel(&primary, &second);

	ESP_LOGI(TAG, "System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
			 ", parent rssi: %d, free heap: %"PRIu32"", primary,
			 esp_mesh_lite_get_level(), MAC2STR(ap_mac), MAC2STR(ap_info.bssid),
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

	static uint32_t seq_number = 0;
	char self_mac[MAC_MAX_LEN];
	snprintf(self_mac, sizeof(self_mac), MACSTR, MAC2STR(ap_mac));
	//snprintf(self_mac, sizeof(self_mac), MACSTR, MAC2STR(sta_mac));
	char parent_mac[MAC_MAX_LEN];
	snprintf(parent_mac, sizeof(parent_mac), MACSTR, MAC2STR(ap_info.bssid));
	int parent_rssi = (ap_info.rssi != 0 ? ap_info.rssi : -120);
	uint32_t free_heap = esp_get_free_heap_size();

	if (esp_mesh_lite_get_level() == 0) {

	} else if (esp_mesh_lite_get_level() == 1) {
#if CONFIG_MESH_ROOT
		cJSON *item = cJSON_CreateObject();
		if (item) {
			cJSON_AddNumberToObject(item, "level", esp_mesh_lite_get_level());
			cJSON_AddNumberToObject(item, "seq_number", seq_number);
			cJSON_AddStringToObject(item, "self_mac", self_mac);
			cJSON_AddStringToObject(item, "parent_mac", parent_mac);
			cJSON_AddNumberToObject(item, "parent_rssi", parent_rssi);
			cJSON_AddNumberToObject(item, "free_heap", free_heap);
			cJSON_AddStringToObject(item, "target", CONFIG_IDF_TARGET);
			cJSON_AddStringToObject(item, "node_comment", "root");

			char *my_json_string = cJSON_PrintUnformatted(item);
			ESP_LOGI(TAG, "my_json_string\n%s",my_json_string);
			int json_length = strlen(my_json_string);
			ESP_LOGI(TAG, "json_length=%d", json_length);
			size_t sended = xMessageBufferSendFromISR(xMessageBufferNodeManager, my_json_string, json_length, NULL);
			if (sended != json_length) {
				ESP_LOGE(TAG, "xMessageBufferSendFromISR fail json_length=%d sended=%d", json_length, sended);
			}
			cJSON_free(my_json_string);
			cJSON_Delete(item);
			seq_number++;
		}
#endif

	} else {

#if !CONFIG_MESH_ROOT
		// Sending messages from all leaves to the root
		// esp_mesh_lite_try_sending_msg("report_info_to_root")
		//	 -->report_info_to_root_process()
		//	 -->report_info_to_root_ack()
		//	 When a message of the expected type is received, stop retransmitting.
		cJSON *item = cJSON_CreateObject();
		if (item) {
			printf("[send to root] level: %d seq_number: %"PRIu32" self_mac: [%s] parent_mac: [%s]\r\n", esp_mesh_lite_get_level(), seq_number, self_mac, parent_mac);
			cJSON_AddNumberToObject(item, "level", esp_mesh_lite_get_level());
			cJSON_AddNumberToObject(item, "seq_number", seq_number);
			cJSON_AddStringToObject(item, "self_mac", self_mac);
			cJSON_AddStringToObject(item, "parent_mac", parent_mac);
			cJSON_AddNumberToObject(item, "parent_rssi", parent_rssi);
			cJSON_AddNumberToObject(item, "free_heap", free_heap);
			cJSON_AddStringToObject(item, "target", CONFIG_IDF_TARGET);
			cJSON_AddStringToObject(item, "node_comment", CONFIG_NODE_COMMENT);
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
			cJSON_Delete(item);
			seq_number++;
		}
#endif
	}
}

#if CONFIG_MESH_ROOT
static void ip_event_sta_got_ip_handler(void *arg, esp_event_base_t event_base,
										int32_t event_id, void *event_data)
{
	static bool tcp_task = false;
	ESP_LOGI(TAG, "ip_event_sta_got_ip_handler");
	if (!tcp_task) {
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
		tcp_task = true;
	}
}

void initialize_mdns(void)
{
	//initialize mDNS
	ESP_ERROR_CHECK( mdns_init() );
	//set mDNS hostname (required if you want to advertise services)
	ESP_ERROR_CHECK( mdns_hostname_set(CONFIG_MDNS_HOSTNAME) );
	ESP_LOGI(TAG, "mdns hostname set to: [%s]", CONFIG_MDNS_HOSTNAME);

#if 0
	//set default mDNS instance name
	ESP_ERROR_CHECK( mdns_instance_name_set("ESP32 with mDNS") );
#endif
}
#endif

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
#if CONFIG_MESH_ROOT
	cJSON *found = NULL;

	found = cJSON_GetObjectItem(payload, "level");
	uint8_t level = found->valueint;
	found = cJSON_GetObjectItem(payload, "seq_number");
	uint32_t seq_number = found->valueint;
	found = cJSON_GetObjectItem(payload, "self_mac");
	char *self_mac = found->valuestring;
	found = cJSON_GetObjectItem(payload, "parent_mac");
	char *parent_mac = found->valuestring;
	printf("[recv from child] level: %d seq_number=%"PRIu32" self_mac: [%s] parent_mac: [%s]\r\n", level, seq_number, self_mac, parent_mac);

	char *my_json_string = cJSON_PrintUnformatted(payload);
	ESP_LOGI(TAG, "my_json_string\n%s",my_json_string);
	int json_length = strlen(my_json_string);
	size_t sended = xMessageBufferSendFromISR(xMessageBufferNodeManager, my_json_string, json_length, NULL);
	if (sended != json_length) {
		ESP_LOGE(TAG, "xMessageBufferSendFromISR fail json_length=%d sended=%d", json_length, sended);
	}
	cJSON_free(my_json_string);

	//esp_mesh_lite_node_info_add was updated to esp_mesh_lite_node_info_update
	//esp_mesh_lite_node_info_add(level, found->valuestring);
#endif
	return NULL;
}

static cJSON* report_info_to_root_ack_process(cJSON *payload, uint32_t seq)
{
	return NULL;
}

static cJSON* report_info_to_parent_process(cJSON *payload, uint32_t seq)
{
	cJSON *found = NULL;

	found = cJSON_GetObjectItem(payload, "level");
	uint8_t level = found->valueint;
	found = cJSON_GetObjectItem(payload, "seq_number");
	uint32_t seq_number = found->valueint;
	found = cJSON_GetObjectItem(payload, "self_mac");
	char *self_mac = found->valuestring;
	found = cJSON_GetObjectItem(payload, "parent_mac");
	char *parent_mac = found->valuestring;
	printf("[recv from child] level: %d, seq_number=%"PRIu32", self_mac: [%s] parent_mac[%s]\r\n", level, seq_number, self_mac, parent_mac);

	return NULL;
}

static cJSON* report_info_to_parent_ack_process(cJSON *payload, uint32_t seq)
{
	return NULL;
}

static cJSON* report_info_to_sibling_process(cJSON *payload, uint32_t seq)
{
	cJSON *found = NULL;

	found = cJSON_GetObjectItem(payload, "level");
	uint8_t level = found->valueint;
	found = cJSON_GetObjectItem(payload, "seq_number");
	uint32_t seq_number = found->valueint;
	found = cJSON_GetObjectItem(payload, "self_mac");
	char *self_mac = found->valuestring;
	found = cJSON_GetObjectItem(payload, "parent_mac");
	char *parent_mac = found->valuestring;
	printf("[recv from sibling] level: %d, seq_number=%"PRIu32", self_mac: [%s] parent_mac: [%s]\r\n", level, seq_number, self_mac, parent_mac);

	return NULL;
}

static cJSON* broadcast_process(cJSON *payload, uint32_t seq)
{
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

#if CONFIG_MESH_ROOT
void json_server(void *pvParameters);
void node_manager(void *pvParameters);
void client_task(void* pvParameters);
void server_task(void* pvParameters);
#endif

void app_main()
{
	// Set the log level for serial port printing.
	esp_log_level_set("*", ESP_LOG_INFO);

	ESP_ERROR_CHECK(esp_storage_init());

#if CONFIG_MESH_ROOT
	// Create MessageBuffer
	xMessageBufferNodeManager = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferNodeManager );
	xMessageBufferJsonServer = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferJsonServer );
	xMessageBufferToClient = xMessageBufferCreate(1024);
	configASSERT( xMessageBufferToClient );
#endif

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	esp_bridge_create_all_netif();

	wifi_init();

	esp_mesh_lite_config_t mesh_lite_config = ESP_MESH_LITE_DEFAULT_INIT();
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

#if CONFIG_MESH_ROOT
	// Create handler
	s_wifi_event_group = xEventGroupCreate();
	xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_sta_got_ip_handler, NULL, NULL));

	// Waiting until the connection is established (WIFI_CONNECTED_BIT)
	xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

	// Initialize mDNS
	ESP_LOGI(TAG, "start mdns_init");
	initialize_mdns();

	// Get the local IP address
	esp_netif_ip_info_t ip_info;
	ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info));
	char cparam0[64];
	sprintf(cparam0, IPSTR, IP2STR(&ip_info.ip));
	ESP_LOGI(TAG, "cparam0=[%s]", cparam0);

	// Start web socket server
	ws_server_start();

	// Start web server
	xTaskCreate(&server_task, "server_task", 1024*4, (void *)cparam0, 5, NULL);

	// Start web client
	xTaskCreate(&client_task, "client_task", 1024*6, NULL, 5, NULL);

	// Start json server
	xTaskCreate(json_server, "json_server", 1024*4, NULL, 5, NULL);

	// Start node manager
	xTaskCreate(node_manager, "node_manager", 1024*8, NULL, 5, NULL);

#endif

	TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_PERIOD_MS, true, NULL, print_system_info_timercb);
	xTimerStart(timer, 0);

	// Preventing local variables from being destroyed
	while(1) {
		vTaskDelay(1000);
	}

}
