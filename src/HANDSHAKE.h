
//Библиотека занимает  126-127 ячейки RTC (конец памяти)
#pragma once
#include "Arduino.h"
#include <espnow.h>
#include <ESP8266WiFi.h>
//const char *constants::msgRequestMac; // кодовое слова запроса мак адреса

class HANDSHAKE
{
private:
    static constexpr size_t rtc_size = 512;
    inline static const uint8_t _broadcas[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    static constexpr uint8_t _maxWifiChanel = 13;
    static constexpr uint32_t rtc_mem_offset = ((rtc_size - sizeof(_broadcas) + sizeof(uint8_t)) / 4);
    // inline static const char *msgRequestMac = "get_mac";
    inline static uint8_t slaveAddressChanal[8] = {0};
    inline static bool getMac = false;

public:
    HANDSHAKE() = delete;
    HANDSHAKE(bool forcibly)
    {
        if (forcibly == true)
        {
            uint8_t mac[6];
            int8_t chanal = -1;
            HANDSHAKE(mac, chanal, true);
        }
    }

    HANDSHAKE(uint8_t *mac, int8_t &chanel, bool forcibly = false)
    {
        chanel = -1;
        if (ESP.getResetInfoPtr()->reason == REASON_DEEP_SLEEP_AWAKE || ESP.getResetInfoPtr()->reason == REASON_SOFT_RESTART)
        {
            // Serial.println("Читаю mac из RTC mamory");
            uint8_t tmp[8];
            ESP.rtcUserMemoryRead(rtc_mem_offset, (uint32_t *)tmp, (size_t)sizeof(tmp));
            memcpy(slaveAddressChanal, tmp, sizeof(slaveAddressChanal));
            chanel = slaveAddressChanal[6];
            // Serial.printf("\nmac lib %02x:%02x:%02x:%02x:%02x:%02x, chanal = %d\n", slaveAddressChanal[0], slaveAddressChanal[1], slaveAddressChanal[2], slaveAddressChanal[3], slaveAddressChanal[4], slaveAddressChanal[5], chanel);
        }
        if (chanel < 0 || chanel > _maxWifiChanel || forcibly == true)
        {
            // Serial.printf("\nSHAKE\n");
            esp_now_init();
            int role = esp_now_get_self_role();
            getMac = false;
            uint32_t start_time = millis();
            esp_now_set_self_role(ESP_NOW_ROLE_COMBO); // устанавливаем режим приемо/передатчика
            esp_now_register_recv_cb(this->onMacRecv_cb);

            for (size_t WifiChanel = 1; WifiChanel <= _maxWifiChanel; WifiChanel++) // Отправляем по всем каналам запрос мак.
            {
                esp_now_add_peer((uint8_t *)_broadcas, ESP_NOW_ROLE_COMBO, WifiChanel, NULL, 0);
                delay(3);
                esp_now_send((uint8_t *)_broadcas, (uint8_t *)msgRequestMac, strlen(msgRequestMac) + 1);
                esp_now_del_peer((uint8_t *)_broadcas);
                if (getMac == true)
                    break;
            }
            while (getMac != true)
            {
                delay(1);
                if (millis() - start_time > 200)
                {
                    if (forcibly == true)
                    {
                        return;
                    }
                    else
                    {
                        ESP.deepSleep(10'000'000, RF_NO_CAL);
                    }
                }
            }
            uint8_t tmp[8];
            memcpy(tmp, slaveAddressChanal, sizeof(slaveAddressChanal));
            ESP.rtcUserMemoryWrite(rtc_mem_offset, (uint32_t *)tmp, (size_t)sizeof(tmp));
            esp_now_unregister_recv_cb();
            esp_now_set_self_role(role);
            esp_now_deinit();
        }
        if (forcibly == false)
        {
            memcpy(mac, slaveAddressChanal, 6);
            chanel = slaveAddressChanal[6];
        }
    }

    // Callback от ESP-NOW
    static void onMacRecv_cb(uint8_t *mac, uint8_t *data, uint8_t len)
    {
        if (len == sizeof(_broadcas) + sizeof(_maxWifiChanel)) // 7 байт прийдет. Первые 6 это мак последний - канал
            if (memcmp(mac, data, 6) == 0)                     // проверяем что во входящем сообщение есть mac slav-a
            {
                memcpy(slaveAddressChanal, data, len);
                getMac = true;
            }
    }
};