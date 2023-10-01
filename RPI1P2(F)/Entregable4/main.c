/* Scan Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
    This example shows how to scan for available set of APs.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

#define DEFAULT_SCAN_LIST_SIZE CONFIG_EXAMPLE_SCAN_LIST_SIZE


#define WIFI_SSID1 	"SSID1"
#define WIFI_PASS1 	"PASS1"
#define WIFI_SSID2 	"SSID2"
#define WIFI_PASS2	"PASS2"
#define WIFI_SSID3	"SSID3"
#define WIFI_PASS3	"PASS3"

static short int FOUND[3];
static wifi_auth_mode_t WIFI_AUTHMODE[3];

static const char *TAG = "scan";

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

		default:
			ESP_LOGW(TAG,"UNKNOWN WIFI_EVENT (%ld)", event_id);
			break;
		}
	}
	if (event_base == IP_EVENT){
		switch (event_id){
		case IP_EVENT_STA_GOT_IP:
			ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&((ip_event_got_ip_t*) event_data)->ip_info.ip));
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

/* Initialize Wi-Fi as sta and set scan method */
static void wifi_scan(void)
{
	short int aux = 0;
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    FOUND[0] = 0;
    FOUND[1] = 0;
    FOUND[2] = 0;

    while(!aux){
    	esp_wifi_scan_start(NULL, true);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
        for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
            if (strlen(WIFI_SSID1) > 0 && strcmp((char *)ap_info[i].ssid, WIFI_SSID1) == 0) {
            	FOUND[0] = 1;
            	WIFI_AUTHMODE[0] = ap_info[i].authmode;
            	aux = 1;
            }
            if (strlen(WIFI_SSID2) > 0 && strcmp((char *)ap_info[i].ssid, WIFI_SSID2) == 0) {
            	FOUND[1] = 1;
            	WIFI_AUTHMODE[1] = ap_info[i].authmode;
            	aux = 1;
            }
            if (strlen(WIFI_SSID3) > 0 && strcmp((char *)ap_info[i].ssid, WIFI_SSID3) == 0) {
            	FOUND[2] = 1;
            	WIFI_AUTHMODE[2] = ap_info[i].authmode;
            	aux = 1;
            }
        }
        if (!aux) ESP_LOGI(TAG,"No se ha encontrado ninguna red conocida, reintentando...");
    }

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));

    if (FOUND[0]){
    	ESP_LOGI(TAG,"Se ha encontrado el SSID1 %s", WIFI_SSID1);
        wifi_config_t wifi_config_sta = {
            .sta = {
                .ssid = WIFI_SSID1,
                .password = WIFI_PASS1,
    	     .threshold.authmode = WIFI_AUTHMODE[0],
            },
        };
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta) );
    }else if (FOUND[1]){
    	ESP_LOGI(TAG,"Se ha encontrado el SSID2 %s", WIFI_SSID2);
        wifi_config_t wifi_config_sta = {
            .sta = {
                .ssid = WIFI_SSID2,
                .password = WIFI_PASS2,
    	     .threshold.authmode = WIFI_AUTHMODE[1],
            },
        };
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta) );
    }else if (FOUND[2]){
    	ESP_LOGI(TAG,"Se ha encontrado el SSID3 %s", WIFI_SSID3);
        wifi_config_t wifi_config_sta = {
            .sta = {
                .ssid = WIFI_SSID3,
                .password = WIFI_PASS3,
    	     .threshold.authmode = WIFI_AUTHMODE[2],
            },
        };
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta) );
    }

    ESP_ERROR_CHECK(esp_wifi_start() );

}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    wifi_scan();
}
