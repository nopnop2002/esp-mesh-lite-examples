/*
	HTTP server over MESH Example

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
#include "esp_spiffs.h"
#include "mdns.h"
#include <sys/socket.h> // ip_struct/inet_ntoa
#include "rom/ets_sys.h" // ets_get_cpu_frequency()
#include "esp_chip_info.h" // esp_chip_info_t
#include "esp_flash.h" // esp_flash_get_size

#include "driver/gpio.h" // gpio_dump_io_configuration
#include "driver/spi_master.h" // SPI_HOST_MAX
#include "driver/i2c.h" // I2C_NUM_MAX
#include "driver/uart.h" // UART_NUM_MAX

#include "esp_mac.h"
#include "esp_bridge.h"
#include "esp_mesh_lite.h"

static const char *TAG = "MAIN";

int gpio_count = 0;

#define GPIO_TABLE_SIZE (64)
int gpio_table[GPIO_TABLE_SIZE];

#if CONFIG_MESH_ROOT
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0

MessageBufferHandle_t xMessageBufferNode;

// The total number of bytes (not messages) the message buffer will be able to hold at any one time.
size_t xBufferSizeBytes = 1024;
#endif

#define MAX_RETRY  5

static esp_err_t build_system_object(cJSON *root)
{
	cJSON *system = cJSON_CreateObject();
	if (system == NULL) {
		ESP_LOGE(TAG, "cJSON_CreateObject fail");
		return ESP_ERR_NO_MEM;
	}

	char wk[64];
	sprintf(wk, "%s@%"PRIu32"Mhz", CONFIG_IDF_TARGET, ets_get_cpu_frequency());
	cJSON_AddStringToObject(system, "soc", wk);

	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	sprintf(wk, "%d CPU cores. WiFi%s%s%s%s",
		chip_info.cores,
		(chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "/EMB" : "",
		(chip_info.features & CHIP_FEATURE_IEEE802154) ? "/802.15.4" : "",
		(chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
		(chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
	cJSON_AddStringToObject(system, "core", wk);

	uint32_t embedded = chip_info.features & CHIP_FEATURE_EMB_FLASH;
	if (embedded) {
		uint32_t size_flash_chip;
		esp_flash_get_size(NULL, &size_flash_chip);
		sprintf(wk, "%"PRIi32"MB", size_flash_chip / (1024 * 1024));
		strcat(wk, " [embedded]");
	} else {
		uint32_t chip_id;
		ESP_ERROR_CHECK(esp_flash_read_id(NULL, &chip_id));
		ESP_LOGI(TAG, "chip ID=0x%"PRIx32, chip_id);
		int flash_capacity = chip_id & 0xff;
		ESP_LOGI(TAG, "flash_capacity=0x%x", flash_capacity);
		int external_flash_size = 0;
		if (flash_capacity == 0x15) {
			external_flash_size = 2;
		} else if (flash_capacity == 0x16) {
			external_flash_size = 4;
		} else if (flash_capacity == 0x17) {
			external_flash_size = 8;
		} else if (flash_capacity == 0x18) {
			external_flash_size = 16;
		} else if (flash_capacity == 0x19) {
			external_flash_size = 32;
		} else if (flash_capacity == 0x20) {
			external_flash_size = 64;
		}
		sprintf(wk, "%dMB", external_flash_size);
		strcat(wk, " [external]");
	}
	cJSON_AddStringToObject(system, "flash", wk);

	// gpio array
	cJSON *array_gpio = cJSON_AddArrayToObject(system, "gpio");
	if (array_gpio == NULL) {
		ESP_LOGE(TAG, "cJSON_CreateArray fail");
		return ESP_ERR_NO_MEM;
	}
	cJSON *gpio[gpio_count];
	for (int i=0;i<gpio_count;i++) {
		gpio[i] = cJSON_CreateNumber(gpio_table[i]);
		cJSON_AddItemToArray(array_gpio, gpio[i]);
	}

	ESP_LOGI(TAG, "SPI_HOST_MAX=%d", SPI_HOST_MAX);
	// spi array
	cJSON *array_spi = cJSON_AddArrayToObject(system, "spi");
	if (array_gpio == NULL) {
		ESP_LOGE(TAG, "cJSON_CreateArray fail");
		return ESP_ERR_NO_MEM;
	}
	cJSON *spi[SPI_HOST_MAX];
	for (int i=0;i<SPI_HOST_MAX;i++) {
		spi[i] = cJSON_CreateNumber(i);
		cJSON_AddItemToArray(array_spi, spi[i]);
	}

	ESP_LOGI(TAG, "I2C_NUM_MAX=%d", I2C_NUM_MAX);
	// i2c array
	cJSON *array_i2c = cJSON_AddArrayToObject(system, "i2c");
	if (array_gpio == NULL) {
		ESP_LOGE(TAG, "cJSON_CreateArray fail");
		return ESP_ERR_NO_MEM;
	}
	cJSON *i2c[I2C_NUM_MAX];
	for (int i=0;i<I2C_NUM_MAX;i++) {
		i2c[i] = cJSON_CreateNumber(i);
		cJSON_AddItemToArray(array_i2c, i2c[i]);
	}

	ESP_LOGI(TAG, "UART_NUM_MAX=%d", UART_NUM_MAX);
	// uart array
	cJSON *array_uart = cJSON_AddArrayToObject(system, "uart");
	if (array_gpio == NULL) {
		ESP_LOGE(TAG, "cJSON_CreateArray fail");
		return ESP_ERR_NO_MEM;
	}
	cJSON *uart[UART_NUM_MAX];
	for (int i=0;i<UART_NUM_MAX;i++) {
		uart[i] = cJSON_CreateNumber(i);
		cJSON_AddItemToArray(array_uart, uart[i]);
	}

	cJSON_AddItemToObject(root, "system", system);
	return ESP_OK;
}

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

#if CONFIG_MESH_ROOT
	if (esp_mesh_lite_get_level() == 1) {
		cJSON *root = cJSON_CreateObject();
		if (root == NULL) {
			ESP_LOGE(TAG, "cJSON_CreateObject fail");
			vTaskDelete(NULL);
		}
		cJSON_AddStringToObject(root, "id", "node_information");
		cJSON_AddNumberToObject(root, "level", esp_mesh_lite_get_level());
		cJSON_AddNumberToObject(root, "seq_number", seq_number);
		cJSON_AddStringToObject(root, "self_mac", self_mac);
		cJSON_AddStringToObject(root, "parent_mac", parent_mac);
		cJSON_AddNumberToObject(root, "parent_rssi", parent_rssi);
		cJSON_AddNumberToObject(root, "free_heap", free_heap);
		cJSON_AddStringToObject(root, "target", CONFIG_IDF_TARGET);
		cJSON_AddStringToObject(root, "node_comment", "root");

		char *json_string = cJSON_PrintUnformatted(root);
		ESP_LOGI(TAG, "json_string\n%s",json_string);
		int json_length = strlen(json_string);
		ESP_LOGI(TAG, "json_length=%d", json_length);
		size_t sended = xMessageBufferSendFromISR(xMessageBufferNode, json_string, json_length, NULL);
		if (sended != json_length) {
			ESP_LOGE(TAG, "xMessageBufferSendFromISR fail json_length=%d sended=%d", json_length, sended);
			vTaskDelete(NULL);
		}
		cJSON_free(json_string);
		cJSON_Delete(root);
		seq_number++;

		if ((seq_number % 10) == 1) {
			// root object
			cJSON *root = cJSON_CreateObject();
			if (root == NULL) {
				ESP_LOGE(TAG, "cJSON_CreateObject fail");
				vTaskDelete(NULL);
			}

			ESP_ERROR_CHECK(build_system_object(root));
			cJSON_AddStringToObject(root, "id", "system_information");
			cJSON_AddStringToObject(root, "self_mac", self_mac);
			char *json_string = cJSON_PrintUnformatted(root);
			ESP_LOGI(TAG, "json_string\n%s",json_string);
			int json_length = strlen(json_string);
			ESP_LOGI(TAG, "json_length=%d", json_length);
			size_t sended = xMessageBufferSendFromISR(xMessageBufferNode, json_string, json_length, NULL);
			ESP_LOGI(TAG, "xMessageBufferSendFromISR sended=%d", sended);
			if (sended != json_length) {
				ESP_LOGE(TAG, "xMessageBufferSendFromISR fail json_length=%d sended=%d", json_length, sended);
				vTaskDelete(NULL);
			}
			cJSON_free(json_string);
			cJSON_Delete(root);
		} // end gpio_information
	}

#else
	if (esp_mesh_lite_get_level() > 1) {
		cJSON *root = cJSON_CreateObject();
		if (root == NULL) {
			ESP_LOGE(TAG, "cJSON_CreateObject fail");
			vTaskDelete(NULL);
		}
		printf("[send to root] level: %d seq_number: %"PRIu32" self_mac: [%s] parent_mac: [%s]\r\n", esp_mesh_lite_get_level(), seq_number, self_mac, parent_mac);
		cJSON_AddStringToObject(root, "id", "node_information");
		cJSON_AddNumberToObject(root, "level", esp_mesh_lite_get_level());
		cJSON_AddNumberToObject(root, "seq_number", seq_number);
		cJSON_AddStringToObject(root, "self_mac", self_mac);
		cJSON_AddStringToObject(root, "parent_mac", parent_mac);
		cJSON_AddNumberToObject(root, "parent_rssi", parent_rssi);
		cJSON_AddNumberToObject(root, "free_heap", free_heap);
		cJSON_AddStringToObject(root, "target", CONFIG_IDF_TARGET);
		cJSON_AddStringToObject(root, "node_comment", CONFIG_NODE_COMMENT);
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
		seq_number++;

		if ((seq_number % 10) == 1) {
			// root object
			cJSON *root = cJSON_CreateObject();
			if (root == NULL) {
				ESP_LOGE(TAG, "cJSON_CreateObject fail");
				vTaskDelete(NULL);
			}

			ESP_ERROR_CHECK(build_system_object(root));
			cJSON_AddStringToObject(root, "id", "system_information");
			cJSON_AddStringToObject(root, "self_mac", self_mac);
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
		} // end gpio_information
	}
#endif
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
static cJSON* json_broadcast_handler(cJSON *payload, uint32_t seq)
{
	return NULL;
}

static cJSON* json_to_root_handler(cJSON *payload, uint32_t seq)
{
	// This handler is only used by the root node.
#if CONFIG_MESH_ROOT
	cJSON *found = NULL;

	found = cJSON_GetObjectItem(payload, "id");
	char *id = found->valuestring;
	if (strcmp(id, "node_information") == 0) {
		found = cJSON_GetObjectItem(payload, "level");
		uint8_t level = found->valueint;
		found = cJSON_GetObjectItem(payload, "self_mac");
		char *self_mac = found->valuestring;
		found = cJSON_GetObjectItem(payload, "parent_mac");
		char *parent_mac = found->valuestring;
		printf("[recv from child] level: %d self_mac: [%s] parent_mac: [%s]\r\n", level, self_mac, parent_mac);
	}

	char *json_string = cJSON_PrintUnformatted(payload);
	ESP_LOGI(TAG, "json_string\n%s",json_string);
	int json_length = strlen(json_string);
	size_t sended = xMessageBufferSendFromISR(xMessageBufferNode, json_string, json_length, NULL);
	if (sended != json_length) {
		ESP_LOGE(TAG, "xMessageBufferSendFromISR fail json_length=%d sended=%d", json_length, sended);
	}
	cJSON_free(json_string);

	//esp_mesh_lite_node_info_add was updated to esp_mesh_lite_node_info_update
	//esp_mesh_lite_node_info_add(level, found->valuestring);
#endif
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

esp_err_t mountSPIFFS(char * partition_label, char * mount_point) {
	ESP_LOGI(TAG, "Initializing SPIFFS");
	esp_vfs_spiffs_conf_t conf = {
		//.base_path = "/spiffs",
		.base_path = mount_point,
		//.partition_label = NULL,
		.partition_label = partition_label,
		.max_files = 4, // maximum number of files which can be open at the same time
		//.max_files = 256,
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
	//ret = esp_spiffs_info(NULL, &total, &used);
	ret = esp_spiffs_info(partition_label, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	}
	return ret;
}

esp_err_t saveGpioTable(char * mount_point, int *gpio_table, int * gpio_count)
{
	int _gpio_count = *gpio_count;
	ESP_LOGI(TAG, "Opening file");
	char fileName[32];
	strcpy(fileName, mount_point);
	strcat(fileName, "/gpio_dump");
	FILE* fp = fopen(fileName, "w");
	if (fp == NULL) {
		ESP_LOGE(TAG, "Failed to open file for writing");
		return ESP_FAIL;
	}
	gpio_dump_io_configuration(fp, SOC_GPIO_VALID_GPIO_MASK);
	fclose(fp);
	ESP_LOGI(TAG, "File written");

	fp = fopen(fileName, "r");
	if (fp == NULL) {
		ESP_LOGE(TAG, "Failed to open file for reading");
		return ESP_FAIL;
	}
	char line[64];
	while ( fgets(line, sizeof(line), fp) != NULL ) {
		//printf("%s", line);
		if (strncmp(line, "IO[", 3) != 0) continue;
		char wk[10];
		int ofs = 0;
		for (int i=3;i<strlen(line);i++) {
			if (line[i] == ']') break;
			wk[ofs++] = line[i];
			wk[ofs] = 0;
		}
		ESP_LOGD(TAG, "wk=[%s]", wk);
		if (strstr(line, "**RESERVED**") == NULL) {
			gpio_table[_gpio_count] = atoi(wk);
			_gpio_count++;
		}
	}
	fclose(fp);
	*gpio_count = _gpio_count;
	return ESP_OK;
}

#if CONFIG_MESH_ROOT
void http_server(void *pvParameters);
#endif

void app_main()
{
	// Set the log level for serial port printing.
	esp_log_level_set("*", ESP_LOG_INFO);

	ESP_ERROR_CHECK(esp_storage_init());

	// Mount SPIFFS File System on FLASH
	char *partition_label = "storage";
	char *mount_point = "/spiffs";
	ESP_ERROR_CHECK(mountSPIFFS(partition_label, mount_point));

	// Build gpio table
	ESP_ERROR_CHECK(saveGpioTable(mount_point, gpio_table, &gpio_count));
	ESP_LOGI(TAG, "gpio_count=%d", gpio_count);
	for (int i=0;i<gpio_count;i++) {
		ESP_LOGI(TAG, "gpio_table[%d]=%d", i, gpio_table[i]);
	}

#if CONFIG_MESH_ROOT
	// Create MessageBuffer
	xMessageBufferNode = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferNode );
#endif

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	esp_bridge_create_all_netif();

	wifi_init();

	esp_mesh_lite_config_t mesh_lite_config = ESP_MESH_LITE_DEFAULT_INIT();
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

	// Start HTTP Server
	xTaskCreate(http_server, "http_server", 8 * 1024, (void *)cparam0, 5, NULL);

#endif

	TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_PERIOD_MS, true, NULL, print_system_info_timercb);
	xTimerStart(timer, 0);

	// Preventing local variables from being destroyed
	while(1) {
		vTaskDelay(1000);
	}

}
