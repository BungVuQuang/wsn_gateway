#ifndef __WIFI_CONFIG_H
#define __WIFI_CONFIG_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "app_http_server.h"
#include "esp_smartconfig.h"
// #define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

extern int WIFI_RECV_INFO;
extern int WIFI_CONNECTED_BIT;
extern int WIFI_FAIL_BIT;
// static const int CONNECTED_BIT = BIT3;

// int WIFI_RECV_INFO=BIT0;
// int WIFI_CONNECTED_BIT=BIT1;
// int WIFI_FAIL_BIT =BIT2;
// //static const int CONNECTED_BIT = BIT3;
// int ESPTOUCH_DONE_BIT=BIT3;

extern EventGroupHandle_t s_wifi_event_group;
extern void wifi_data_callback(char *data, int len);
extern void wifi_init_sta(void);
void initialise_wifi(void);
void wifi_init_softap(void);
void wifi_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data);
#endif