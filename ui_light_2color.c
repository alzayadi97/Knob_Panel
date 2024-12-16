#include "ui_light_2color.h"
#include "lvgl.h"
#include "lv_example_pub.h"
#include "lv_example_image.h"
#include "bsp/esp-bsp.h"
#include <stdio.h>

// Global variables
SemaphoreHandle_t lighting_mutex = NULL;
EventGroupHandle_t event_group = NULL;
light_set_attribute_t light_set_conf = {0}; // Global definition for external use

// File-specific static variables
static light_set_attribute_t light_xor = {0};
static time_out_count time_20ms, time_500ms;

// LVGL objects
static lv_obj_t *page;
static lv_obj_t *img_light_bg, *label_pwm_set;
static lv_obj_t *img_light_pwm_25, *img_light_pwm_50, *img_light_pwm_75, *img_light_pwm_100, *img_light_pwm_0;

// Event bit for brightness adjustment
#define LIGHT_ADJUST_EVENT (1 << 0)

// Images for light levels
static const ui_light_img_t light_image = {
    .img_bg = {&light_warm_bg, &light_cool_bg},
    .img_pwm_25 = {&light_warm_25, &light_cool_25},
    .img_pwm_50 = {&light_warm_50, &light_cool_50},
    .img_pwm_75 = {&light_warm_75, &light_cool_75},
    .img_pwm_100 = {&light_warm_100, &light_cool_100},
};

// LVGL Layer
lv_layer_t light_2color_Layer = {
    .lv_obj_name = "light_2color_Layer",
    .lv_obj_parent = NULL,
    .lv_obj_layer = NULL,
    .lv_show_layer = NULL,
    .enter_cb = light_2color_layer_enter_cb,
    .exit_cb = light_2color_layer_exit_cb,
    .timer_cb = light_2color_layer_timer_cb,
};

// Initialize lighting resources
void init_light_resources() {
    if (!lighting_mutex) {
        lighting_mutex = xSemaphoreCreateMutex();
        configASSERT(lighting_mutex);
    }
    if (!event_group) {
        event_group = xEventGroupCreate();
        configASSERT(event_group);
    }
}

