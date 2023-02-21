#include <stdio.h>
#include <string.h>

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_ble_mesh_sensor_model_api.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_timer.h"
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_local_data_operation_api.h"
#include "ble_mesh_example_init.h"

#include "ble_mesh_example_nvs.h"
#include "board.h"
#include "esp_netif.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_smartconfig.h"
#include "esp_wifi.h"
#include "mdns.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "app_http_server.h"
#include "wifi_connecting.h"
#include "json_generator.h"
#include "json_parser.h"
#define TAG "Client"

//===============================  MQTT HTTP VARIABLE=================
#define WEB_SERVER "api.thingspeak.com"
#define WEB_PORT "80"
static const char *TAG_MQTT = "MQTT:";
static esp_mqtt_client_handle_t client = NULL;
static TaskHandle_t loopHandle = NULL;
char REQUEST[512];
char SUB_REQUEST[100];
char recv_buf[512];
int WIFI_CONNECTED_BIT = BIT0;
int MESSAGE_ARRIVE_BIT = BIT1;
int WIFI_RECV_INFO = BIT2;
int WIFI_FAIL_BIT = BIT4;
char data_temp2[15];
char *cluster_temp2;
char *temper_temp2;
char *batt_temp2;
char data_temp[50];
uint8_t rx_node = 0;
static int s_retry_num = 0;
typedef enum
{
    INITIAL_STATE,
    NORMAL_STATE,
    LOST_WIFI_STATE,
    CHANGE_PASSWORD_STATE,

} wifi_state_t;

struct wifi_info_t // struct gồm các thông tin của wifi
{
    char SSID[20];
    char PASSWORD[10];
    wifi_state_t state;
} __attribute__((packed)) wifi_info = {
    .SSID = "FPT",
    .PASSWORD = "toanluong",
    .state = INITIAL_STATE,
};
//================================BLE CLIENT VARIABLE====================
char data_rx_mqtt[60];
uint8_t rx_mess_check = 0;
uint8_t rx_mess_cluster = 0;
uint8_t rx_mess_node = 0;
char data_tx_ble[20];
//================================JSON MESSAGE VARIABLE====================
typedef struct
{
    char buf[256];
    size_t offset;
} json_gen_test_result_t;

static json_gen_test_result_t result;

typedef struct
{
    char cluster;
    char node;
    char temperature[5];
    char mode[8];
} data_setup_t;

static data_setup_t data_setup;
//=======================================================================
enum provision_type_t
{
    PROV_NONE = -1,
    PROV_SMART_CONFIG = 0,
    PROV_ACCESS_POINT = 1
} provision_type_t;
enum provision_type_t provision_type = PROV_NONE;
EventGroupHandle_t s_wifi_event_group;
EventGroupHandle_t s_mqtt_event_group;
#define CID_ESP 0x02E5
#define TEMPERATURE_GROUP_ADDRESS 0xC001
#define PROV_OWN_ADDR 0x0001
#define SUBSCRIPTION_ADDR 0xC100
#define MSG_SEND_TTL 3
#define MSG_SEND_REL false
#define MSG_TIMEOUT 0
#define MSG_ROLE ROLE_PROVISIONER

char data_ack[5] = "ACK";
static uint16_t sensor_prop_id;
#define COMP_DATA_PAGE_0 0x00
static esp_ble_mesh_client_t sensor_client;
#define APP_KEY_IDX 0x0000
#define APP_KEY_OCTET 0x12

#define COMP_DATA_1_OCTET(msg, offset) (msg[offset])
#define COMP_DATA_2_OCTET(msg, offset) (msg[offset + 1] << 8 | msg[offset])

#define ESP_BLE_MESH_VND_MODEL_ID_CLIENT 0x0000
#define ESP_BLE_MESH_VND_MODEL_ID_SERVER 0x0001
#define ESP_BLE_MESH_VND_MODEL_ID_TEMPERATURE_CLIENT 0x0002
#define ESP_BLE_MESH_VND_MODEL_OP_SEND ESP_BLE_MESH_MODEL_OP_3(0x00, CID_ESP)
#define ESP_BLE_MESH_VND_MODEL_OP_STATUS ESP_BLE_MESH_MODEL_OP_3(0x01, CID_ESP)

static uint8_t dev_uuid[ESP_BLE_MESH_OCTET16_LEN];
uint8_t temp = 0;
static struct example_info_store
{
    uint16_t server_addr; /* Vendor server unicast address */
    uint16_t vnd_tid;     /* TID contained in the vendor message */
} store = {
    .server_addr = ESP_BLE_MESH_ADDR_UNASSIGNED,
    .vnd_tid = 0,
};

static struct example_info_store_2
{
    uint16_t server_addr; /* Vendor server unicast address */
    uint16_t vnd_tid;     /* TID contained in the vendor message */
} store2 = {
    .server_addr = ESP_BLE_MESH_ADDR_UNASSIGNED,
    .vnd_tid = 0,
};

static struct example_info_store_3
{
    uint16_t server_addr; /* Vendor server unicast address */
    uint16_t vnd_tid;     /* TID contained in the vendor message */
} store3 = {
    .server_addr = ESP_BLE_MESH_ADDR_UNASSIGNED,
    .vnd_tid = 0,
};
static struct example_info_store_4
{
    uint16_t server_addr; /* Vendor server unicast address */
    uint16_t vnd_tid;     /* TID contained in the vendor message */
} store4 = {
    .server_addr = ESP_BLE_MESH_ADDR_UNASSIGNED,
    .vnd_tid = 0,
};
static struct example_info_store_5
{
    uint16_t server_addr; /* Vendor server unicast address */
    uint16_t vnd_tid;     /* TID contained in the vendor message */
} store5 = {
    .server_addr = ESP_BLE_MESH_ADDR_UNASSIGNED,
    .vnd_tid = 0,
};
static struct example_info_store_6
{
    uint16_t server_addr; /* Vendor server unicast address */
    uint16_t vnd_tid;     /* TID contained in the vendor message */
} store6 = {
    .server_addr = ESP_BLE_MESH_ADDR_UNASSIGNED,
    .vnd_tid = 0,
};
static struct example_info_store_7
{
    uint16_t server_addr; /* Vendor server unicast address */
    uint16_t vnd_tid;     /* TID contained in the vendor message */
} store7 = {
    .server_addr = ESP_BLE_MESH_ADDR_UNASSIGNED,
    .vnd_tid = 0,
};
static struct example_info_store_8
{
    uint16_t server_addr; /* Vendor server unicast address */
    uint16_t vnd_tid;     /* TID contained in the vendor message */
} store8 = {
    .server_addr = ESP_BLE_MESH_ADDR_UNASSIGNED,
    .vnd_tid = 0,
};

static struct example_info_store_9
{
    uint16_t server_addr; /* Vendor server unicast address */
    uint16_t vnd_tid;     /* TID contained in the vendor message */
} store9 = {
    .server_addr = ESP_BLE_MESH_ADDR_UNASSIGNED,
    .vnd_tid = 0,
};

static struct example_info_store_10
{
    uint16_t server_addr; /* Vendor server unicast address */
    uint16_t vnd_tid;     /* TID contained in the vendor message */
} store10 = {
    .server_addr = ESP_BLE_MESH_ADDR_UNASSIGNED,
    .vnd_tid = 0,
};
static nvs_handle_t my_handler;
static const char *NVS_KEY_WIFI = "WIFI";
static nvs_handle_t NVS_HANDLE;
static const char *NVS_KEY = "Node1";
static const char *NVS_KEY2 = "Node2";
static const char *NVS_KEY3 = "Node3";
static const char *NVS_KEY4 = "Node4";
static const char *NVS_KEY5 = "Node5";
static const char *NVS_KEY6 = "Node6";
static const char *NVS_KEY7 = "Node7";
static const char *NVS_KEY8 = "Node8";
static const char *NVS_KEY9 = "Node9";
static const char *NVS_KEY10 = "Node10";
static struct esp_ble_mesh_key
{
    uint16_t net_idx;
    uint16_t app_idx;
    uint8_t app_key[ESP_BLE_MESH_OCTET16_LEN];
} prov_key;

