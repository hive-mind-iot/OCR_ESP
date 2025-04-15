#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define MAX_AP_COUNT 20

static const char *TAG = "wifi_scanner";

static void wifi_scan_task(void *pvParameters)
{
    // Configure WiFi scan
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300,
    };

    while (1) {
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
        }
        
        ESP_LOGI(TAG, "--------------------------------------------------------------------------------");
        ESP_LOGI(TAG, "Scan completed. Waiting before next scan...");
        
        // Wait for 10 seconds before scanning again
        vTaskDelay(10000 / portTICK_PERIOD_MS);
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

    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Create scan task
    xTaskCreate(wifi_scan_task, "wifi_scan_task", 4096, NULL, 5, NULL);
}