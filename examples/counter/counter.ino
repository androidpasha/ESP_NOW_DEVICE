#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ESP_NOW_DEVICE.h>
#include "HANDSHAKE.h"
#define DEVIDER 10                                  // Делитель передачи данных импульсов. Например слать каждый 10й импульс
CONTROLLER COUNTER(Counter_Gas, 0); // Создаем объект класса датчика
ADC_MODE(ADC_VCC);
void setup()
{
  COUNTER.sendStatus(ESP_NOW_SEND_SUCCESS);
  if (COUNTER.content.switch_awakening % (DEVIDER) == 0)
  {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    system_phy_set_max_tpw(82);               // 0..82/ Set maximum value of RF TX Power, unit : 0.25dBm
    uint8_t broadcastAddress[6];              // мак адрес приемника что воткнут в розетку
    int8_t esp_channel = -1;                  // канал роутера wifi
    HANDSHAKE(broadcastAddress, esp_channel); // поиск slave с перезагрузкой при неудаче
    if (esp_now_init() != ESP_NOW_ROLE_IDLE || esp_channel == -1)
      system_deep_sleep_instant(0);                                          // при сбое перезагрузка
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);                                       // Роль передатчика
    esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, esp_channel, NULL, 0);         // регистрируем пиры
    esp_now_send(broadcastAddress, (uint8_t *)&COUNTER.content, sizeof(COUNTER.content)); // Отправляем данные на указанный мак broadcastAddress
  }
  system_deep_sleep_set_option(RF_NO_CAL);
  system_deep_sleep(0);
}
void loop() {}