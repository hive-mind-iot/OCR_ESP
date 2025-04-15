#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#define MAX_AP_COUNT 20
#define TARGET_SSID "neoinfo2"

static const char *TAG = "wifi_scanner";
static EventGroupHandle_t wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;
static const int WIFI_FAIL_BIT = BIT1;
static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi station started");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying connection to the AP");
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI(TAG, "Failed to connect to the AP");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected! IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static bool wifi_scan_and_connect(void)
{
    bool target_found = false;
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300,
    };

    ESP_LOGI(TAG, "Starting WiFi scan...");
    
    // Start WiFi scan
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    
    // Get scan results
    uint16_t ap_count = MAX_AP_COUNT;
    wifi_ap_record_t ap_records[MAX_AP_COUNT];
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_records));
    
    // Print scan results
    ESP_LOGI(TAG, "Found %d access points:", ap_count);
    ESP_LOGI(TAG, "--------------------------------------------------------------------------------");
    ESP_LOGI(TAG, "%-32s | %-18s | %-4s | %-5s | %-12s", "SSID", "BSSID", "CH", "RSSI", "AUTH MODE");
    ESP_LOGI(TAG, "--------------------------------------------------------------------------------");
    
    for (int i = 0; i < ap_count; i++) {
        char mac_str[18];
        sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
                ap_records[i].bssid[0], ap_records[i].bssid[1], ap_records[i].bssid[2],
                ap_records[i].bssid[3], ap_records[i].bssid[4], ap_records[i].bssid[5]);
        
        char *auth_mode_str;
        switch (ap_records[i].authmode) {
            case WIFI_AUTH_OPEN:
                auth_mode_str = "OPEN";
                break;
            case WIFI_AUTH_WEP:
                auth_mode_str = "WEP";
                break;
            case WIFI_AUTH_WPA_PSK:
                auth_mode_str = "WPA_PSK";
                break;
            case WIFI_AUTH_WPA2_PSK:
                auth_mode_str = "WPA2_PSK";
                break;
            case WIFI_AUTH_WPA_WPA2_PSK:
                auth_mode_str = "WPA_WPA2_PSK";
                break;
            case WIFI_AUTH_WPA2_ENTERPRISE:
                auth_mode_str = "WPA2_ENT";
                break;
            case WIFI_AUTH_WPA3_PSK:
                auth_mode_str = "WPA3_PSK";
                break;
            case WIFI_AUTH_WPA2_WPA3_PSK:
                auth_mode_str = "WPA2_WPA3";
                break;
            default:
                auth_mode_str = "UNKNOWN";
                break;
        }
        
        ESP_LOGI(TAG, "%-32s | %-18s | %-4d | %-5d | %-12s",
                 (char *)ap_records[i].ssid,
                 mac_str,
                 ap_records[i].primary,
                 ap_records[i].rssi,
                 auth_mode_str);
        
        // Check if this is our target SSID
        if (strcmp((char *)ap_records[i].ssid, TARGET_SSID) == 0) {
            ESP_LOGI(TAG, "Found target network: %s", TARGET_SSID);
            target_found = true;
        }
    }
    
    ESP_LOGI(TAG, "--------------------------------------------------------------------------------");
    return target_found;
}

static void connect_to_wifi(void)
{
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = TARGET_SSID,
            // No password for open network
            .password = "",
            .threshold.authmode = WIFI_AUTH_OPEN,
        },
    };
    
    ESP_LOGI(TAG, "Connecting to %s", TARGET_SSID);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
    
    // Wait for the connection
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to %s", TARGET_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to %s", TARGET_SSID);
    } else {
        ESP_LOGE(TAG, "Unexpected event");
    }
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Create WiFi event group
    wifi_event_group = xEventGroupCreate();

    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Scan for WiFi networks and check if target is found
    bool target_found = wifi_scan_and_connect();
    
    if (target_found) {
        // Connect to the target network
        connect_to_wifi();
    } else {
        ESP_LOGW(TAG, "Target network '%s' not found", TARGET_SSID);
    }
    
    // Continue with regular scanning
    while (1) {
        ESP_LOGI(TAG, "Scanning again in 10 seconds...");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        
        // If we're not already connected, try to scan and connect again
        if (!(xEventGroupGetBits(wifi_event_group) & WIFI_CONNECTED_BIT)) {
            if (wifi_scan_and_connect()) {
                connect_to_wifi();
            }
        } else {
            // Just do a scan without attempting to connect
            wifi_scan_and_connect();
        }
    }
}