// Update UI elements dynamically
static void update_light_ui() {
    // Hide all brightness levels initially
    lv_obj_add_flag(img_light_pwm_0, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(img_light_pwm_25, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(img_light_pwm_50, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(img_light_pwm_75, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(img_light_pwm_100, LV_OBJ_FLAG_HIDDEN);

    // Show the correct brightness level
    switch (light_set_conf.light_pwm) {
        case 100:
            lv_obj_clear_flag(img_light_pwm_100, LV_OBJ_FLAG_HIDDEN);
            break;
        case 75:
            lv_obj_clear_flag(img_light_pwm_75, LV_OBJ_FLAG_HIDDEN);
            break;
        case 50:
            lv_obj_clear_flag(img_light_pwm_50, LV_OBJ_FLAG_HIDDEN);
            break;
        case 25:
            lv_obj_clear_flag(img_light_pwm_25, LV_OBJ_FLAG_HIDDEN);
            break;
        case 0:
        default:
            lv_obj_clear_flag(img_light_pwm_0, LV_OBJ_FLAG_HIDDEN);
            break;
    }

    // Update the background image based on the temperature
    lv_img_set_src(img_light_bg, light_image.img_bg[light_set_conf.light_cck]);

    // Update the brightness label
    if (light_set_conf.light_pwm) {
        lv_label_set_text_fmt(label_pwm_set, "%d%%", light_set_conf.light_pwm);
    } else {
        lv_label_set_text(label_pwm_set, "--");
    }
}

// Event callback for handling user interactions
static void light_2color_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (LV_EVENT_FOCUSED == code) {
        lv_group_set_editing(lv_group_get_default(), true);
    } else if (LV_EVENT_KEY == code) {
        uint32_t key = lv_event_get_key(e);
        if (is_time_out(&time_500ms)) {
            if (xSemaphoreTake(lighting_mutex, portMAX_DELAY)) {
                if (LV_KEY_RIGHT == key && light_set_conf.light_pwm < 100) {
                    light_set_conf.light_pwm += 25;
                    xEventGroupSetBits(event_group, LIGHT_ADJUST_EVENT);
                } else if (LV_KEY_LEFT == key && light_set_conf.light_pwm > 0) {
                    light_set_conf.light_pwm -= 25;
                    xEventGroupSetBits(event_group, LIGHT_ADJUST_EVENT);
                }
                xSemaphoreGive(lighting_mutex);
                update_light_ui();
            }
        }
    } else if (LV_EVENT_CLICKED == code) {
        if (xSemaphoreTake(lighting_mutex, portMAX_DELAY)) {
            light_set_conf.light_cck = (light_set_conf.light_cck == LIGHT_CCK_WARM) ? LIGHT_CCK_COOL : LIGHT_CCK_WARM;
            xSemaphoreGive(lighting_mutex);
            update_light_ui();
        }
    }
}

// Initialize the UI components for the lighting control
void ui_light_2color_init(lv_obj_t *parent) {
    light_xor.light_pwm = 0xFF;
    light_xor.light_cck = LIGHT_CCK_MAX;

    light_set_conf.light_pwm = 50;
    light_set_conf.light_cck = LIGHT_CCK_WARM;

    page = lv_obj_create(parent);
    lv_obj_set_size(page, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_radius(page, 0, 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(page);

    img_light_bg = lv_img_create(page);
    lv_img_set_src(img_light_bg, &light_warm_bg);
    lv_obj_align(img_light_bg, LV_ALIGN_CENTER, 0, 0);

    label_pwm_set = lv_label_create(page);
    lv_obj_set_style_text_font(label_pwm_set, &HelveticaNeue_Regular_24, 0);
    if (light_set_conf.light_pwm) {
        lv_label_set_text_fmt(label_pwm_set, "%d%%", light_set_conf.light_pwm);
    } else {
        lv_label_set_text(label_pwm_set, "--");
    }
    lv_obj_align(label_pwm_set, LV_ALIGN_CENTER, 0, 65);

    img_light_pwm_0 = lv_img_create(page);
    lv_img_set_src(img_light_pwm_0, &light_close_status);
    lv_obj_add_flag(img_light_pwm_0, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(img_light_pwm_0, LV_ALIGN_TOP_MID, 0, 0);

    img_light_pwm_25 = lv_img_create(page);
    lv_img_set_src(img_light_pwm_25, &light_warm_25);
    lv_obj_align(img_light_pwm_25, LV_ALIGN_TOP_MID, 0, 0);

    img_light_pwm_50 = lv_img_create(page);
    lv_img_set_src(img_light_pwm_50, &light_warm_50);
    lv_obj_align(img_light_pwm_50, LV_ALIGN_TOP_MID, 0, 0);

    img_light_pwm_75 = lv_img_create(page);
    lv_img_set_src(img_light_pwm_75, &light_warm_75);
    lv_obj_add_flag(img_light_pwm_75, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(img_light_pwm_75, LV_ALIGN_TOP_MID, 0, 0);

    img_light_pwm_100 = lv_img_create(page);
    lv_img_set_src(img_light_pwm_100, &light_warm_100);
    lv_obj_add_flag(img_light_pwm_100, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(img_light_pwm_100, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_add_event_cb(page, light_2color_event_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(page, light_2color_event_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(page, light_2color_event_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(page, light_2color_event_cb, LV_EVENT_CLICKED, NULL);
    ui_add_obj_to_encoder_group(page);
}

// Callbacks for lv_layer_t
bool light_2color_layer_enter_cb(void *layer) {
    LV_LOG_USER("Layer Enter Callback");
    lv_layer_t *create_layer = layer;
    if (NULL == create_layer->lv_obj_layer) {
        create_layer->lv_obj_layer = lv_obj_create(lv_scr_act());
        lv_obj_remove_style_all(create_layer->lv_obj_layer);
        lv_obj_set_size(create_layer->lv_obj_layer, LV_HOR_RES, LV_VER_RES);
        ui_light_2color_init(create_layer->lv_obj_layer);
        set_time_out(&time_20ms, 20);
        set_time_out(&time_500ms, 200);
        return true;
    }
    return false;
}

bool light_2color_layer_exit_cb(void *layer) {
    LV_LOG_USER("Layer Exit Callback");
    bsp_led_rgb_set(0x00, 0x00, 0x00);
    return true;
}

void light_2color_layer_timer_cb(lv_timer_t *tmr) {
    feed_clock_time();
}