static esp_ble_mesh_cfg_srv_t config_server = {
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
#if defined(CONFIG_BLE_MESH_FRIEND)
    .friend_state = ESP_BLE_MESH_FRIEND_ENABLED,
#else
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
#endif
    .default_ttl = 7,
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
};

static esp_ble_mesh_client_t config_client;

static const esp_ble_mesh_client_op_pair_t vnd_op_pair[] = {
    {ESP_BLE_MESH_VND_MODEL_OP_SEND, ESP_BLE_MESH_VND_MODEL_OP_STATUS},
};

static esp_ble_mesh_client_t vendor_client = {
    .op_pair_size = ARRAY_SIZE(vnd_op_pair),
    .op_pair = vnd_op_pair,
};
uint8_t temp_pro = 1;
// static esp_ble_mesh_model_op_t vnd_op[] = {
//     ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_STATUS, 0),
//     ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_STATUS, 0),
//     ESP_BLE_MESH_MODEL_OP_END,
// };
static esp_ble_mesh_model_op_t vnd_op[] = {
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_STATUS, 2),
    ESP_BLE_MESH_MODEL_OP_END,
};
static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_CFG_CLI(&config_client),
    ESP_BLE_MESH_MODEL_SENSOR_CLI(NULL, &sensor_client),
};

// static esp_ble_mesh_model_t root_models2[] = {
//     ESP_BLE_MESH_MODEL_SENSOR_CLI(NULL, &sensor_client),
// };

static esp_ble_mesh_model_t vnd_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_CLIENT,
                              vnd_op, NULL, &vendor_client),

};
void example_ble_mesh_send_sensor_message(uint32_t opcode);
static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, vnd_models),
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .elements = elements,
    .element_count = ARRAY_SIZE(elements),
};

static esp_ble_mesh_prov_t provision = {
    .prov_uuid = dev_uuid,
    .prov_unicast_addr = PROV_OWN_ADDR,
    .prov_start_address = 0x0005,
};

int json_parse_data_setup(char *json, data_setup_t *out_data);

static void flush_str(char *buf, void *priv)
{
    json_gen_test_result_t *result = (json_gen_test_result_t *)priv;
    if (result)
    {
        if (strlen(buf) > sizeof(result->buf) - result->offset)
        {
            printf("Result Buffer too small\r\n");
            return;
        }
        memcpy(result->buf + result->offset, buf, strlen(buf));
        result->offset += strlen(buf);
    }
}
/**
 *  void nvs_save_wifiInfo(nvs_handle_t c_handle, const char *key, void *out_value)
 *  @brief lưu lại thông tin wifi từ nvs
 *
 *  @param[in] c_handle nvs_handle_t
 *  @param[in] key Key để lấy dữ liệu
 *  @param[in] value Data output
 *  @param[in] length chiều dài dữ liệu
 *  @return None
 */
void nvs_save_wifiInfo(nvs_handle_t c_handle, const char *key, const void *value, size_t length)
{
    esp_err_t err;
    nvs_open("storage0", NVS_READWRITE, &c_handle);
    // strcpy(wifi_info.SSID, "anhbung");
    nvs_set_blob(c_handle, key, value, length);
    err = nvs_commit(c_handle);
    if (err != ESP_OK)
        return err;

    // Close
    nvs_close(c_handle);
}
/**
 *  void nvs_get_wifiInfo(nvs_handle_t c_handle, const char *key, void *out_value)
 *  @brief Lấy lại trong tin wifi từ nvs
 *
 *  @param[in] c_handle nvs_handle_t
 *  @param[in] key Key để lấy dữ liệu
 *  @param[in] out_value Data output
 *  @return None
 */
void nvs_get_wifiInfo(nvs_handle_t c_handle, const char *key, void *out_value) // lấy thông tin wifi
{
    esp_err_t err;
    err = nvs_open("storage0", NVS_READWRITE, &c_handle);
    if (err != ESP_OK)
        return err;
    size_t required_size = 0; // value will default to 0, if not set yet in NVS
    err = nvs_get_blob(c_handle, NVS_KEY_WIFI, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        return err;
    if (required_size == 0)
    {
        printf("Nothing saved yet!\n");
    }
    else
    {
        nvs_get_blob(c_handle, NVS_KEY_WIFI, out_value, &required_size);

        err = nvs_commit(c_handle);
        if (err != ESP_OK)
            return err;

        // Close
        nvs_close(c_handle);
    }
}
/**
 *  void wifi_data_callback(char *data, int len)
 *  @brief Hàm này được gọi lại mỗi khi nhận được dữ liệu wifi local
 *
 *  @param[in] data dữ liệu
 *  @param[in] len chiều dài dữ liệu
 *  @return None
 */
void wifi_data_callback(char *data, int len) // ham nay xu ly thong tin wifi cau hinh ban dau
{
    char data_wifi[30];
    sprintf(data_wifi, "%.*s", len, data);
    printf("%.*s", len, data);
    char *pt = strtok(data_wifi, "/");
    strcpy(wifi_info.SSID, pt);
    pt = strtok(NULL, "/");
    strcpy(wifi_info.PASSWORD, pt);
    printf("\nssid: %s \n pass: %s\n", wifi_info.SSID, wifi_info.PASSWORD);
    nvs_save_wifiInfo(my_handler, NVS_KEY_WIFI, &wifi_info, sizeof(wifi_info)); // luu lai
    xEventGroupSetBits(s_wifi_event_group, WIFI_RECV_INFO);                     // set cờ
}

/**
 *  @brief Lưu lại thông tin cac node
 *
 *  @param[in] node index của node
 *  @return None
 */
static void mesh_example_info_store(uint8_t node)
{
    if (node == 1)
    {
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY, &store, sizeof(store));
    }
    else if (node == 2)
    {
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY2, &store2, sizeof(store2));
    }
    else if (node == 3)
    {
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY3, &store3, sizeof(store3));
    }
    else if (node == 4)
    {
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY4, &store4, sizeof(store4));
    }
    else if (node == 5)
    {
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY5, &store5, sizeof(store5));
    }
    else if (node == 6)
    {
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY6, &store6, sizeof(store6));
    }
    else if (node == 7)
    {
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY7, &store7, sizeof(store7));
    }
    else if (node == 8)
    {
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY8, &store8, sizeof(store8));
    }
    else if (node == 9)
    {
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY9, &store9, sizeof(store9));
    }
    else if (node == 10)
    {
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY10, &store10, sizeof(store10));
    }
}

/**
 *  @brief khôi phục lại thông tin cac node
 *
 *  @return None
 */
