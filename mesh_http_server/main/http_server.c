/*	Simple HTTP Server Example

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
#include "esp_http_server.h"
#include "cJSON.h"

static const char *TAG = "SERVER";

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
	char *system_string;
} NODE_t;

// https://docs.espressif.com/projects/esp-techpedia/en/latest/esp-friends/solution-introduction/mesh/mesh-comparison.html
// Internal testing can reach 100 nodes
#define NODE_SIZE 100
NODE_t nodes[NODE_SIZE];

extern MessageBufferHandle_t xMessageBufferNode;

#define BUF_SIZE 512

int compar(const void* a, const void* b) {
	if (((NODE_t*)a)->level > ((NODE_t*)b)->level) {
		return 1; // ascending order
	} else {
		return -1; // Descending order
	}
}

/* system get handler */
static esp_err_t system_get_handler(httpd_req_t *req)
{
	ESP_LOGI(__FUNCTION__, "req->uri=[%s] req->content_len=%d", req->uri, req->content_len);
	if (req->method == HTTP_GET) {
		ESP_LOGI(__FUNCTION__, "req->method is HTTP_GET");
	}

	// convert "xx_xx_xx_xx_xx_xx" to "xx:xx:xx:xx:xx:xx"
	char self_mac[24];
	strcpy(self_mac, &req->uri[8]);
	for (int j=0;j<strlen(self_mac);j++) {
		if (self_mac[j] == '_') self_mac[j] = ':';
	}
	ESP_LOGI(__FUNCTION__, "self_mac=[%s]", self_mac);

	// Search node table
	int tableIndex = -1;
	for (int i=0;i<NODE_SIZE;i++) {
		ESP_LOGD(TAG, "nodes[%d].entry_tick=%"PRIu32, i, nodes[i].entry_tick);
		if (nodes[i].entry_tick == 0) continue;
		ESP_LOGD(TAG, "nodes[%d].self_mac=[%s]", i, nodes[i].self_mac);
		// Set last notification time
		if (strcmp(nodes[i].self_mac, self_mac) == 0) {
			tableIndex = i;
		}
	}

	// Send response
	ESP_LOGI(TAG, "tableIndex=%d", tableIndex);
	ESP_LOGI(TAG, "system_string=%p",nodes[tableIndex].system_string);
	if (tableIndex != -1 && nodes[tableIndex].system_string != NULL) {
		ESP_LOGI(TAG, "system_string=\n%s",nodes[tableIndex].system_string);
		//ESP_LOG_BUFFER_HEXDUMP(TAG, nodes[tableIndex].system_string, strlen(nodes[tableIndex].system_string), ESP_LOG_INFO);
		//httpd_resp_sendstr_chunk(req, nodes[tableIndex].system_string);

		// Convert JSON to HTML
		char chunk[256] = {0};
		for (int i=0;i<strlen(nodes[tableIndex].system_string);i++) {
			if (nodes[tableIndex].system_string[i] == 0x0a) {
				strcat(chunk, "<br>");
				ESP_LOGI(TAG, "strlen(chunk)=%d", strlen(chunk));
				ESP_LOGI(TAG, "chunk=[%s]", chunk);
				httpd_resp_sendstr_chunk(req, chunk);
				chunk[0] = 0;
			} else if (nodes[tableIndex].system_string[i] == 0x09) {
				strcat(chunk, "&nbsp;");
				strcat(chunk, "&nbsp;");
				strcat(chunk, "&nbsp;");
				strcat(chunk, "&nbsp;");
			} else {
				if (strlen(chunk) == 255) {
					ESP_LOGW(TAG, "chunk is too small");
				} else {
					strncat(chunk, &nodes[tableIndex].system_string[i], 1);
				}
			}
		}
		if (strlen(chunk) != 0) {
			httpd_resp_sendstr_chunk(req, chunk);
		}

	}

	/* Send empty chunk to signal HTTP response completion */
	httpd_resp_sendstr_chunk(req, NULL);
	return ESP_OK;
}

