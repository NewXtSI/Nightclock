#define LGFX_AUTODETECT // Autodetect board
#define LGFX_USE_V1     // set to use new version of library

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include "config.h"

#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>

#include <lvgl.h>
#include "lv_conf.h"

static LGFX lcd; // declare display variable
AsyncWebServer server(80);

/*** Setup screen resolution for LVGL ***/

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];

/*** Function declaration ***/
void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);

#define WL_MAC_ADDR_LENGTH 6

void setup()
{
    Serial.begin(115200); /* prepare for possible serial debug */
    delay(200);

    static char wifiHostname[32];
    uint8_t mac[WL_MAC_ADDR_LENGTH];
    WiFi.macAddress(mac);
    snprintf(wifiHostname, sizeof(wifiHostname), "Nightclock-%02X%02X", mac[4], mac[5]);
    WiFi.hostname(wifiHostname);
    WiFi.mode(WIFI_STA);

    WiFi.begin(WIFI_SSID, WIFI_PSK);
    while (!WiFi.isConnected()) {
        delay(10);
    }
    Serial.printf("WiFi connected: %s -> %s\n", wifiHostname, WiFi.localIP().toString().c_str());

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

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hi!");
    });

    AsyncElegantOTA.begin(&server);    // Start ElegantOTA

    server.begin();
    Serial.println("HTTP server started");
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
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