static void mesh_example_info_restore(void)
{
    esp_err_t err = ESP_OK;
    bool exist = false;

    ble_mesh_nvs_restore(NVS_HANDLE, NVS_KEY, &store, sizeof(store), &exist);
    ble_mesh_nvs_restore(NVS_HANDLE, NVS_KEY2, &store2, sizeof(store2), &exist);
    ble_mesh_nvs_restore(NVS_HANDLE, NVS_KEY3, &store3, sizeof(store3), &exist);
    ble_mesh_nvs_restore(NVS_HANDLE, NVS_KEY4, &store4, sizeof(store4), &exist);
    ble_mesh_nvs_restore(NVS_HANDLE, NVS_KEY5, &store5, sizeof(store5), &exist);
    ble_mesh_nvs_restore(NVS_HANDLE, NVS_KEY6, &store6, sizeof(store6), &exist);
    ble_mesh_nvs_restore(NVS_HANDLE, NVS_KEY7, &store7, sizeof(store7), &exist);
    ble_mesh_nvs_restore(NVS_HANDLE, NVS_KEY8, &store8, sizeof(store8), &exist);
    ble_mesh_nvs_restore(NVS_HANDLE, NVS_KEY9, &store9, sizeof(store9), &exist);
    ble_mesh_nvs_restore(NVS_HANDLE, NVS_KEY10, &store10, sizeof(store10), &exist);
}

int64_t start_time;
/**
 *  @brief Task này thực hiện Publish bản tin khi có tin nhắn gửi đến
 *
 *  @param[in] pvParameters tham số truyền vào khi tạo task
 *  @return ESP_OK
 */
static void xTaskTransmitMQTT(void *pvParameters)
{
    int msg_id;
    while (1)
    {
        xEventGroupWaitBits(s_mqtt_event_group, // đợi đến khi có tin nhắn từ node gửi đến
                            MESSAGE_ARRIVE_BIT,
                            pdFALSE,
                            pdFALSE,
                            portMAX_DELAY);
        start_time = esp_timer_get_time();
        if (strstr(cluster_temp2, "1") != NULL)
        {
            switch (rx_node)
            {
            case 1:
                sprintf(data_temp, "{\"Cluster\":\"%s\",\"Node1\":\"%s\", \"Battery1\":\"%s\"}", cluster_temp2, temper_temp2, batt_temp2);
                msg_id = esp_mqtt_client_publish(client, "/smarthome/devices", data_temp, 0, 1, 0);
                ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
                break;
            case 2:
                sprintf(data_temp, "{\"Cluster\":\"%s\",\"Node2\":\"%s\", \"Battery2\":\"%s\"}", cluster_temp2, temper_temp2, batt_temp2);
                msg_id = esp_mqtt_client_publish(client, "/smarthome/devices", data_temp, 0, 1, 0);
                ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
                break;
            case 3:
                sprintf(data_temp, "{\"Cluster\":\"%s\",\"Node3\":\"%s\", \"Battery3\":\"%s\"}", cluster_temp2, temper_temp2, batt_temp2);
                msg_id = esp_mqtt_client_publish(client, "/smarthome/devices", data_temp, 0, 1, 0);
                ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
                break;
            case 4:
                sprintf(data_temp, "{\"Cluster\":\"%s\",\"Node4\":\"%s\", \"Battery4\":\"%s\"}", cluster_temp2, temper_temp2, batt_temp2);
                msg_id = esp_mqtt_client_publish(client, "/smarthome/devices", data_temp, 0, 1, 0);
                ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
                break;
            case 5:
                sprintf(data_temp, "{\"Cluster\":\"%s\",\"Node5\":\"%s\", \"Battery5\":\"%s\"}", cluster_temp2, temper_temp2, batt_temp2);
                msg_id = esp_mqtt_client_publish(client, "/smarthome/devices", data_temp, 0, 1, 0);
                ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
                break;
            default:
                break;
            }
        }
        else
        {
            switch (rx_node)
            {
            case 1:
                sprintf(data_temp, "{\"Cluster\":\"%s\",\"Node1\":\"%s\", \"Battery1\":\"%s\"}", cluster_temp2, temper_temp2, batt_temp2);
                msg_id = esp_mqtt_client_publish(client, "/smarthome/devices", data_temp, 0, 1, 0);
                ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
                break;
            case 2:
                sprintf(data_temp, "{\"Cluster\":\"%s\",\"Node2\":\"%s\", \"Battery2\":\"%s\"}", cluster_temp2, temper_temp2, batt_temp2);
                msg_id = esp_mqtt_client_publish(client, "/smarthome/devices", data_temp, 0, 1, 0);
                ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
                break;
            case 3:
                sprintf(data_temp, "{\"Cluster\":\"%s\",\"Node3\":\"%s\", \"Battery3\":\"%s\"}", cluster_temp2, temper_temp2, batt_temp2);
                msg_id = esp_mqtt_client_publish(client, "/smarthome/devices", data_temp, 0, 1, 0);
                ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
                break;
            case 4:
                sprintf(data_temp, "{\"Cluster\":\"%s\",\"Node4\":\"%s\", \"Battery4\":\"%s\"}", cluster_temp2, temper_temp2, batt_temp2);
                msg_id = esp_mqtt_client_publish(client, "/smarthome/devices", data_temp, 0, 1, 0);
                ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
                break;
            case 5:
                sprintf(data_temp, "{\"Cluster\":\"%s\",\"Node5\":\"%s\", \"Battery5\":\"%s\"}", cluster_temp2, temper_temp2, batt_temp2);
                msg_id = esp_mqtt_client_publish(client, "/smarthome/devices", data_temp, 0, 1, 0);
                ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
                break;
            default:
                break;
            }
        }
        xEventGroupClearBits(s_mqtt_event_group, MESSAGE_ARRIVE_BIT); // xoá Eventbit
        int64_t end_time = esp_timer_get_time();
        ESP_LOGI(TAG, "time %lldus",
                 end_time - start_time);
        vTaskDelay(500 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}
static inline void Provision_Handler(int addr);
char ssid_remote[15];
char password_remote[15];
char num_delete[5];
int unicast_delete;
/**
 *  @brief Xử lý các sự kiện từ Mqtt
 *
 *  @param[in] event Data về event được gửi đến
 *  @return ESP_OK
 */
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    char *ptr;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:

        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, "/Temperature/Setup", 1); // sub topic
        ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA");
        sprintf(data_rx_mqtt, "%.*s", event->data_len, event->data);
        if (strstr(data_rx_mqtt, "Node") != NULL) // tin nhắn thay đổi ngưỡng led
        {
            json_parse_data_setup(data_rx_mqtt, &data_setup);
            sprintf(data_tx_ble, "%s %s", data_setup.mode, data_setup.temperature);
            printf("Data ble tx: %s\r\n", data_tx_ble);
            rx_mess_check = 1;
        }
        else if (strstr(data_rx_mqtt, "Delete") != NULL) // tin nhắn xoá 1 node khỏi mạng
        {
            strtok(data_rx_mqtt, " ");
            ptr = strtok(NULL, " ");
            sprintf(num_delete, "%s", ptr);
            unicast_delete = 4 + atoi(num_delete);
            Provision_Handler(unicast_delete);
        }
        else
        {
            ptr = strtok(data_rx_mqtt, " ");
            sprintf(ssid_remote, "%s", ptr);
            ptr = strtok(NULL, " ");
            sprintf(password_remote, "%s", ptr);
            strcpy(wifi_info.SSID, ssid_remote);
            strcpy(wifi_info.PASSWORD, password_remote);
            nvs_save_wifiInfo(my_handler, NVS_KEY_WIFI, &wifi_info, sizeof(wifi_info)); // lưu lại thông tin wifi
        }
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    default:
        ESP_LOGI(TAG_MQTT, "Other event id:%d", event->event_id);
        break;
    }

    return ESP_OK;
}
/**
 *  @brief Hàm này thực hiện xoá 1 node ra khỏi mạng
 *
 *  @param[in] addr Unicast address
 *  @return none
 */
static inline void Provision_Handler(int addr)
{
    printf("Da xoa node %d\n", addr);
    esp_ble_mesh_provisioner_delete_node_with_addr(addr);
}

