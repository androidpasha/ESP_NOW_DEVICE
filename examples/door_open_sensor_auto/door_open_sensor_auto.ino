#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ESP_NOW_DEVICE.h>
#include "HANDSHAKE.h"
uint8_t status = ESP_NOW_SEND_UNKNOW;
uint8_t broadcastAddress[6];                                          // мак адрес приемника что воткнут в розетку
int8_t esp_channel = -1;                                              // канал роутера wifi
#define repeat_minutes 5                                              // Интервал повторных отправок в случае неудачной доставки сообщения, в минутах
#define deep_sleep true                                               // Уходить в сон автоматически функциями библиотеки после отчета о доставке
CONTROLLER DOOR_SENSOR(Door_Open_Sensor, repeat_minutes, deep_sleep); // Создаем объект класса датчика с типом VCC_logger. Можно без последней переменной deepSleep_enable, тогда укладывать спать библиотека не будет.
ADC_MODE(ADC_VCC);

void OnDataSent_cb(uint8_t *mac_addr, uint8_t sendStatus) { ::status = sendStatus; } // Получаю отчет о доставке

void setup()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  HANDSHAKE(broadcastAddress, esp_channel);//поиск slave с перезагрузкой при неудаче
  if (esp_now_init() != ESP_NOW_ROLE_IDLE)
    ESP.deepSleep(repeat_minutes * 60ul * 1'000'000ul); // при сбое перезагрузка
  if (esp_channel == -1)
    ESP.restart();

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);                                               // Роль передатчика
  esp_now_register_send_cb(OnDataSent_cb);                                                      // Назначаем обратную функцию
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, esp_channel, NULL, 0);                 // регистрируем пиры
  esp_now_send(broadcastAddress, (uint8_t *)&DOOR_SENSOR.content, sizeof(DOOR_SENSOR.content)); // Отправляем данные на указанный мак broadcastAddress
}
void loop()
{
  if (status == ESP_NOW_SEND_SUCCESS)
    DOOR_SENSOR.sendStatus(status);
  else if (millis() > 300)
  {
    HANDSHAKE(true); // Принудительный одноразовый поиск slave без перезагрузки
    DOOR_SENSOR.sendStatus(ESP_NOW_SEND_FAIL);
  }
}