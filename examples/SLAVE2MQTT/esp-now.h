#pragma once
#include <Arduino.h>
#include <espnow.h>
#include <ESP8266WiFi.h>
#include <queue>
#include <constants.h>

#define MAX_QUEUE_SIZE 20 // маск хранящихся сообщений в очереди
#define MAC_SIZE 6        // размер MAC адрема

// const char *msgRequestMac = "get mac"; // Ключ запроса mac нового устройства для сопряжения
struct DewData                           // Структура которая будет хранится в очереди MQTT
{
    Content content;       // структура принятая от устройства по rsp-now
    uint8_t mac[MAC_SIZE]; // Для уникальности устройств
    bool newDev;           // Для регистрации устройств при их первом подключении
} dewData;

std::queue<DewData> MQTTqueue;                      // Очередь для последующей отправки по MQTT
std::queue<Content> TelegramQueue;                  // Очередь для последующей отправки по телеграм
std::vector<std::array<uint8_t, MAC_SIZE>> macList; // Тут храним мак адреса ранее подключенных устройств
// прототипы
void esp_now_setup();
void send_MyMAC(uint8_t *);
void OnDataRecv(uint8_t *, uint8_t *, uint8_t);

void esp_now_setup()
{
    if (esp_now_init() != ESP_NOW_ROLE_IDLE)
    {
        Serial.println(F("Error initializing ESP-NOW"));
        ESP.restart();
    }
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    esp_now_register_recv_cb(OnDataRecv);
}

void send_MyMAC(uint8_t *broadcastAddress) // Отправка мак и канала wifi устройству, которое его запросило
{
    int role = esp_now_get_self_role();        // Запоминаем режим
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO); // устанавливаем режим приемо/передатчика
    esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, WiFi.channel(), NULL, 0);
    uint8_t myMac[7];
    WiFi.macAddress(myMac);                               // Читаем в myMac свой адресс
    myMac[6] = WiFi.channel();                            // В последний байт кладем канал
    esp_now_send(broadcastAddress, myMac, sizeof(myMac)); // Отправляем свой адрес и канал
    esp_now_del_peer(broadcastAddress);
    esp_now_set_self_role(role); // возвращаем режим
}




void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
{
    if (len == sizeof(Content))
    {
        const Content &deviceContent  = *reinterpret_cast<const Content*>(incomingData);
        bool newDev = true;
        for (auto &n : macList)
            if (std::equal(std::begin(n), std::end(n), mac))
            {
                newDev = false;
                break;
            }
        if (newDev)
            macList.push_back({mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]});

        for (size_t i = 0; i < MAX_QUEUE_SIZE && MQTTqueue.size() > MAX_QUEUE_SIZE; i++) //удаляем старое из очереди если она большого размера не трогая новые устройства
        {
            if (MQTTqueue.front().newDev == true)
            {
                DewData tmp = MQTTqueue.front();
                MQTTqueue.pop();
                MQTTqueue.push(tmp);
                break;
            }
            else
                MQTTqueue.pop();
        }

        MQTTqueue.push({deviceContent, {mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]}, newDev});

        if (TelegramQueue.size() > MAX_QUEUE_SIZE)
            TelegramQueue.pop();

        if (deviceContent.deviceType <= Mail) // 2 Дверь, звонок, почта

            TelegramQueue.push(deviceContent);

        if (deviceContent.deviceType == Door_Open_Sensor)
            for (size_t i = 0; i < Users.size(); i++) // Прохожусь по всем пользователям
                if (newDay(&Users[i].Day))            // Если наступил новый день
                    Users[i].Predictions = true;      // флаг предсказаний
    }

    else if (len == strlen(msgRequestMac) + 1)// при запросе моего mac отправляю его
        if (strcmp((const char *)incomingData, msgRequestMac) == 0) //
            send_MyMAC(mac);
}