static void example_ble_mesh_set_msg_common(esp_ble_mesh_client_common_param_t *common,
                                            esp_ble_mesh_node_t *node,
                                            esp_ble_mesh_model_t *model, uint32_t opcode)
{
    common->opcode = opcode;
    common->model = model;
    common->ctx.net_idx = prov_key.net_idx;
    common->ctx.app_idx = prov_key.app_idx;
    common->ctx.addr = node->unicast_addr;
    common->ctx.send_ttl = MSG_SEND_TTL;
    common->ctx.send_rel = MSG_SEND_REL;
    common->msg_timeout = MSG_TIMEOUT;
    common->msg_role = MSG_ROLE;
}
/**
 *  @brief Hàm này thực hiện sau khi hoàn thành việc cấp phép
 *
 *  @param[in] node_index index của node trong mạng
 *  @param[in] uuid UUID
 *  @param[in] primary_addr Unicast address
 *  @param[in] element_num element_num
 *  @param[in] net_idx net_idx
 *  @return none
 */
static esp_err_t prov_complete(uint16_t node_index, const esp_ble_mesh_octet16_t uuid,
                               uint16_t primary_addr, uint8_t element_num, uint16_t net_idx)
{
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_cfg_client_get_state_t get = {0};
    esp_ble_mesh_node_t *node = NULL;
    char name[10] = {'\0'};
    esp_err_t err;

    ESP_LOGI(TAG, "node_index %u, primary_addr 0x%04x, element_num %u, net_idx 0x%03x",
             node_index, primary_addr, element_num, net_idx);
    ESP_LOG_BUFFER_HEX("uuid", uuid, ESP_BLE_MESH_OCTET16_LEN);
    char data_new[50];
    sprintf(data_new, "NEW Co thiet bi thu %d ket noi vao mang", primary_addr - 4);
    esp_mqtt_client_publish(client, "/smarthome/devices", data_new, 0, 1, 0);
    store.server_addr = primary_addr;
    switch (primary_addr)
    {
    case 5: // lưu lại thông tin các node
        store.server_addr = primary_addr;
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY, &store, sizeof(store));
        break;

    case 6:
        store2.server_addr = primary_addr;
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY2, &store2, sizeof(store2));
        break;
    case 7:
        store3.server_addr = primary_addr;
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY3, &store3, sizeof(store3));
        break;
    case 8:
        store4.server_addr = primary_addr;
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY4, &store4, sizeof(store4));
        break;
    case 9:
        store5.server_addr = primary_addr;
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY5, &store5, sizeof(store5));
        break;
    case 10:
        store6.server_addr = primary_addr;
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY6, &store6, sizeof(store6));
        break;
    case 11:
        store7.server_addr = primary_addr;
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY7, &store7, sizeof(store7));
        break;
    case 12:
        store8.server_addr = primary_addr;
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY8, &store8, sizeof(store8));
        break;
    case 13:
        store9.server_addr = primary_addr;
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY9, &store9, sizeof(store9));
        break;
    case 14:
        store10.server_addr = primary_addr;
        ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY10, &store10, sizeof(store10));
        break;
    default:
        break;
    }

    sprintf(name, "%s%02x", "NODE-", node_index);
    err = esp_ble_mesh_provisioner_set_node_name(node_index, name);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set node name");
        return ESP_FAIL;
    }

    node = esp_ble_mesh_provisioner_get_node_with_addr(primary_addr);
    if (node == NULL)
    {
        ESP_LOGE(TAG, "Failed to get node 0x%04x info", primary_addr);
        return ESP_FAIL;
    }

    example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
    get.comp_data_get.page = COMP_DATA_PAGE_0;
    err = esp_ble_mesh_config_client_get_state(&common, &get);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send Config Composition Data Get");
        return ESP_FAIL;
    }

    return ESP_OK;
}
/**
 *  @brief Hàm này thực hiện gửi yêu cầu cấp phếp đến node nếu đúng định dạng UUID
 *
 *  @param[in] dev_uuid uuid của node
 *  @param[in] addr unicast address của node
 *  @param[in] addr_type addr_type
 *  @param[in] oob_info oob_info
 *  @param[in] adv_type adv_type
 *  @param[in] bearer loại bearer
 *  @return none
 */
static void recv_unprov_adv_pkt(uint8_t dev_uuid[ESP_BLE_MESH_OCTET16_LEN], uint8_t addr[BD_ADDR_LEN],
                                esp_ble_mesh_addr_type_t addr_type, uint16_t oob_info,
                                uint8_t adv_type, esp_ble_mesh_prov_bearer_t bearer)
{
    esp_ble_mesh_unprov_dev_add_t add_dev = {0};
    esp_err_t err;
    ESP_LOG_BUFFER_HEX("Device address", addr, BD_ADDR_LEN);
    ESP_LOGI(TAG, "Address type 0x%02x, adv type 0x%02x", addr_type, adv_type);
    ESP_LOG_BUFFER_HEX("Device UUID", dev_uuid, ESP_BLE_MESH_OCTET16_LEN);
    ESP_LOGI(TAG, "oob info 0x%04x, bearer %s", oob_info, (bearer & ESP_BLE_MESH_PROV_ADV) ? "PB-ADV" : "PB-GATT");

    memcpy(add_dev.addr, addr, BD_ADDR_LEN);
    add_dev.addr_type = (uint8_t)addr_type;
    memcpy(add_dev.uuid, dev_uuid, ESP_BLE_MESH_OCTET16_LEN);
    add_dev.oob_info = oob_info;
    add_dev.bearer = (uint8_t)bearer;
    /* Note: If unprovisioned device adv packets have not been received, we should not add
             device with ADD_DEV_START_PROV_NOW_FLAG set. */
    err = esp_ble_mesh_provisioner_add_unprov_dev(&add_dev,
                                                  ADD_DEV_RM_AFTER_PROV_FLAG | ADD_DEV_START_PROV_NOW_FLAG | ADD_DEV_FLUSHABLE_DEV_FLAG);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start provisioning device");
    }
}
/**
 *  @brief Hàm này gọi lại khi có event cấp phép đến 1 node
 *
 *  @param[in] event Event của callBack
 *  @param[in] param Thông tin của Node trong gói quảng cáo
 *  @return none
 */
