#define LGFX_AUTODETECT // Autodetect board
#define LGFX_USE_V1     // set to use new version of library

#include <Arduino.h>

#include "config.h"

#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>

#include <lvgl.h>
#include "lv_conf.h"

#include <ezTime.h>

/*** Setup screen resolution for LVGL ***/

static LGFX lcd; // declare display variable

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];

extern EventGroupHandle_t   task_event;
extern Timezone nightclockTZ;


/*** Function declaration ***/
void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);

lv_obj_t *lvClockLabel = NULL;
lv_obj_t *nightmodeButton = NULL;
lv_obj_t *tvButton = NULL;

void lv_clockTimer(lv_timer_t *timer) {
    char clocktext[20];
    sprintf(clocktext, "%02d:%02d", nightclockTZ.hour(), nightclockTZ.minute());
    lv_label_set_text(lvClockLabel, clocktext);
}

void ui_task(void *param) {
    lcd.init();  // Initialize LovyanGFX
    lcd.setBrightness(0);

    lv_init();   // Initialize lvgl

    // Setting display to landscape
    if (lcd.width() < lcd.height())
        lcd.setRotation(lcd.getRotation() ^ 1);

    /* LVGL : Setting up buffer to use for display */
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

    /*** LVGL : Setup & Initialize the display device driver ***/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = display_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*** LVGL : Setup & Initialize the input device driver ***/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    xEventGroupSetBits(task_event, TASK_UI_READY);

    lv_obj_t *mainscreen = lv_obj_create(nullptr);
    lvClockLabel = lv_label_create(mainscreen);
    lv_obj_set_pos(lvClockLabel, 20, 20);

    nightmodeButton = lv_btn_create(mainscreen);
    lv_obj_set_pos(nightmodeButton, screenWidth-20-64,20);
    lv_obj_set_size(nightmodeButton, 64, 64);
    lv_obj_add_flag(nightmodeButton, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_border_color(nightmodeButton, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_color(nightmodeButton, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_color(nightmodeButton, lv_color_hex(0xff0000), LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(nightmodeButton, lv_color_hex(0xff0000), LV_STATE_CHECKED);
    lv_obj_set_style_border_color(nightmodeButton, lv_color_hex(0x0000ff), LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(nightmodeButton, lv_color_hex(0x0000ff), LV_STATE_PRESSED);
    lv_obj_add_event_cb(
        nightmodeButton,
        [](lv_event_t *event) {
        bool bActive;

        lv_obj_t *obj = lv_event_get_target(event);
        if (lv_obj_get_state(obj) & LV_STATE_CHECKED) {
            nightmodeActive(true);
        } else {
            nightmodeActive(false);
        }
      },
      LV_EVENT_VALUE_CHANGED, nightmodeButton);

    tvButton = lv_btn_create(mainscreen);
    lv_obj_set_pos(tvButton, screenWidth-20-64, 92);
    lv_obj_set_size(tvButton, 64, 64);
    lv_obj_add_flag(tvButton, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_border_color(tvButton, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_color(tvButton, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_color(tvButton, lv_color_hex(0xff0000), LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(tvButton, lv_color_hex(0xff0000), LV_STATE_CHECKED);
    lv_obj_set_style_border_color(tvButton, lv_color_hex(0x0000ff), LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(tvButton, lv_color_hex(0x0000ff), LV_STATE_PRESSED);
    lv_obj_add_event_cb(
        tvButton,
        [](lv_event_t *event) {
        bool bActive;

        lv_obj_t *obj = lv_event_get_target(event);
        if (lv_obj_get_state(obj) & LV_STATE_CHECKED) {
            tVBedroomSwitchedOn(true);
        } else {
            tVBedroomSwitchedOn(false);
        }
      },
      LV_EVENT_VALUE_CHANGED, tvButton);

    lv_obj_t *spraybutton = lv_btn_create(mainscreen);
    lv_obj_set_pos(spraybutton, screenWidth-20-64, 92+72);
    lv_obj_set_size(spraybutton, 64, 64);
    lv_obj_set_style_border_color(spraybutton, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_color(spraybutton, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_color(spraybutton, lv_color_hex(0x0000ff), LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(spraybutton, lv_color_hex(0x0000ff), LV_STATE_PRESSED);
    lv_obj_add_event_cb(
        spraybutton,
        [](lv_event_t *event) {
        bedroomAromaspray(3);
      },
      LV_EVENT_CLICKED, spraybutton);

    lv_scr_load(mainscreen);

    lv_timer_t *timer = lv_timer_create(lv_clockTimer, 800, NULL);

    while (1) {
        lv_timer_handler(); /* let the GUI do its work */
        const portTickType xDelay = 50 / portTICK_RATE_MS;
        vTaskDelay(xDelay);
        if (nightmodeActive()) {
            if ((lv_obj_get_state(nightmodeButton) & LV_STATE_CHECKED) != LV_STATE_CHECKED) {
                lv_obj_add_state(nightmodeButton, LV_STATE_CHECKED);
            }
            lcd.setBrightness(5);
        } else {
            if (lv_obj_get_state(nightmodeButton) & LV_STATE_CHECKED) {
                lv_obj_clear_state(nightmodeButton, LV_STATE_CHECKED);
            }
            lcd.setBrightness(100);
        }
        if (tVBedroomSwitchedOn()) {
            if ((lv_obj_get_state(tvButton) & LV_STATE_CHECKED) != LV_STATE_CHECKED) {
                lv_obj_add_state(tvButton, LV_STATE_CHECKED);
            }
        } else {
            if (lv_obj_get_state(tvButton) & LV_STATE_CHECKED) {
                lv_obj_clear_state(tvButton, LV_STATE_CHECKED);
            }
        }
    }

    vTaskDelete(NULL);
}

/*** Display callback to flush the buffer to screen ***/
void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    lcd.startWrite();
    lcd.setAddrWindow(area->x1, area->y1, w, h);
    lcd.pushColors((uint16_t *)&color_p->full, w * h, true);
    lcd.endWrite();

    lv_disp_flush_ready(disp);
}

/*** Touchpad callback to read the touchpad ***/
void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX, touchY;
    bool touched = lcd.getTouch(&touchX, &touchY);

    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;

        // Serial.printf("Touch (x,y): (%03d,%03d)\n",touchX,touchY );
    }
}

