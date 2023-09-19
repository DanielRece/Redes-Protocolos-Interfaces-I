/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#define RESET_TIME_1 CONFIG_RESET_TIME_1
#define RESET_TIME_2 CONFIG_RESET_TIME_2

void a_task(void *pvParameter)
{
    printf("Hello world!\n");
    while (true) {
        printf("I am executing a task...\n");
        vTaskDelay(RESET_TIME_1*1000/ portTICK_PERIOD_MS);
    }
    vTaskDelete( NULL );
}

void another_task(void *pvParameter)
{
    printf("Hello one more time world!\n");
    while (true) {
        printf("I am executing another task...\n");
        vTaskDelay(RESET_TIME_2*1000/ portTICK_PERIOD_MS);
    }
    vTaskDelete( NULL );
}

void app_main(void)
{
    nvs_flash_init();
    xTaskCreate( &a_task, "a_task", 2048, NULL, 5, NULL );
    xTaskCreate( &another_task, "another_task", 2048, NULL, 5, NULL );
}
