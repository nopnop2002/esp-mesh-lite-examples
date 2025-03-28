/*	USB asynchronous example, that uses separate RX callback and TX tasks

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
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "esp_log.h"

extern MessageBufferHandle_t xMessageBufferTx;
extern MessageBufferHandle_t xMessageBufferRx;

#define BUF_SIZE 128

void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event)
{
	static uint8_t rx_buffer[BUF_SIZE+1];
	static int rx_offset = 0;

	size_t received;
	ESP_LOGD(pcTaskGetName(NULL), "CONFIG_TINYUSB_CDC_RX_BUFSIZE=%d", CONFIG_TINYUSB_CDC_RX_BUFSIZE);
	uint8_t work[CONFIG_TINYUSB_CDC_RX_BUFSIZE+1];
	esp_err_t ret = tinyusb_cdcacm_read(itf, work, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &received);
	if (ret == ESP_OK) {
		ESP_LOGD(pcTaskGetName(NULL), "Data from channel=%d received=%d", itf, received);

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
		ESP_LOGE(pcTaskGetName(NULL), "tinyusb_cdcacm_read error");
	}
}

void usb_init(void) {
	const tinyusb_config_t tusb_cfg = {
		.device_descriptor = NULL,
		.string_descriptor = NULL,
		.external_phy = false,
		.configuration_descriptor = NULL,
	};
	ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

	tinyusb_config_cdcacm_t acm_cfg = {
		.usb_dev = TINYUSB_USBDEV_0,
		.cdc_port = TINYUSB_CDC_ACM_0,
		.rx_unread_buf_sz = 64,
		.callback_rx = &tinyusb_cdc_rx_callback, // the first way to register a callback
		.callback_rx_wanted_char = NULL,
		//.callback_line_state_changed = &tinyusb_cdc_line_state_changed_callback,
		.callback_line_coding_changed = NULL
	};
	ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
}

void tusb_tx(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");

	uint8_t crlf[2] = { 0x0d, 0x0a };
	uint8_t tx_buffer[CONFIG_TINYUSB_CDC_RX_BUFSIZE+1];
	while (1) {
		//Waiting for UART transmit event.
		size_t received = xMessageBufferReceive(xMessageBufferTx, tx_buffer, CONFIG_TINYUSB_CDC_RX_BUFSIZE, portMAX_DELAY);
		ESP_LOGI(pcTaskGetName(NULL), "xMessageBufferReceive received=%d", received);
		if (received > 0) {
			ESP_LOGI(pcTaskGetName(NULL), "xMessageBufferReceive tx_buffer=[%.*s]",received, tx_buffer);
			tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, tx_buffer, received);
			tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, crlf, 2);
			tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
		}
	}
}
