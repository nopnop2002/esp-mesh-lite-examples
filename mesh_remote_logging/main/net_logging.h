#ifndef NET_LOGGING_H_
#define NET_LOGGING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"
#include "esp_log.h"

typedef struct {
	uint16_t port;
	char ipv4[20]; // xxx.xxx.xxx.xxx
	char mac[20]; // a4:cf:12:05:c6:34
	TaskHandle_t taskHandle;
} PARAMETER_t;

// The total number of bytes (not messages) the message buffer will be able to hold at any one time.
#define xBufferSizeBytes 1024
// The size, in bytes, required to hold each item in the message,
#define xItemSize 256


int logging_vprintf(const char *fmt, va_list l);
esp_err_t remote_logging_init(char * mac, bool enableStdout);

#ifdef __cplusplus
}
#endif

#endif /* NET_LOGGING_H_ */