static void example_ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                             esp_ble_mesh_prov_cb_param_t *param)
{
    switch (event)
    {
    case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code %d", param->prov_register_comp.err_code);
        mesh_example_info_restore(); /* Restore proper mesh example info */
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT: // sự kiện bật scanner các gói tin quảng cáo
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT, err_code %d", param->provisioner_prov_enable_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT: // sự kiện tắt scanner các gói tin quảng cáo
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT, err_code %d", param->provisioner_prov_disable_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_RECV_UNPROV_ADV_PKT_EVT: // sự kiện phát hiện thiết bị chưa đc cấp phép
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_RECV_UNPROV_ADV_PKT_EVT");
        recv_unprov_adv_pkt(param->provisioner_recv_unprov_adv_pkt.dev_uuid, param->provisioner_recv_unprov_adv_pkt.addr,
                            param->provisioner_recv_unprov_adv_pkt.addr_type, param->provisioner_recv_unprov_adv_pkt.oob_info,
                            param->provisioner_recv_unprov_adv_pkt.adv_type, param->provisioner_recv_unprov_adv_pkt.bearer);
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_LINK_OPEN_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_LINK_OPEN_EVT, bearer %s",
                 param->provisioner_prov_link_open.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_LINK_CLOSE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_LINK_CLOSE_EVT, bearer %s, reason 0x%02x",
                 param->provisioner_prov_link_close.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT", param->provisioner_prov_link_close.reason);
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_COMPLETE_EVT: // sự kiện hoàn thành cấp phép
        prov_complete(param->provisioner_prov_complete.node_idx, param->provisioner_prov_complete.device_uuid,
                      param->provisioner_prov_complete.unicast_addr, param->provisioner_prov_complete.element_num,
                      param->provisioner_prov_complete.netkey_idx);
        break;
    case ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT, err_code %d", param->provisioner_add_unprov_dev_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT: // match UUID của mode
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT, err_code %d", param->provisioner_set_dev_uuid_match_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT, err_code %d", param->provisioner_set_node_name_comp.err_code);
        if (param->provisioner_set_node_name_comp.err_code == 0)
        {
            const char *name = esp_ble_mesh_provisioner_get_node_name(param->provisioner_set_node_name_comp.node_index);
            if (name)
            {
                ESP_LOGI(TAG, "Node %d name %s", param->provisioner_set_node_name_comp.node_index, name);
            }
        }
        break;
    case ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT: // add app key vào các model
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT, err_code %d", param->provisioner_add_app_key_comp.err_code);
        if (param->provisioner_add_app_key_comp.err_code == 0)
        {
            esp_err_t err;
            prov_key.app_idx = param->provisioner_add_app_key_comp.app_idx;
            // add appkey vào sensor model
            err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
                                                                       ESP_BLE_MESH_MODEL_ID_SENSOR_CLI, ESP_BLE_MESH_CID_NVAL);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to bind AppKey to sensor client");
            }
            // add appkey vào vendor model
            err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
                                                                       ESP_BLE_MESH_VND_MODEL_ID_CLIENT, CID_ESP);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to bind AppKey to vendor client");
            }
        }
        break;
    case ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT, err_code %d", param->provisioner_bind_app_key_to_model_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_STORE_NODE_COMP_DATA_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_STORE_NODE_COMP_DATA_COMP_EVT, err_code %d", param->provisioner_store_node_comp_data_comp.err_code);
        break;
    case ESP_BLE_MESH_MODEL_SUBSCRIBE_GROUP_ADDR_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_SUBSCRIBE_GROUP_ADDR_COMP_EVT, err_code %d", param->model_sub_group_addr_comp.err_code);
        break;
    default:
        break;
    }
}

static void example_ble_mesh_parse_node_comp_data(const uint8_t *data, uint16_t length)
{
    uint16_t cid, pid, vid, crpl, feat;
    uint16_t loc, model_id, company_id;
    uint8_t nums, numv;
    uint16_t offset;
    int i;

    cid = COMP_DATA_2_OCTET(data, 0);
    pid = COMP_DATA_2_OCTET(data, 2);
    vid = COMP_DATA_2_OCTET(data, 4);
    crpl = COMP_DATA_2_OCTET(data, 6);
    feat = COMP_DATA_2_OCTET(data, 8);
    offset = 10;

    ESP_LOGI(TAG, "********************** Composition Data Start **********************");
    ESP_LOGI(TAG, "* CID 0x%04x, PID 0x%04x, VID 0x%04x, CRPL 0x%04x, Features 0x%04x *", cid, pid, vid, crpl, feat);
    for (; offset < length;)
    {
        loc = COMP_DATA_2_OCTET(data, offset);
        nums = COMP_DATA_1_OCTET(data, offset + 2);
        numv = COMP_DATA_1_OCTET(data, offset + 3);
        offset += 4;
        ESP_LOGI(TAG, "* Loc 0x%04x, NumS 0x%02x, NumV 0x%02x *", loc, nums, numv);
        for (i = 0; i < nums; i++)
        {
            model_id = COMP_DATA_2_OCTET(data, offset);
            ESP_LOGI(TAG, "* SIG Model ID 0x%04x *", model_id);
            offset += 2;
        }
        for (i = 0; i < numv; i++)
        {
            company_id = COMP_DATA_2_OCTET(data, offset);
            model_id = COMP_DATA_2_OCTET(data, offset + 2);
            ESP_LOGI(TAG, "* Vendor Model ID 0x%04x, Company ID 0x%04x *", model_id, company_id);
            offset += 4;
        }
    }
    ESP_LOGI(TAG, "*********************** Composition Data End ***********************");
}

/**
 *  @brief Hàm này gọi lại khi có event tin nhắn từ node gửi đến
 *
 *  @param[in] event Event của callBack
 *  @param[in] param Thông tin của Node gửi đến
 *  @return none
 */
static void example_ble_mesh_config_client_cb(esp_ble_mesh_cfg_client_cb_event_t event,
                                              esp_ble_mesh_cfg_client_cb_param_t *param)
{
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_cfg_client_set_state_t set = {0};
    esp_ble_mesh_node_t *node = NULL;
    esp_err_t err;

    ESP_LOGI(TAG, "Config client, err_code %d, event %u, addr 0x%04x, opcode 0x%04x",
             param->error_code, event, param->params->ctx.addr, param->params->opcode);

    if (param->error_code)
    {
        ESP_LOGE(TAG, "Send config client message failed, opcode 0x%04x", param->params->opcode);
        return;
    }

    node = esp_ble_mesh_provisioner_get_node_with_addr(param->params->ctx.addr);
    if (!node)
    {
        ESP_LOGE(TAG, "Failed to get node 0x%04x info", param->params->ctx.addr);
        return;
    }

    switch (event)
    {
    case ESP_BLE_MESH_CFG_CLIENT_GET_STATE_EVT:
        if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET)
        {
            ESP_LOG_BUFFER_HEX("Composition data", param->status_cb.comp_data_status.composition_data->data,
                               param->status_cb.comp_data_status.composition_data->len);
            example_ble_mesh_parse_node_comp_data(param->status_cb.comp_data_status.composition_data->data,
                                                  param->status_cb.comp_data_status.composition_data->len);
            err = esp_ble_mesh_provisioner_store_node_comp_data(param->params->ctx.addr,
                                                                param->status_cb.comp_data_status.composition_data->data,
                                                                param->status_cb.comp_data_status.composition_data->len);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to store node composition data");
                break;
            }

            example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
            set.app_key_add.net_idx = prov_key.net_idx;
            set.app_key_add.app_idx = prov_key.app_idx;
            memcpy(set.app_key_add.app_key, prov_key.app_key, ESP_BLE_MESH_OCTET16_LEN);
            err = esp_ble_mesh_config_client_set_state(&common, &set);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to send Config AppKey Add");
            }
        }
        break;
    case ESP_BLE_MESH_CFG_CLIENT_SET_STATE_EVT:
        if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD)
        {
            if (temp_pro == 1)
            {
                ESP_LOGI(TAG, "ESP_BLE_MESH_CFG_CLIENT_SET_STATE_EVT 2");
                example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
                set.model_app_bind.element_addr = node->unicast_addr;
                set.model_app_bind.model_app_idx = prov_key.app_idx;
                set.model_app_bind.model_id = ESP_BLE_MESH_MODEL_ID_SENSOR_SRV;
                set.model_app_bind.company_id = ESP_BLE_MESH_CID_NVAL;
                err = esp_ble_mesh_config_client_set_state(&common, &set);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "Failed to send Config Model App Bind");
                    return;
                }
            }
            if (temp_pro == 2)
            {
                example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
                set.model_app_bind.element_addr = node->unicast_addr;
                set.model_app_bind.model_app_idx = prov_key.app_idx;
                set.model_app_bind.model_id = ESP_BLE_MESH_VND_MODEL_ID_SERVER;
                set.model_app_bind.company_id = CID_ESP;
                err = esp_ble_mesh_config_client_set_state(&common, &set);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "Failed to send Config Model App Bind");
                }
                ESP_LOGI(TAG, " temp_pro = 2");
            }
        }
        else if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND)
        {
            if (param->status_cb.model_app_status.model_id == ESP_BLE_MESH_MODEL_ID_SENSOR_SRV &&
                param->status_cb.model_app_status.company_id == ESP_BLE_MESH_CID_NVAL)
            {
                ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_ID_SENSOR_SRV ");
                example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
                set.model_app_bind.element_addr = node->unicast_addr;
                set.model_app_bind.model_app_idx = prov_key.app_idx;
                set.model_app_bind.model_id = ESP_BLE_MESH_MODEL_ID_SENSOR_SETUP_SRV;
                set.model_app_bind.company_id = ESP_BLE_MESH_CID_NVAL;
                err = esp_ble_mesh_config_client_set_state(&common, &set);
                if (err)
                {
                    ESP_LOGE(TAG, "Failed to send Config Model App Bind");
                    return;
                }
            }
            else if (param->status_cb.model_app_status.model_id == ESP_BLE_MESH_MODEL_ID_SENSOR_SETUP_SRV &&
                     param->status_cb.model_app_status.company_id == ESP_BLE_MESH_CID_NVAL)
            {
                example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
                set.app_key_add.net_idx = prov_key.net_idx;
                set.app_key_add.app_idx = prov_key.app_idx;
                memcpy(set.app_key_add.app_key, prov_key.app_key, ESP_BLE_MESH_OCTET16_LEN);
                err = esp_ble_mesh_config_client_set_state(&common, &set);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "Failed to send Config AppKey Add vendor");
                }
                temp_pro = 2;
            }
            else
            {
                example_ble_mesh_send_sensor_message(ESP_BLE_MESH_MODEL_OP_SENSOR_GET);
                ESP_LOGW(TAG, "Provision and config successfully");
            }
        }
        break;
    case ESP_BLE_MESH_CFG_CLIENT_PUBLISH_EVT:
        if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_STATUS)
        {
            ESP_LOG_BUFFER_HEX("Composition data", param->status_cb.comp_data_status.composition_data->data,
                               param->status_cb.comp_data_status.composition_data->len);
        }
        break;
    case ESP_BLE_MESH_CFG_CLIENT_TIMEOUT_EVT:
        switch (param->params->opcode)
        {
        case ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET:
        {
            esp_ble_mesh_cfg_client_get_state_t get = {0};
            example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
            get.comp_data_get.page = COMP_DATA_PAGE_0;
            err = esp_ble_mesh_config_client_get_state(&common, &get);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to send Config Composition Data Get");
            }
            break;
        }
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
            example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
            set.app_key_add.net_idx = prov_key.net_idx;
            set.app_key_add.app_idx = prov_key.app_idx;
            memcpy(set.app_key_add.app_key, prov_key.app_key, ESP_BLE_MESH_OCTET16_LEN);
            err = esp_ble_mesh_config_client_set_state(&common, &set);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to send Config AppKey Add");
            }
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
            example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
            set.model_app_bind.element_addr = node->unicast_addr;
            set.model_app_bind.model_app_idx = prov_key.app_idx;
            set.model_app_bind.model_id = ESP_BLE_MESH_VND_MODEL_ID_SERVER;
            set.model_app_bind.company_id = CID_ESP;
            err = esp_ble_mesh_config_client_set_state(&common, &set);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to send Config Model App Bind");
            }

            break;
        default:
            break;
        }
        break;
    default:
        ESP_LOGE(TAG, "Invalid config client event %u", event);
        break;
    }
}

