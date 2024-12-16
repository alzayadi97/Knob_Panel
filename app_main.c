/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "ui_light_2color.h"
#include "app_audio.h"
#include "bsp/esp-bsp.h"

static const char *TAG = "main";

// Uncomment to enable memory monitoring
#define MEMORY_MONITOR 0

#if MEMORY_MONITOR
#define ARRAY_SIZE_OFFSET 5

static esp_err_t print_real_time_stats(TickType_t xTicksToWait) {
    TaskStatus_t *start_array = NULL, *end_array = NULL;
    UBaseType_t start_array_size, end_array_size;
    uint32_t start_run_time, end_run_time;
    esp_err_t ret;

    start_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    start_array = malloc(sizeof(TaskStatus_t) * start_array_size);
    if (!start_array) {
        return ESP_ERR_NO_MEM;
    }
    start_array_size = uxTaskGetSystemState(start_array, start_array_size, &start_run_time);
    if (!start_array_size) {
        free(start_array);
        return ESP_ERR_INVALID_SIZE;
    }

    vTaskDelay(xTicksToWait);
    end_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    end_array = malloc(sizeof(TaskStatus_t) * end_array_size);
    if (!end_array) {
        free(start_array);
        return ESP_ERR_NO_MEM;
    }
    end_array_size = uxTaskGetSystemState(end_array, end_array_size, &end_run_time);
    if (!end_array_size) {
        free(start_array);
        free(end_array);
        return ESP_ERR_INVALID_SIZE;
    }

    uint32_t total_elapsed_time = end_run_time - start_run_time;
    if (!total_elapsed_time) {
        free(start_array);
        free(end_array);
        return ESP_ERR_INVALID_STATE;
    }

    printf("| Task\t\t| Run Time\t| Percentage\n");
    for (int i = 0; i < start_array_size; i++) {
        int match_index = -1;
        for (int j = 0; j < end_array_size; j++) {
            if (start_array[i].xHandle == end_array[j].xHandle) {
                match_index = j;
                break;
            }
        }
        if (match_index >= 0) {
            uint32_t task_elapsed_time = end_array[match_index].ulRunTimeCounter - start_array[i].ulRunTimeCounter;
            uint32_t percentage_time = (task_elapsed_time * 100UL) / (total_elapsed_time * portNUM_PROCESSORS);
            printf("| %s\t\t| %d\t| %d%%\n", start_array[i].pcTaskName, task_elapsed_time, percentage_time);
        }
    }

    free(start_array);
    free(end_array);
    return ESP_OK;
}

static void monitor_task(void *arg) {
    const int STATS_TICKS = pdMS_TO_TICKS(2000);
    while (true) {
        ESP_LOGI(TAG, "System Info:");
        printf("Free Memory: Internal=%d SPIRAM=%d\n",
               heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
               heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        printf("Largest Block: Internal=%d SPIRAM=%d\n",
               heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
               heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
        if (print_real_time_stats(STATS_TICKS) == ESP_OK) {
            printf("Real-time stats collected.\n");
        } else {
            printf("Error collecting real-time stats.\n");
        }
        vTaskDelay(STATS_TICKS);
    }
}

static void sys_monitor_start(void) {
    xTaskCreatePinnedToCore(monitor_task, "Monitor Task", 4096, NULL, configMAX_PRIORITIES - 3, NULL, 0);
}
#endif

void init_ledc(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = 18, // Use a non-conflicting GPIO pin
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void voice_announcement_task(void *arg) {
    while (1) {
        xEventGroupWaitBits(event_group, LIGHT_ADJUST_EVENT, pdTRUE, pdFALSE, portMAX_DELAY);
        uint8_t level;
        xSemaphoreTake(lighting_mutex, portMAX_DELAY);
        level = light_set_conf.light_pwm;
        xSemaphoreGive(lighting_mutex);

        ESP_LOGI(TAG, "Announcing brightness level: %d%%", level);
        switch (level) {
            case 100: audio_handle_info(SOUND_TYPE_100); break;
            case 75:  audio_handle_info(SOUND_TYPE_75); break;
            case 50:  audio_handle_info(SOUND_TYPE_50); break;
            case 25:  audio_handle_info(SOUND_TYPE_25); break;
            default:  ESP_LOGW(TAG, "No audio mapped for brightness level: %d%%", level);
        }
    }
}

esp_err_t bsp_board_init(void) {
    ESP_ERROR_CHECK(bsp_led_init());
    ESP_ERROR_CHECK(bsp_spiffs_mount());
    return ESP_OK;
}

void app_main(void) {
    ESP_LOGI(TAG, "Compile time: %s %s", __DATE__, __TIME__);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    init_light_resources();
    bsp_display_start();
    init_ledc();

    ESP_LOGI(TAG, "Starting application...");
    bsp_display_unlock();

    xTaskCreate(voice_announcement_task, "Voice Announcement Task", 2048, NULL, 5, NULL);
    bsp_board_init();
    audio_play_start();

#if MEMORY_MONITOR
    sys_monitor_start();
#endif
}
