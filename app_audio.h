/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

typedef enum{
    SOUND_TYPE_KNOB,
    SOUND_TYPE_SNORE,
    SOUND_TYPE_WASH_END_CN,
    SOUND_TYPE_WASH_END_EN,
    SOUND_TYPE_FACTORY,
    SOUND_TYPE_100, // Audio for 100% brightness
    SOUND_TYPE_75,  // Audio for 75% brightness
    SOUND_TYPE_50,  // Audio for 50% brightness
    SOUND_TYPE_25   // Audio for 25% brightness
}PDM_SOUND_TYPE;
#include "esp_err.h" // Ensure this is included for esp_err_t
esp_err_t audio_force_quite(bool ret);

esp_err_t audio_handle_info(PDM_SOUND_TYPE voice);

esp_err_t audio_play_start();