/**
 *  @brief Hàm gửi tin nhắn đến node cụ thể theo unicast address
 *
 *  @param[in] addr địa chỉ unicast address
 *  @param[in] data data cần gửi
 *  @return none
 */
void example_ble_mesh_send_vendor_message(uint16_t addr, void *data)
{
    esp_ble_mesh_msg_ctx_t ctx = {0};
    uint32_t opcode;
    esp_err_t err;

    ctx.net_idx = prov_key.net_idx;
    ctx.app_idx = prov_key.app_idx;
    ctx.addr = addr;
    ctx.send_ttl = MSG_SEND_TTL;
    ctx.send_rel = MSG_SEND_REL;
    opcode = ESP_BLE_MESH_VND_MODEL_OP_SEND; // opcode để phân loại tin nhắn
    ESP_LOGI(TAG, "Da gui tn den node %02x", addr);
    err = esp_ble_mesh_client_model_send_msg(vendor_client.model, &ctx, opcode,
                                             strlen(data), (uint8_t *)data, MSG_TIMEOUT, false, MSG_ROLE); // gửi tin nhắn kèm opcode
    ESP_LOGI(TAG, "Farme: %.*s\n", strlen(data), (char *)data);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send vendor message 0x%06x", opcode);
        return;
    }
}
/**
 *  @brief Khi quá time mà không gửi đc tin nhắn
 *
 *  @param[in] opcode opcode
 *  @return none
 */
static void example_ble_mesh_sensor_timeout(uint32_t opcode)
{
    example_ble_mesh_send_sensor_message(opcode);
}
/**
 *  @brief Hàm này gọi lại khi có event tin nhắn từ node gửi đến
 *
 *  @param[in] event Event của callBack
 *  @param[in] param Thông tin của Node gửi đến
 *  @return none
 */
