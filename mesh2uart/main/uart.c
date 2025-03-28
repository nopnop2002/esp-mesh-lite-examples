/*	UART asynchronous example, that uses separate RX and TX tasks

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
#include "driver/uart.h"
#include "esp_log.h"

extern MessageBufferHandle_t xMessageBufferTx;
extern MessageBufferHandle_t xMessageBufferRx;

#define BUF_SIZE 128

void uart_init(void) {
	const uart_config_t uart_config = {
		//.baud_rate = 115200,
		.baud_rate = CONFIG_UART_BAUD_RATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
		.source_clk = UART_SCLK_DEFAULT,
#else
		.source_clk = UART_SCLK_APB,
#endif
	};
	// We won't use a buffer for sending data.
	uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);
	uart_param_config(UART_NUM_1, &uart_config);
	//uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	uart_set_pin(UART_NUM_1, CONFIG_UART_TX_GPIO, CONFIG_UART_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void uart_tx(void* pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start using GPIO%d", CONFIG_UART_TX_GPIO);

	uint8_t crlf[2] = { 0x0d, 0x0a };
	char tx_buffer[BUF_SIZE+1];
	while(1) {
		size_t received = xMessageBufferReceive(xMessageBufferTx, tx_buffer, BUF_SIZE, portMAX_DELAY);
		ESP_LOGI(pcTaskGetName(NULL), "xMessageBufferReceive received=%d", received);
		if (received > 0) {
			ESP_LOGI(pcTaskGetName(NULL), "xMessageBufferReceive tx_buffer=[%.*s]",received, tx_buffer);
			int txBytes = uart_write_bytes(UART_NUM_1, tx_buffer, received);
			if (txBytes != received) {
				ESP_LOGE(pcTaskGetName(NULL), "uart_write_bytes Fail. txBytes=%d received=%d", txBytes, received);
			}
			txBytes = uart_write_bytes(UART_NUM_1, crlf, sizeof(crlf));
		}
	} // end while

	// Never reach here
	vTaskDelete(NULL);
}

void uart_rx(void* pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start using GPIO%d", CONFIG_UART_RX_GPIO);

	char rx_buffer[BUF_SIZE+1];
	char work[BUF_SIZE+1];
	int rx_offset = 0;
	while (1) {
		int received = uart_read_bytes(UART_NUM_1, work, BUF_SIZE, 10 / portTICK_PERIOD_MS);
		// There is some rxBuf in rx buffer
		if (received > 0) {
			ESP_LOGD(pcTaskGetName(NULL), "received=%d", received);
			ESP_LOGD(pcTaskGetName(NULL), "work=[%.*s]",received, work);

			for(int i=0;i<received;i++) {
				if (work[i] == 0x0d) {
					ESP_LOGI(pcTaskGetName(NULL), "rx_buffer=[%.*s]", rx_offset, rx_buffer);
					size_t sended = xMessageBufferSend(xMessageBufferRx, rx_buffer, rx_offset, 100);
					if (sended != rx_offset) {
						ESP_LOGE(pcTaskGetName(NULL), "xMessageBufferSend fail received=%d sended=%d", received, sended);
						break;
					}
					rx_offset = 0;
				} else if (work[i] == 0x0a) {

				} else {
					rx_buffer[rx_offset] = work[i];
					if (rx_offset < BUF_SIZE) rx_offset++;
				}
			}
			
		} else {
			// There is no receive data
			//ESP_LOGI(pcTaskGetName(NULL), "Read %d", rxBytes);
		}
	} // end while

	// Stop connection
	ESP_LOGI(pcTaskGetName(NULL), "Task Delete");
	vTaskDelete(NULL);
}
