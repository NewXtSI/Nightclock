#include <Arduino.h>

#include "config.h"
#include <HTTPClient.h>
#include "WiFi.h"

extern EventGroupHandle_t   task_event;

void refreshDataFromIOBroker();
int getIOBrokerObject(const char *objectname, char *objectData);
int setIOBrokerObject(const char *objectname, const char *objectData);

bool lockIOBrokerData();
void unlockIOBrokerData();

WiFiClientSecure client;
HTTPClient http;

bool bSprayTest = false;

typedef struct {
    bool bNightMode;
    bool bPresence;
    bool bTv;
} iobrokerdata_t;

iobrokerdata_t      globalIOBrokerData;

void iobroker_task(void *param) {
    xEventGroupSetBits(task_event, TASK_IOBROKER_READY);
    #pragma message "TODO: SSL verification"
    client.setInsecure();
    while (1) {
        if (WiFi.isConnected()) {
            if (!bSprayTest) {
                setIOBrokerObject(IOBROKER_AROMA_BEDROOM, "10");
                bSprayTest = true;
            }
            delay(1000);
            refreshDataFromIOBroker();
        } else {
            delay(2000);
        }
    }
}

int setIOBrokerObject(const char *objectname, const char *objectData) {
    char requestURL[512];

    snprintf(requestURL, sizeof(requestURL), "%sset/%s?value=%s", IOBROKER_URL, objectname, objectData);

    http.begin(client, requestURL);
    int httpCode = http.GET();
    if (httpCode > 0) {
        Serial.printf("[HTTP] SET \"%s\" code: %d\n", requestURL, httpCode);
        http.end();
        if (httpCode != HTTP_CODE_OK) {
            return httpCode;
        }
        return 0;
    }
    http.end();
    return httpCode;

}

int getIOBrokerObject(const char *objectname, char *objectData) {
    char requestURL[512];

    snprintf(requestURL, sizeof(requestURL), "%sgetPlainValue/%s", IOBROKER_URL, objectname);

    http.begin(client, requestURL);
    int httpCode = http.GET();
    if (httpCode > 0) {
        Serial.printf("[HTTP] GET \"%s\" code: %d\n", requestURL, httpCode);
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.println(payload);
            sprintf(objectData, "%s", payload.c_str());
        }
        http.end();
        return 0;
    }
    http.end();
    return httpCode;
}

void refreshDataFromIOBroker() {
    char strData[256];
    getIOBrokerObject(IOBROKER_OBJECT_NIGHTMODE, strData);
    getIOBrokerObject(IOBROKER_OBJECT_PRESENCE_BEDROOM, strData);
    getIOBrokerObject(IOBROKER_OBJECT_TV_BEDROOM, strData);
}

bool nightmodeActive() {
    if (lockIOBrokerData) {
        bool bResult = globalIOBrokerData.bNightMode;
        unlockIOBrokerData();
        return bResult;
    }
    return false;
}

bool presenceBedroomDetected() {
    if (lockIOBrokerData) {
        bool bResult = globalIOBrokerData.bPresence;
        unlockIOBrokerData();
        return bResult;
    }
    return false;
}

bool tVBedroomSwitchedOn() {
    if (lockIOBrokerData) {
        bool bResult = globalIOBrokerData.bTv;
        unlockIOBrokerData();
        return bResult;
    }
    return false;
}

void nightmodeActive(bool bSwitch) {
    setIOBrokerObject(IOBROKER_OBJECT_NIGHTMODE, bSwitch ? "true" : "false");
    if (lockIOBrokerData) {
        globalIOBrokerData.bNightMode = bSwitch;
        unlockIOBrokerData();
    }
}

void tVBedroomSwitchedOn(bool bSwitch) {
    setIOBrokerObject(IOBROKER_OBJECT_TV_BEDROOM, bSwitch ? "true" : "false");
    if (lockIOBrokerData) {
        globalIOBrokerData.bTv = bSwitch;
        unlockIOBrokerData();
    }
}

SemaphoreHandle_t               ioBrokerSemaphore = NULL;

bool lockIOBrokerData() {
    if (ioBrokerSemaphore == NULL)
        ioBrokerSemaphore = xSemaphoreCreateMutex();
    if (xSemaphoreTake(ioBrokerSemaphore, (TickType_t)5000) == pdTRUE) {
        return true;
    } else {
        Serial.printf("%s could not get semaphore!\n", __PRETTY_FUNCTION__);
        return false;
    }

    return true;
}

void unlockIOBrokerData() {
    xSemaphoreGive(ioBrokerSemaphore);
}