static void example_ble_mesh_sensor_client_cb(esp_ble_mesh_sensor_client_cb_event_t event,
                                              esp_ble_mesh_sensor_client_cb_param_t *param)
{
    esp_ble_mesh_node_t *node = NULL;
    printf("RSSI: %d \n", param->params->ctx.recv_rssi);
    ESP_LOGI(TAG, "Sensor client, event %u, addr 0x%04x", event, param->params->ctx.addr);
    if (param->params->ctx.addr == 5 || param->params->ctx.addr == 10)
    {
        rx_node = 1;
    }
    else if (param->params->ctx.addr == 6 || param->params->ctx.addr == 11)
    {
        rx_node = 2;
    }
    else if (param->params->ctx.addr == 7 || param->params->ctx.addr == 12)
    {
        rx_node = 3;
    }
    else if (param->params->ctx.addr == 8 || param->params->ctx.addr == 13)
    {
        rx_node = 4;
    }
    else if (param->params->ctx.addr == 9 || param->params->ctx.addr == 14)
    {
        rx_node = 5;
    }
    if (rx_mess_check == 1)
    {
        ESP_LOGI(TAG, "rx_mess_check == 1\n");
        if (data_setup.cluster == '1' && data_setup.node == '1' && rx_node == 1)
        {
            example_ble_mesh_send_vendor_message(5, data_tx_ble);
        }
        else if (data_setup.cluster == '1' && data_setup.node == '2' && rx_node == 2)
        {
            ESP_LOGI(TAG, "data_setup.cluster == '1' && data_setup.node == '2'\n");
            ESP_LOGI(TAG, "Data: %s\n", data_tx_ble);
            example_ble_mesh_send_vendor_message(6, data_tx_ble);
        }
        else if (data_setup.cluster == '1' && data_setup.node == '3' && rx_node == 3)
        {
            example_ble_mesh_send_vendor_message(7, data_tx_ble);
        }
        else if (data_setup.cluster == '1' && data_setup.node == '4' && rx_node == 4)
        {
            example_ble_mesh_send_vendor_message(8, data_tx_ble);
        }
        else if (data_setup.cluster == '1' && data_setup.node == '5' && rx_node == 5)
        {
            example_ble_mesh_send_vendor_message(9, data_tx_ble);
        }
        else if (data_setup.cluster == '2' && data_setup.node == '1' && rx_node == 1)
        {
            example_ble_mesh_send_vendor_message(10, data_tx_ble);
        }
        else if (data_setup.cluster == '2' && data_setup.node == '2' && rx_node == 2)
        {
            example_ble_mesh_send_vendor_message(11, data_tx_ble);
        }
        else if (data_setup.cluster == '2' && data_setup.node == '3' && rx_node == 3)
        {
            example_ble_mesh_send_vendor_message(12, data_tx_ble);
        }
        else if (data_setup.cluster == '2' && data_setup.node == '4' && rx_node == 4)
        {
            example_ble_mesh_send_vendor_message(13, data_tx_ble);
        }
        else if (data_setup.cluster == '2' && data_setup.node == '5' && rx_node == 5)
        {
            example_ble_mesh_send_vendor_message(14, data_tx_ble);
        }
    }
    if (param->error_code)
    {
        ESP_LOGE(TAG, "Send sensor client message failed (err %d)", param->error_code);
        return;
    }

    node = esp_ble_mesh_provisioner_get_node_with_addr(param->params->ctx.addr);
    if (!node)
    {
        ESP_LOGE(TAG, "Node 0x%04x not exists", param->params->ctx.addr);
        return;
    }

    switch (event)
    {
    case ESP_BLE_MESH_SENSOR_CLIENT_GET_STATE_EVT:
        switch (param->params->opcode)
        {
        case ESP_BLE_MESH_MODEL_OP_SENSOR_GET:
            ESP_LOGI(TAG, "Sensor Status, opcode 0x%04x", param->params->ctx.recv_op);
            if (param->status_cb.sensor_status.marshalled_sensor_data->len)
            {
                example_ble_mesh_send_vendor_message(param->params->ctx.addr, data_ack); // gửi lại ack
            }
            break;
        default:
            ESP_LOGE(TAG, "Unknown Sensor Get opcode 0x%04x", param->params->ctx.recv_op);
            break;
        }
        break;
    case ESP_BLE_MESH_SENSOR_CLIENT_SET_STATE_EVT:
        switch (param->params->opcode)
        {
        default:
            ESP_LOGE(TAG, "Unknown Sensor Set opcode 0x%04x", param->params->ctx.recv_op);
            break;
        }
        break;
    case ESP_BLE_MESH_SENSOR_CLIENT_PUBLISH_EVT:
        // ESP_LOGI(TAG, "ESP_BLE_MESH_SENSOR_CLIENT_PUBLISH_EVT\n");
        if (rx_mess_check == 0)
        {
            example_ble_mesh_send_vendor_message(param->params->ctx.addr, data_ack); // gửi lại ack
            sprintf(data_temp2, "%.*s", param->status_cb.sensor_status.marshalled_sensor_data->len, (char *)(param->status_cb.sensor_status.marshalled_sensor_data->data));
            cluster_temp2 = strtok(data_temp2, " ");
            temper_temp2 = strtok(NULL, " ");
            batt_temp2 = strtok(NULL, " ");
            xEventGroupSetBits(s_mqtt_event_group, MESSAGE_ARRIVE_BIT);
        }
        else if (rx_mess_check == 1)
        {
            rx_mess_check = 0;
        }
        ESP_LOGI(TAG, "Sensor Data: %.*s ESP_BLE_MESH_SENSOR_CLIENT_PUBLISH_EVT\n", param->status_cb.sensor_status.marshalled_sensor_data->len, (char *)(param->status_cb.sensor_status.marshalled_sensor_data->data));
        break;
    case ESP_BLE_MESH_SENSOR_CLIENT_TIMEOUT_EVT:
        example_ble_mesh_sensor_timeout(param->params->opcode);
    default:
        break;
    }
}
/**
 *  @brief Gửi thông tin gateway về sensor model lần đầu tiên đến các node
 *
 *  @param[in] opcode opcode message muốn nhận
 *  @return none
 */
void example_ble_mesh_send_sensor_message(uint32_t opcode)
{
    esp_ble_mesh_sensor_client_get_state_t get = {0};
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_node_t *node = NULL;
    esp_err_t err = ESP_OK;
    ESP_LOGI(TAG, "Da gui tin nhan den node 0x%04X\n", store.server_addr);
    node = esp_ble_mesh_provisioner_get_node_with_addr(store.server_addr); // gửi theo Unicast address
    if (node == NULL)
    {
        ESP_LOGE(TAG, "Node 0x%04x not exists", store.server_addr);
        return;
    }
    example_ble_mesh_set_msg_common(&common, node, sensor_client.model, opcode); // gửi các tham số liên quan đến sensor model

    err = esp_ble_mesh_sensor_client_get_state(&common, &get);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send sensor message 0x%04x", opcode);
    }
}
static void example_ble_mesh_custom_model_cb(esp_ble_mesh_model_cb_event_t event,
                                             esp_ble_mesh_model_cb_param_t *param)
{
    static int64_t start_time;

    switch (event)
    {
    case ESP_BLE_MESH_MODEL_OPERATION_EVT:
        if (param->model_operation.opcode == ESP_BLE_MESH_VND_MODEL_OP_STATUS)
        {
            // int64_t end_time = esp_timer_get_time();
            // ESP_LOGI(TAG, "Sensor Node, event %u, addr 0x%04x", event, param->model_operation.ctx->addr);
            // ESP_LOGI(TAG, "Recv 0x%06x, tid 0x%04x, time %lldus",
            //          param->model_operation.opcode, store.vnd_tid, end_time - start_time);
            // ESP_LOGI(TAG, "Mess_Node: %.*s\n", param->model_operation.length, (char *)(param->model_operation.msg));
        }
        break;
    case ESP_BLE_MESH_MODEL_SEND_COMP_EVT:
        if (param->model_send_comp.err_code)
        {
            ESP_LOGE(TAG, "Failed to send message 0x%06x", param->model_send_comp.opcode);
            break;
        }
        // start_time = esp_timer_get_time();
        break;
    case ESP_BLE_MESH_MODEL_PUBLISH_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_PUBLISH_COMP_EVT, err_code %d",
                 param->model_publish_comp.err_code);
        if (param->model_publish_comp.err_code)
        {
            ESP_LOGE(TAG, "Failed to publish message ");
            break;
        }
        rx_mess_check = 0;
        break;
    case ESP_BLE_MESH_CLIENT_MODEL_RECV_PUBLISH_MSG_EVT:
        // ESP_LOGI(TAG, "Receive publish message 0x%06x", param->client_recv_publish_msg.opcode);
        break;
    case ESP_BLE_MESH_CLIENT_MODEL_SEND_TIMEOUT_EVT:
        ESP_LOGW(TAG, "Client message 0x%06x timeout", param->client_send_timeout.opcode);
        // example_ble_mesh_send_vendor_message(true);
        break;
    default:
        break;
    }
}

/**
 *  int json_parse_data_setup(char *json, data_setup_t *out_data)
 *  @brief Phân giã các key và value của chuỗi JSON đầu vào
 *
 *  @param[in] json Chuỗi jSON đầu vào
 *  @param[in] out_data Data sau khi được Parse
 *  @return 0 if OK, −1 on error
 */
int json_parse_data_setup(char *json, data_setup_t *out_data)
{
    jparse_ctx_t jctx;
    int ret = json_parse_start(&jctx, json, strlen(json));
    if (ret != OS_SUCCESS)
    {
        printf("Parser failed\n");
        return -1;
    }
    if (json_obj_get_string(&jctx, "Cluster", &out_data->cluster, 20) != OS_SUCCESS)
    {
        printf("Parser failed\n");
    }
    if (json_obj_get_string(&jctx, "Node", &out_data->node, 20) != OS_SUCCESS)
    {
        printf("Parser failed\n");
    }
    if (json_obj_get_string(&jctx, "Temperature", &out_data->temperature, 20) != OS_SUCCESS)
    {
        printf("Parser failed\n");
    }
    if (json_obj_get_string(&jctx, "Mode", &out_data->mode, 20) != OS_SUCCESS)
    {
        printf("Parser failed\n");
    }
    json_parse_end(&jctx);
    return 0;
}

