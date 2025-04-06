/*	Mesh node manager
	Manages all node information within a mesh.

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
#include "freertos/message_buffer.h"

#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "NODE_MANAGER";

// node table
#define STR_SIZE 32
typedef struct {
	TickType_t entry_tick;
	uint8_t level;
	uint32_t seq_number;
	char self_mac[STR_SIZE+1];
	char parent_mac[STR_SIZE+1];
	int parent_rssi;
	uint32_t free_heap;
	char target[STR_SIZE+1];
	char node_comment[STR_SIZE+1];
	cJSON *json;
} NODE_t;

// https://docs.espressif.com/projects/esp-techpedia/en/latest/esp-friends/solution-introduction/mesh/mesh-comparison.html
// Internal testing can reach 100 nodes
#define NODE_SIZE 100
NODE_t nodes[NODE_SIZE];

extern MessageBufferHandle_t xMessageBufferNodeManager;
extern MessageBufferHandle_t xMessageBufferJsonServer;
//extern MessageBufferHandle_t xMessageBufferToClient;

#define BUF_SIZE 256
#define JSON_SIZE 512

int compar(const void* a, const void* b) {
	if (((NODE_t*)a)->level > ((NODE_t*)b)->level) {
		return 1; // ascending order
	} else {
		return -1; // Descending order
	}
}

static void print_node_info_timercb(TimerHandle_t timer)
{
	for (int i=0;i<NODE_SIZE;i++) {
		if (nodes[i].entry_tick == 0) continue;
		ESP_LOGI("NODES", "nodes[%d] level=%d self_mac=[%s] parent_mac=[%s]", i, nodes[i].level, nodes[i].self_mac, nodes[i].parent_mac);
	}
}

void node_manager(void *pvParameters)
{
	ESP_LOGI(TAG, "Start");
	esp_log_level_set(TAG, ESP_LOG_WARN);

	// Initialize node table
	for (int i=0;i<NODE_SIZE;i++) {
		nodes[i].entry_tick = 0;
		nodes[i].level = 0xFF;
		nodes[i].json = NULL;
	}

	// Initialize json string
	char previus_json_string[JSON_SIZE+1] = {0};

	// Start timer
	TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_PERIOD_MS, true, NULL, print_node_info_timercb);
	xTimerStart(timer, 0);

	char buffer[BUF_SIZE+1];
	while(1) {
		size_t received = xMessageBufferReceive(xMessageBufferNodeManager, buffer, BUF_SIZE, portMAX_DELAY);
		ESP_LOGI(TAG, "xMessageBufferReceive received=%d", received);
		ESP_LOGI(TAG, "buffer=[%.*s]", received, buffer);
		if (received == BUF_SIZE) {
			ESP_LOGW(TAG, "BUF_SIZE[%d] may be small", BUF_SIZE);
		}

		cJSON *root = cJSON_Parse(buffer);
		cJSON *found = NULL;

		found = cJSON_GetObjectItem(root, "level");
		uint8_t level = found->valueint;
		found = cJSON_GetObjectItem(root, "seq_number");
		uint32_t seq_number = found->valueint;
		found = cJSON_GetObjectItem(root, "self_mac");
		char *self_mac = found->valuestring;
		found = cJSON_GetObjectItem(root, "parent_mac");
		char *parent_mac = found->valuestring;
		found = cJSON_GetObjectItem(root, "parent_rssi");
		int parent_rssi= found->valueint;
		found = cJSON_GetObjectItem(root, "free_heap");
		uint32_t free_heap= found->valueint;
		found = cJSON_GetObjectItem(root, "target");
		char *target = found->valuestring;
		found = cJSON_GetObjectItem(root, "node_comment");
		char *node_comment = found->valuestring;
		ESP_LOGD(TAG, "level=%d seq_number=%"PRIu32" self_mac=[%s] parent_mac=[%s]", level, seq_number, self_mac, parent_mac);

		// Search node table
		int tableIndex = -1;
		for (int i=0;i<NODE_SIZE;i++) {
			ESP_LOGD(TAG, "nodes[%d].entry_tick=%"PRIu32, i, nodes[i].entry_tick);
			if (nodes[i].entry_tick == 0) continue;
			ESP_LOGD(TAG, "nodes[%d] level=%d self_mac=[%s] parent_mac=[%s]", i, nodes[i].level, nodes[i].self_mac, nodes[i].parent_mac);
			// Set last notification time
			if (strcmp(nodes[i].self_mac, self_mac) == 0) {
				nodes[i].level = level;
				nodes[i].entry_tick = xTaskGetTickCount();
				nodes[i].seq_number = seq_number;
				strncpy(nodes[i].parent_mac, parent_mac, STR_SIZE);
				nodes[i].free_heap = free_heap;
				tableIndex = i;
			}
		}

		// Add a new node to the node table
		ESP_LOGI(TAG, "tableIndex=%d", tableIndex);
		// Determine the last notification time
		if (tableIndex == -1) {
			for (int i=0;i<NODE_SIZE;i++) {
				if (nodes[i].entry_tick != 0) continue;
				nodes[i].entry_tick = xTaskGetTickCount();
				nodes[i].level = level;
				nodes[i].seq_number = seq_number;
				strncpy(nodes[i].self_mac, self_mac, STR_SIZE);
				strncpy(nodes[i].parent_mac, parent_mac, STR_SIZE);
				nodes[i].parent_rssi = parent_rssi;
				nodes[i].free_heap = free_heap;
				strncpy(nodes[i].target, target, STR_SIZE);
				strncpy(nodes[i].node_comment, node_comment, STR_SIZE);
				break;
			}
		}

		// Remove missing nodes from the node table
		TickType_t current_tick = xTaskGetTickCount();
		for (int i=0;i<NODE_SIZE;i++) {
			if (nodes[i].entry_tick == 0) continue;
			TickType_t diff_tick = current_tick - nodes[i].entry_tick;
			ESP_LOGI(TAG, "nodes[%d] level=%d self_mac=[%s] parent_mac=[%s] diff_tick=%"PRIu32,
				i, nodes[i].level, nodes[i].self_mac, nodes[i].parent_mac, diff_tick);
			if (diff_tick > 60000) {
				ESP_LOGW(TAG, "nodes[%s] is gone", nodes[i].self_mac);
				nodes[i].entry_tick = 0;
				nodes[i].level = 0xFF;
			}
		}
		cJSON_Delete(root);

		// Sort node table by level
		qsort(nodes, NODE_SIZE, sizeof(NODE_t), compar);
#if 0
		esp_log_level_set(TAG, ESP_LOG_INFO);
		for (int i=0;i<NODE_SIZE;i++) {
			if (nodes[i].entry_tick == 0) continue;
			ESP_LOGI(TAG, "nodes[%d] level=%d self_mac=[%s] parent_mac=[%s]", i, nodes[i].level, nodes[i].self_mac, nodes[i].parent_mac);
		}
		esp_log_level_set(TAG, ESP_LOG_WARN);
#endif


		// Get the number of nodes
		int node_count = 0;
		int root_node = -1;
		for (int i=0;i<NODE_SIZE;i++) {
			if (nodes[i].entry_tick == 0) continue;
			node_count++;
			if (nodes[i].level == 1) root_node = i;
		}

		// There is no root node
		ESP_LOGI(TAG, "node_count=%d root_node=%d", node_count, root_node);
		if (root_node == -1) continue;

		// Build tree format json
		for (int target_level=10;target_level>0;target_level--) {
			for (int i=0;i<NODE_SIZE;i++) {
				if (nodes[i].entry_tick == 0) continue;
				if (nodes[i].level != target_level) continue;
				nodes[i].json = cJSON_CreateObject();
				ESP_LOGI(TAG, "target_level=%d nodes[%d] self_mac=[%s] parent_mac=[%s]",
					target_level, i, nodes[i].self_mac, nodes[i].parent_mac);
				cJSON_AddStringToObject(nodes[i].json, "mac", nodes[i].self_mac);
				cJSON_AddStringToObject(nodes[i].json, "comment", nodes[i].node_comment);

				for (int j=0;j<NODE_SIZE;j++) {
					if (nodes[j].entry_tick == 0) continue;
					if (strcmp(nodes[i].self_mac, nodes[j].parent_mac) != 0) continue;
					ESP_LOGI(TAG, "nodes[%d] has child node. self_mac=[%s]", j, nodes[j].self_mac);
					cJSON *array;
					array = cJSON_CreateArray();
					for (int k=0;k<NODE_SIZE;k++) {
						if (nodes[k].entry_tick == 0) continue;
						if (strcmp(nodes[i].self_mac, nodes[k].parent_mac) != 0) continue;
						cJSON_AddItemToArray(array, nodes[k].json);
					}
					cJSON_AddItemToObject(nodes[i].json, "child", array);
					break;
				}
	
			}
		}

		// print json string
		char *now_json_string = cJSON_PrintUnformatted(nodes[root_node].json);
		if (strlen(now_json_string) > JSON_SIZE) {
			ESP_LOGE(TAG, "JSON_SIZE [%d] may be small", JSON_SIZE);
			break;
		}

		ESP_LOGD(TAG, "now_json_string\n-%s-",now_json_string);
		ESP_LOGD(TAG, "previus_json_string\n-%s-",previus_json_string);
		if (strcmp(previus_json_string, now_json_string) != 0) {
			strcpy(previus_json_string, now_json_string);
			ESP_LOGW(TAG, "previus_json_string\n-%s-",previus_json_string);

			cJSON *root = cJSON_CreateObject();
			cJSON_AddStringToObject(root, "id", "save");
			cJSON_AddStringToObject(root, "json", previus_json_string);
			char *my_json_string = cJSON_Print(root);
			ESP_LOGD(TAG, "my_json_string=%d bytes", strlen(my_json_string));
			ESP_LOGD(TAG, "\n[%s]",my_json_string);
			//size_t xBytesSent = xMessageBufferSend(xMessageBufferToClient, my_json_string, strlen(my_json_string), portMAX_DELAY);
			size_t xBytesSent = xMessageBufferSend(xMessageBufferJsonServer, my_json_string, strlen(my_json_string), portMAX_DELAY);
			if (xBytesSent != strlen(my_json_string)) {
				ESP_LOGE(TAG, "xMessageBufferSend fail");
				break;
			}

		}
		cJSON_free(now_json_string);
		cJSON_Delete(nodes[root_node].json);


	}

	// Some error occurred
	ESP_LOGE(TAG, "finish");
	vTaskDelete(NULL);
}