/* root get handler */
static esp_err_t root_get_handler(httpd_req_t *req)
{
	ESP_LOGI(__FUNCTION__, "req->uri=[%s] req->content_len=%d", req->uri, req->content_len);
	if (req->method == HTTP_GET) {
		ESP_LOGI(__FUNCTION__, "req->method is HTTP_GET");
	}

	if (strncmp(req->uri, "/system/", 8) == 0) {
		return system_get_handler(req);
	}

	// Send response
	httpd_resp_sendstr_chunk(req, "<table border=\"1\">");
	httpd_resp_sendstr_chunk(req, "<tr style=\"color:#ffffff;\" bgcolor=\"#800000\">");
	httpd_resp_sendstr_chunk(req, "<th>level</th>");
	httpd_resp_sendstr_chunk(req, "<th>seq_number</th>");
	httpd_resp_sendstr_chunk(req, "<th>self_mac</th>");
	httpd_resp_sendstr_chunk(req, "<th>parent_mac</th>");
	httpd_resp_sendstr_chunk(req, "<th>parent_rssi</th>");
	httpd_resp_sendstr_chunk(req, "<th>free heap</th>");
	httpd_resp_sendstr_chunk(req, "<th>target</th>");
	httpd_resp_sendstr_chunk(req, "<th>node comment</th>");
	httpd_resp_sendstr_chunk(req, "</tr>");

	char work[128];
	char self_mac[24];
	for (int i=0;i<NODE_SIZE;i++) {
		if (nodes[i].entry_tick == 0) continue;
		ESP_LOGI(__FUNCTION__, "nodes[%d] level=%d self_mac=[%s] parent_mac=[%s]", i, nodes[i].level, nodes[i].self_mac, nodes[i].parent_mac);
		httpd_resp_sendstr_chunk(req, "<tr>");
		sprintf(work, "<td>%d</td>", nodes[i].level);
		httpd_resp_sendstr_chunk(req, work);
		sprintf(work, "<td>%"PRIu32"</td>", nodes[i].seq_number);
		httpd_resp_sendstr_chunk(req, work);
		strcpy(self_mac, nodes[i].self_mac);
		for (int j=0;j<strlen(self_mac);j++) {
			if (self_mac[j] == ':') self_mac[j] = '_';
		}
		ESP_LOGI(TAG, "self_mac=[%s]", self_mac);
		ESP_LOGI(TAG, "system_string=%p",nodes[i].system_string);
		if (nodes[i].system_string == NULL) {
			sprintf(work, "<td>%s</td>", nodes[i].self_mac);
		} else {
			sprintf(work, "<td><a href=\"/system/%s\">%s</a></td>", self_mac, nodes[i].self_mac);
		}
		httpd_resp_sendstr_chunk(req, work);
		if (nodes[i].level == 1) {
			sprintf(work, "<td><br></td>");
		} else {
			sprintf(work, "<td>%s</td>", nodes[i].parent_mac);
		}
		httpd_resp_sendstr_chunk(req, work);
		if (nodes[i].level == 1) {
			sprintf(work, "<td><br></td>");
		} else {
			sprintf(work, "<td>%d</td>", nodes[i].parent_rssi);
		}
		httpd_resp_sendstr_chunk(req, work);
		sprintf(work, "<td>%"PRIu32"</td>", nodes[i].free_heap);
		httpd_resp_sendstr_chunk(req, work);
		sprintf(work, "<td>%s</td>", nodes[i].target);
		httpd_resp_sendstr_chunk(req, work);
		sprintf(work, "<td>%s</td>", nodes[i].node_comment);
		httpd_resp_sendstr_chunk(req, work);
		httpd_resp_sendstr_chunk(req, "</tr>");
	}
	httpd_resp_sendstr_chunk(req, "</table>");

	/* Send empty chunk to signal HTTP response completion */
	httpd_resp_sendstr_chunk(req, NULL);
	return ESP_OK;
}

/* favicon get handler */
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
	ESP_LOGI(__FUNCTION__, "favicon_get_handler");
	return ESP_OK;
}

