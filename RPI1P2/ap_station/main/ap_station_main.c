/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
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

#include "esp_mac.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID_STA      CONFIG_ESP_WIFI_SSID_STA
#define EXAMPLE_ESP_WIFI_PASS_STA      CONFIG_ESP_WIFI_PASSWORD_STA
#define EXAMPLE_ESP_MAXIMUM_RETRY_STA  CONFIG_ESP_MAXIMUM_RETRY_STA

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE_STA WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE_STA WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID_STA
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE_STA WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID_STA
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD_STA WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD_STA WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD_STA WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD_STA WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD_STA WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD_STA WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD_STA WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD_STA WIFI_AUTH_WAPI_PSK
#endif

#define EXAMPLE_ESP_WIFI_SSID_AP      CONFIG_ESP_WIFI_SSID_AP
#define EXAMPLE_ESP_WIFI_PASS_AP      CONFIG_ESP_WIFI_PASSWORD_AP
#define EXAMPLE_ESP_WIFI_CHANNEL_AP   CONFIG_ESP_WIFI_CHANNEL_AP
#define EXAMPLE_MAX_STA_CONN_AP       CONFIG_ESP_MAX_STA_CONN_AP

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi ap_station";

static int s_retry_num = 0;


static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT){
		switch(event_id){
		case WIFI_EVENT_STA_START:
			esp_wifi_connect();
			break;
		case WIFI_EVENT_STA_DISCONNECTED:
			ESP_LOGW(TAG, "WIFI_EVENT_STA_DISCONNECTED, REASON: %d", ((wifi_event_sta_disconnected_t*)event_data)->reason);
	        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY_STA) {
	            esp_wifi_connect();
	            s_retry_num++;
	            ESP_LOGI(TAG, "retry to connect to the AP");
	        } else {
	            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
	        }
	        ESP_LOGI(TAG,"connect to the AP fail");
			break;

		case WIFI_EVENT_STA_STOP:
			ESP_LOGI(TAG,"WIFI_EVENT_STA_STOP");
			break;

		case WIFI_EVENT_STA_CONNECTED:
			ESP_LOGI(TAG,"WIFI_EVENT_STA_CONNECTED");
			break;

		case WIFI_EVENT_STA_AUTHMODE_CHANGE:
			ESP_LOGI(TAG,"WIFI_EVENT_STA_AUTHMODE_CHANGE");
			break;

		case WIFI_EVENT_AP_START:
			ESP_LOGI(TAG,"WIFI_EVENT_AP_START");
			break;

		case WIFI_EVENT_AP_STOP:
			ESP_LOGI(TAG,"WIFI_EVENT_AP_STOP");
			break;

		case WIFI_EVENT_AP_PROBEREQRECVED:
			ESP_LOGI(TAG,"WIFI_EVENT_AP_PROBEREQRECVED");
			break;

		case WIFI_EVENT_AP_STACONNECTED:
			ESP_LOGI(TAG,"WIFI_EVENT_AP_STACONNECTED, station "MACSTR" join, AID=%d",MAC2STR(((wifi_event_ap_staconnected_t *) event_data)->mac)
					,((wifi_event_ap_staconnected_t *) event_data)->aid);
			break;

		case WIFI_EVENT_AP_STADISCONNECTED:
			ESP_LOGW(TAG,"WIFI_EVENT_AP_STADISCONNECTED, station "MACSTR" leave, AID=%d",MAC2STR(((wifi_event_ap_stadisconnected_t *) event_data)->mac)
					,((wifi_event_ap_stadisconnected_t *) event_data)->aid);
			break;

		default:
			ESP_LOGW(TAG,"UNKNOWN WIFI_EVENT (%ld)", event_id);
			break;
		}
	}


	if (event_base == IP_EVENT){
		switch (event_id){
		case IP_EVENT_STA_GOT_IP:
			ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&((ip_event_got_ip_t*) event_data)->ip_info.ip));
			s_retry_num = 0;
			xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
			break;

		case IP_EVENT_STA_LOST_IP:
			ESP_LOGW(TAG,"IP_EVENT_STA_LOST_IP");
			break;

		default:
			ESP_LOGW(TAG,"UNKNOWN IP_EVENT (%ld)", event_id);
			break;
		}
    }
}

void wifi_init_ap_sta(void)
{
    
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    
    wifi_config_t wifi_config_sta = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID_STA,
            .password = EXAMPLE_ESP_WIFI_PASS_STA,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    wifi_config_t wifi_config_ap = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID_AP,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID_AP),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL_AP,
            .password = EXAMPLE_ESP_WIFI_PASS_AP,
            .max_connection = EXAMPLE_MAX_STA_CONN_AP,
            #ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
                .authmode = WIFI_AUTH_WPA3_PSK,
                .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            #else
                .authmode = WIFI_AUTH_WPA2_PSK,
            #endif
        }
    };

    if (strlen(EXAMPLE_ESP_WIFI_PASS_AP) == 0) {
        wifi_config_ap.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap));

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init in ap_sta mode finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID_STA, EXAMPLE_ESP_WIFI_PASS_STA);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID_STA, EXAMPLE_ESP_WIFI_PASS_STA);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    
    ESP_LOGI(TAG, "AP_SSID:%s AP_password:%s AP_channel:%d",
             EXAMPLE_ESP_WIFI_SSID_AP, EXAMPLE_ESP_WIFI_PASS_AP, EXAMPLE_ESP_WIFI_CHANNEL_AP);

}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP_STA");
    wifi_init_ap_sta();
}
