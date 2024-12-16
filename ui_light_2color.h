#ifndef UI_LIGHT_H
#define UI_LIGHT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "lvgl.h"

// Enum for light settings
typedef enum {
    LIGHT_CCK_WARM,
    LIGHT_CCK_COOL,
    LIGHT_CCK_MAX,
} LIGHT_CCK_TYPE;

// Struct for light attributes
typedef struct {
    uint8_t light_pwm;
    LIGHT_CCK_TYPE light_cck;
} light_set_attribute_t;

// Struct for light images
typedef struct {
    const lv_img_dsc_t *img_bg[2];
    const lv_img_dsc_t *img_pwm_25[2];
    const lv_img_dsc_t *img_pwm_50[2];
    const lv_img_dsc_t *img_pwm_75[2];
    const lv_img_dsc_t *img_pwm_100[2];
} ui_light_img_t;

// Extern variables
extern SemaphoreHandle_t lighting_mutex;
extern EventGroupHandle_t event_group;
extern light_set_attribute_t light_set_conf;

// Event bit for brightness adjustment
#define LIGHT_ADJUST_EVENT (1 << 0)

// Function declarations
void init_light_resources();
void ui_light_2color_init(lv_obj_t *parent);

// Callbacks for lv_layer_t
bool light_2color_layer_enter_cb(void *layer);
bool light_2color_layer_exit_cb(void *layer);
void light_2color_layer_timer_cb(lv_timer_t *tmr);

#ifdef __cplusplus
}
#endif

#endif // UI_LIGHT_H