/* Function to start the web server */
esp_err_t start_server(int port)
{
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	// Purge“"Least Recently Used” connection
	config.lru_purge_enable = true;
	// TCP Port number for receiving and transmitting HTTP traffic
	config.server_port = port;
	/* Use the URI wildcard matching function in order to
	 * allow the same handler to respond to multiple different
	 * target URIs which match the wildcard scheme */
	config.uri_match_fn = httpd_uri_match_wildcard;

	// Start the httpd server
	if (httpd_start(&server, &config) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to start file server!");
		return ESP_FAIL;
	}

	// Set URI handlers
	httpd_uri_t _root_get_handler = {
		.uri		 = "/*",
		.method		 = HTTP_GET,
		.handler	 = root_get_handler,
	};
	httpd_register_uri_handler(server, &_root_get_handler);

	httpd_uri_t _favicon_get_handler = {
		.uri		 = "/favicon.ico",
		.method		 = HTTP_GET,
		.handler	 = favicon_get_handler,
	};
	httpd_register_uri_handler(server, &_favicon_get_handler);

	return ESP_OK;
}


void http_server(void *pvParameters)
{
	char *task_parameter = (char *)pvParameters;
	ESP_LOGI(TAG, "Start task_parameter=%s", task_parameter);

	char url[64];
	int port = CONFIG_WEB_SERVER_PORT;
	sprintf(url, "http://%s:%d", task_parameter, port);
	ESP_LOGI(TAG, "Starting HTTP server on %s", url);
	ESP_ERROR_CHECK(start_server(port));

	//NODE_t nodes[NODE_SIZE];
	for (int i=0;i<NODE_SIZE;i++) {
		nodes[i].entry_tick = 0;
		nodes[i].level = 0xFF;
		nodes[i].system_string = NULL;
	}
	char buffer[BUF_SIZE+1];
	while(1) {
		size_t received = xMessageBufferReceive(xMessageBufferNode, buffer, BUF_SIZE, portMAX_DELAY);
		ESP_LOGI(TAG, "xMessageBufferReceive received=%d", received);
		ESP_LOGI(TAG, "buffer=[%.*s]", received, buffer);
		if (received == 0) {
			ESP_LOGE(TAG, "BUF_SIZE[%d] may be small", BUF_SIZE);
			break;
		}

		cJSON *root = cJSON_Parse(buffer);
		cJSON *found = NULL;
		found = cJSON_GetObjectItem(root, "id");
		char *id = found->valuestring;
		ESP_LOGI(TAG, "id=[%s]", id);

		if (strcmp(id, "node_information") == 0) {
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
				ESP_LOGD(TAG, "nodes[%d] self_mac=[%s] diff_tick=%"PRIu32, i, nodes[i].self_mac, diff_tick);
				if (diff_tick > 60000) {
					ESP_LOGW(TAG, "nodes[%s] is gone", nodes[i].self_mac);
					nodes[i].entry_tick = 0;
					nodes[i].level = 0xFF;
					cJSON_free(nodes[i].system_string);
					nodes[i].system_string = NULL;
				}
			}

			// Sort node table by level
			qsort(nodes, NODE_SIZE, sizeof(NODE_t), compar);
		} // end node_information

		if (strcmp(id, "system_information") == 0) {
			found = cJSON_GetObjectItem(root, "self_mac");
			char *self_mac = found->valuestring;
			ESP_LOGI(TAG, "self_mac=[%s]", self_mac);
			// Search node table
			for (int i=0;i<NODE_SIZE;i++) {
				ESP_LOGD(TAG, "nodes[%d].entry_tick=%"PRIu32, i, nodes[i].entry_tick);
				if (nodes[i].entry_tick == 0) continue;
				ESP_LOGI(TAG, "node[%d].self_mac=[%s]", i, nodes[i].self_mac);
				if (strcmp(nodes[i].self_mac, self_mac) == 0) {
					cJSON *system = cJSON_GetObjectItem(root, "system");;
					nodes[i].system_string = cJSON_Print(system);
					ESP_LOGI(TAG, "system_string\n%s",nodes[i].system_string);
					//cJSON_free(system_string);
					break;
				}
			}
		}

		cJSON_Delete(root);
	} // end while

	// Never reach here
	ESP_LOGI(TAG, "finish");
	vTaskDelete(NULL);
}
