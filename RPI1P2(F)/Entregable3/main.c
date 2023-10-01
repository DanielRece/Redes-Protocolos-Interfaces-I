#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_STA_WIFI_SSID      CONFIG_STA_WIFI_SSID
#define EXAMPLE_STA_WIFI_PASS      CONFIG_STA_WIFI_PASSWORD
#define EXAMPLE_STA_MAXIMUM_RETRY  CONFIG_STA_MAXIMUM_RETRY


#define EXAMPLE_AP_WIFI_SSID      	CONFIG_AP_WIFI_SSID
#define EXAMPLE_AP_WIFI_PASS      	CONFIG_AP_WIFI_PASSWORD
#define EXAMPLE_AP_WIFI_CHANNEL   	CONFIG_AP_WIFI_CHANNEL
#define EXAMPLE_AP_MAX_STA_CONN     CONFIG_AP_MAX_STA_CONN

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

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
	        if (s_retry_num < EXAMPLE_STA_MAXIMUM_RETRY) {
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

void wifi_init_sta(void)
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
            .ssid = EXAMPLE_STA_WIFI_SSID,
            .password = EXAMPLE_STA_WIFI_PASS,
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    wifi_config_t wifi_config_ap = {
        .ap = {
            .ssid = EXAMPLE_AP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_AP_WIFI_SSID),
            .channel = EXAMPLE_AP_WIFI_CHANNEL,
            .password = EXAMPLE_AP_WIFI_PASS,
            .max_connection = EXAMPLE_AP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_AP_WIFI_PASS) == 0) {
        wifi_config_ap.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_STA_WIFI_SSID, EXAMPLE_STA_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_STA_WIFI_SSID, EXAMPLE_STA_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
/*
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
  */
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

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
}