/**
 *  @brief Khởi tạo BLE Mesh module và đăng ký các hàm callback xử lý event
 *
 *  @return None
 */
static esp_err_t ble_mesh_init(void)
{
    uint8_t match[2] = {0x32, 0x10}; // 2 byte đầu của UUID
    esp_err_t err;

    prov_key.net_idx = ESP_BLE_MESH_KEY_PRIMARY;
    prov_key.app_idx = APP_KEY_IDX;
    memset(prov_key.app_key, APP_KEY_OCTET, sizeof(prov_key.app_key));

    esp_ble_mesh_register_prov_callback(example_ble_mesh_provisioning_cb);           // function callback xử lý event cấp phép của Gateway
    esp_ble_mesh_register_config_client_callback(example_ble_mesh_config_client_cb); // function callback xử lý các event liên kết Model
    esp_ble_mesh_register_custom_model_callback(example_ble_mesh_custom_model_cb);
    esp_ble_mesh_register_sensor_client_callback(example_ble_mesh_sensor_client_cb); // function callback xử lý tin nhắn từ Node

    esp_ble_mesh_init(&provision, &composition);    // Initialize BLE Mesh module
    esp_ble_mesh_client_model_init(&vnd_models[0]); // Initialize Model Sensor và Vender
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);
    esp_ble_mesh_provisioner_set_dev_uuid_match(match, sizeof(match), 0x0, false);                    // chỉ cấp phép khi match UUID do gateway quy định
    esp_ble_mesh_provisioner_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);             // Bắt đầu scanner các gói quảng cáo
    esp_ble_mesh_provisioner_add_local_app_key(prov_key.app_key, prov_key.net_idx, prov_key.app_idx); // add a local AppKey for Provisioner.
    ESP_LOGI(TAG, "ESP BLE Mesh Provisioner initialized");
    return ESP_OK;
}
/**
 *  static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
 *  @brief Nhận các event được kich hoạt
 *  @param[in] handler_args argument
 *  @param[in] base Tên Event
 *  @param[in] event_id Mã Event
 *  @param[in] event_data dữ liệu từ event loop
 *  @return None
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG_MQTT, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}
/**
 *  static void mqtt_app_start(void)
 *  @brief Kết nối đến Broker
 *  @return None
 */
static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://broker.mqttdashboard.com:1883", // uri của broker hiveMQ
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client); // đăng kí hàm callback
    esp_mqtt_client_start(client);                                                        // bắt đầu kết nối
}

/**
 *  @brief Event Handler xử lý các sự kiện về kết nối đến Router
 *
 *  @param[in] arg argument
 *  @param[in] event_base Tên Event
 *  @param[in] event_id Mã Event
 *  @param[in] event_data IP được trả về
 *  @return None
 */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) // bắt đầu kết nối
    {
        printf("WIFI_EVENT EVENT WIFI_EVENT_STA_START : the event id is %d \n", event_id);
        esp_wifi_connect();
        // start task after the wifi is connected
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) // event báo mất kết nối đến AP
    {
        printf("WIFI_EVENT EVENT WIFI_EVENT_STA_DISCONNECTED : the event id is %d \n", event_id);
        if (loopHandle != NULL)
        {
            printf("WIFI_EVENT task !NULL *** %d \n", (int)loopHandle);
            vTaskDelete(loopHandle);
        }
        else
        {
            printf("WIFI_EVENT task  NULL *** %d \n", (int)loopHandle);
        }

        if (client != NULL) // mất mạng
        {
            esp_mqtt_client_stop(client); // tắt mqtt
        }

        if (s_retry_num < 10) // cố gắng kết nối dưới 10 lần
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else // quá 10 thì coi như mất mạng
        {
            wifi_info.state = INITIAL_STATE;
            nvs_save_wifiInfo(my_handler, NVS_KEY_WIFI, &wifi_info, sizeof(wifi_info)); // lưu lại trạng thái wifi
            esp_restart();                                                              // reset esp32
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) // Event đã được cấp IP
    {
        printf("IP EVENT IP_EVENT_STA_GOT_IP : the event id is %d \n", event_id);
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        // wifi connected ip assigned now start mqtt.
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT); // set bit WIFI_CONNECTED_BIT
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) // Event đã kết nối thành công
    {
        ble_mesh_init();
        xTaskCreate(&xTaskTransmitMQTT, "xTaskTransmitMQTT", 4096, NULL, 5, NULL);
        printf("WIFI_EVENT EVENT WIFI_EVENT_STA_CONNECTED : the event id is %d \n", event_id);
        printf("Starting the MQTT app \n");
        mqtt_app_start();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) // K được cấp phát IP
    {
        printf("IP_EVENT EVENT IP_EVENT_STA_LOST_IP : the event id is %d \n", event_id);
    }
}
/**
 *  void wifi_init_sta(void)
 *  @brief Kết nối đến router ở chế độ Station
 *
 *  @return None
 */
void wifi_init_sta(void) // khoi tao wifi o che do station
{
    printf("wifi_init_sta\n");
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL)); // đăng ký function call back được gọi bất cứ có sự kiện nào
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };
    strcpy((char *)wifi_config.ap.ssid, wifi_info.SSID);
    strcpy((char *)wifi_config.ap.password, wifi_info.PASSWORD);
    printf("%s\n", wifi_info.SSID);
    printf("%s\n", wifi_info.PASSWORD);
    esp_wifi_stop();
    // esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // set mode station
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start()); // bắt đầu kết nối đến router

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    while (1)
    {
        EventBits_t uxBits = xEventGroupWaitBits(s_wifi_event_group,
                                                 WIFI_CONNECTED_BIT, /* The bits within the event group to waitfor. */
                                                 pdTRUE,             /* WIFI_CONNECTED_BIT should be cleared before returning. */
                                                 pdFALSE,            /* Don't waitfor both bits, either bit will do. */
                                                 portMAX_DELAY);     /* Wait forever. */
        if ((uxBits & WIFI_CONNECTED_BIT) == WIFI_CONNECTED_BIT)
        {
            wifi_info.state = NORMAL_STATE;
            nvs_save_wifiInfo(my_handler, NVS_KEY_WIFI, &wifi_info, sizeof(wifi_info)); // lưu lại trạng thái của wifi                              // bật led báo mạng
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);                 // bật tín hiệu báo mạng đã  kết nối
            ESP_LOGI(TAG, "WIFI_CONNECTED_BIT");
            break;
        }
    }
    ESP_LOGI(TAG, "Got IP Address.");
}

void app_main(void)
{
    esp_err_t err;
    err = nvs_flash_init(); // initialise nvs flash
    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    nvs_get_wifiInfo(my_handler, NVS_KEY_WIFI, &wifi_info); // lấy thông tin wifi lưu trữ từ nvs
    err = bluetooth_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
        return;
    }
    /* Open nvs namespace for storing/restoring mesh  info */
    err = ble_mesh_nvs_open(&NVS_HANDLE);
    if (err)
    {
        return;
    }
    ble_mesh_get_dev_uuid(dev_uuid); // tạo UUID 128 bit cho gateway
    // initialise_wifi();
    s_mqtt_event_group = xEventGroupCreate();
    s_wifi_event_group = xEventGroupCreate();
    if (wifi_info.state == INITIAL_STATE) // Xem state có phải ban đầu hay k
    {
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
        assert(sta_netif);
        wifi_init_softap(); // chạy AP để user config wifi
    }
    else if (wifi_info.state == NORMAL_STATE) // Trạng thái bình thường
    {
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
        assert(sta_netif);

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        wifi_init_sta(); // chạy chế độ station
    }
}