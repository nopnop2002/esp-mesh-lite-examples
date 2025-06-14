#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/message_buffer.h"
#include "lwip/sockets.h"

#include "esp_system.h"
#include "esp_log.h"

#include "net_logging.h"

MessageBufferHandle_t xMessageBufferUDP = NULL;
bool writeToStdout;

int logging_vprintf( const char *fmt, va_list l ) {
	// Convert according to format
	char buffer[xItemSize];
	//int buffer_len = vsprintf(buffer, fmt, l);
	int buffer_len = vsnprintf(buffer, xItemSize, fmt, l);

#if 0
	xItemSize > buffer_len
	I (307) MAIN: xItemSize=20 buffer_len=11 strlen(buffer)=11
	I (307) MAIN: buffer=[76 61 6c 75 65 3a 20 31 30 30 0a]

	xItemSize = buffer_len
	I (307) MAIN: xItemSize=11 buffer_len=11 strlen(buffer)=10
	I (307) MAIN: buffer=[76 61 6c 75 65 3a 20 31 30 30 00]

	xItemSize < buffer_len
	I (307) MAIN: xItemSize=10 buffer_len=11 strlen(buffer)=9
	I (307) MAIN: buffer=[76 61 6c 75 65 3a 20 31 30 00 00]
#endif

	//printf("logging_vprintf buffer_len=%d\n",buffer_len);
	//printf("logging_vprintf buffer=[%.*s]\n", buffer_len, buffer);
	// Send MessageBuffer
	if (buffer_len > 0) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		size_t sended;
		if (xMessageBufferUDP != NULL) {
			sended = xMessageBufferSendFromISR(xMessageBufferUDP, &buffer, strlen(buffer), &xHigherPriorityTaskWoken);
			assert(sended == strlen(buffer));
		}
	}

	// Write to stdout
	if (writeToStdout) {
		return vprintf( fmt, l );
	} else {
		return 0;
	}
}

void _dump(char *id, char *data, int len)
{
  int i;
  printf("[%s]\n",id);
  for(i=0;i<len;i++) {
	printf("%0x ",data[i]);
	if ( (i % 10) == 9) printf("\n");
  }
  printf("\n");
}

// UDP Client Task
void udp_client(void *pvParameters) {
	PARAMETER_t *task_parameter = pvParameters;
	PARAMETER_t param;
	memcpy((char *)&param, task_parameter, sizeof(PARAMETER_t));
	printf("Start:param.port=%d param.ipv4=[%s] param.mac=[%s]\n", param.port, param.ipv4, param.mac);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(param.port);
	//addr.sin_addr.s_addr = htonl(INADDR_BROADCAST); /* send message to 255.255.255.255 */
	//addr.sin_addr.s_addr = inet_addr("255.255.255.255"); /* send message to 255.255.255.255 */
	addr.sin_addr.s_addr = inet_addr(param.ipv4);

	/* create the socket */
	int fd;
	int ret;
	fd = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP ); // Create a UDP socket.
	if (fd < 0) {
		printf("lwip_socket fail\n");
		vTaskDelete(NULL);
	}

	char *buffer = malloc(xItemSize);
	if (buffer == NULL) {
		printf("buffer malloc fail\n");
		vTaskDelete(NULL);
	}
	char *_buffer = malloc(xItemSize+20);
	if (_buffer == NULL) {
		printf("_buffer malloc fail\n");
		vTaskDelete(NULL);
	}

	// Send ready to receive notify
	xTaskNotifyGive(param.taskHandle);

	while(1) {
		size_t received = xMessageBufferReceive(xMessageBufferUDP, buffer, xItemSize, portMAX_DELAY);
		//printf("xMessageBufferReceive received=%d\n", received);
		if (received > 0) {
			//printf("xMessageBufferReceive buffer=[%.*s]\n",received, buffer);
			//_dump("buffer", buffer, received);
			size_t _received = received + strlen(param.mac) + 1;
			//printf("xMessageBufferReceive _received=%d",_received);
			strcpy(_buffer, param.mac);
			strcat(_buffer, ">");
			strcat(_buffer, buffer);
			//_dump("_buffer", _buffer, _received);

			ret = lwip_sendto(fd, _buffer, _received, 0, (struct sockaddr *)&addr, sizeof(addr));
			if (ret !=  _received) {
				printf("lwip_sendto fail\n");
				break;
			}
		} else {
			printf("xMessageBufferReceive fail\n");
			break;
		}
	} // end while

/*
buffer included escape code
[buffer]
1b 5b 30 3b 33 32 6d 49 20 28
36 31 32 33 29 20 4d 41 49 4e
3a 20 63 68 69 70 20 6d 6f 64
65 6c 20 69 73 20 31 2c 20 1b
5b 30 6d a

ESC [ 0 ; 3 2 m I SP (
6 1 2 3 ) SP M A I N
: SP c h i p SO m o d
e l SP i s SP 1 , SP ESC
[ 0 m LF

I (6123) MAIN: chip model is 1,

Start coloring:
ESC [ 0 ; 3 2 m
ESC [ ColorCode m
Finished coloring:
ESC [ 0 m

buffer NOT included escape code
[buffer]
3c 62 61 2d 61 64 64 3e 69 64
78 3a 31 20 28 69 66 78 3a 30
2c 20 66 38 3a 62 37 3a 39 37
3a 33 36 3a 64 65 3a 35 32 29
2c 20 74 69 64 3a 30 2c 20 73
73 6e 3a 34 2c 20 77 69 6e 53
69 7a 65 3a 36 34
*/

	// Close socket
	ret = lwip_close(fd);
	LWIP_ASSERT("ret == 0", ret == 0);
	vTaskDelete( NULL );

}

esp_err_t remote_logging_init(char * mac, bool enableStdout)
{
	// Create MessageBuffer
	xMessageBufferUDP = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferUDP );

	// Start UDP client task
	PARAMETER_t param;
	param.port = CONFIG_LOGGING_SERVER_PORT;
	strcpy(param.ipv4, CONFIG_LOGGING_SERVER_IP);
	strcpy(param.mac, mac);
	param.taskHandle = xTaskGetCurrentTaskHandle();
	xTaskCreate(udp_client, "UDP", 1024*4, (void *)&param, 2, NULL);

	// Wait for ready to receive notify
	uint32_t value = ulTaskNotifyTake( pdTRUE, pdMS_TO_TICKS(1000) );
	printf("ulTaskNotifyTake=%"PRIi32"\n", value);
	if (value == 0) {
		printf("stop remote logging\n");
		vMessageBufferDelete(xMessageBufferUDP);
		xMessageBufferUDP = NULL;
	}

	// Set function used to output log entries.
	writeToStdout = enableStdout;
	esp_log_set_vprintf(logging_vprintf);
	return ESP_OK;
}

