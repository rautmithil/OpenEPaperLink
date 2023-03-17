#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <time.h>

#include "contentmanager.h"
#include "flasher.h"
#include "makeimage.h"
#include "pendingdata.h"
#include "serial.h"
#include "settings.h"
#include "soc/rtc_wdt.h"
#include "tag_db.h"
#include "web.h"

void timeTask(void* parameter) {
    while (1) {
        time_t now;
        time(&now);
        tm tm;
        if (!getLocalTime(&tm)) {
            Serial.println("Waiting for valid time from NTP-server");
        } else {
            if (now % 10 == 0) wsSendSysteminfo();
            if (now % 30 == 3) Ping();
            if (now % 300 == 6) saveDB("/current/tagDB.json");

            contentRunner();
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.print(">\n");

    pinMode(ONBOARD_LED, OUTPUT);
    digitalWrite(ONBOARD_LED, HIGH);

    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "0.nl.pool.ntp.org", "europe.pool.ntp.org", "time.nist.gov");
    // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

    init_web();
    loadDB("/current/tagDB.json");

    xTaskCreate(zbsRxTask, "zbsRX Process", 10000, NULL, 2, NULL);
    xTaskCreate(garbageCollection, "pending-data cleanup", 5000, NULL, 1, NULL);
    xTaskCreate(webSocketSendProcess, "ws", 5000, NULL,configMAX_PRIORITIES-10, NULL);
    xTaskCreate(timeTask, "timed tasks", 10000, NULL, 2, NULL);
}

void loop() {
    vTaskDelay(30000 / portTICK_PERIOD_MS);
}