#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include "config.h"

// Tasks
void ui_task(void *param);

AsyncWebServer server(80);
EventGroupHandle_t      task_event;

TaskHandle_t            ui_taskhandle;

#define WL_MAC_ADDR_LENGTH 6

void setup()
{
    task_event = xEventGroupCreate();

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

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hi!");
    });

    xTaskCreatePinnedToCore(ui_task, "ui_task", 1024 * 6, NULL, 3, &ui_taskhandle, 1);
    AsyncElegantOTA.begin(&server);    // Start ElegantOTA

    xEventGroupWaitBits(
        task_event,
        (TASK_UI_READY),
        pdFALSE, pdTRUE, 6500000);

    server.begin();
    Serial.println("HTTP server started");
}

void loop()
{
}

