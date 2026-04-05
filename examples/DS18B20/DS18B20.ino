// Залил 10.12.2025 с добавлением порога изменения температуры для отправки данных
//  Вариант датчика температуры 18B20. Данные отправляются через интервал. Потом уходим в глубокий сон.
//  При неудачной отправке пытаемся отправить данные каждые ERROR_REPEAT_MINUTES с поиском Slave
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ESP_NOW_DEVICE.h>
#include "DS18B20.h"
#define ENERGY_SAVE // Закоментировать если хотим получать данные без задержки ( в 2 раза больше пробуждений) или оставить для запроса и получения предыдущей температуры в одно пробуждение
#define TIME_INTERVAL_MEASURE_MINETS 10
#define ERROR_REPEAT_MINUTES 5          // Интервал повторных отправок в случае неудачной доставки сообщения, в минутах
#define TEMPERATURE_DIIF_THRESHOLD 0.1f // порог изменения температуры для отправки
uint8_t broadcastAddress[6];            // мак адрес приемника что воткнут в розетку. Заполняется автоматически.
int8_t esp_channel;                     // канал роутера wifi. Заполняется автоматически.
uint8_t sendStatus = ESP_NOW_SEND_UNKNOW;
CONTROLLER Sensor(Temperature_Sensor, ERROR_REPEAT_MINUTES); // Создаем объект класса датчика  Можно добавить свой тип в constant.h cpp
DS18B20 termometr(GPIO_ID_PIN0);
static constexpr uint32_t rtc_mem_offset = 122;
ADC_MODE(ADC_VCC);
void OnDataSent_cb(uint8_t *mac_addr, uint8_t sendStatus) // Обратный вызов. Отчет о доставке сообщения. Необходимо библиотеке сообщить статус доставки для подсчетов и перехода в глубокий сон
{
  Sensor.sendStatus(sendStatus);
  ::sendStatus = sendStatus;
}

void setup()
{
  // Serial.begin(74880);
  delay(1000);
  if (ESP.getResetInfoPtr()->reason != REASON_DEEP_SLEEP_AWAKE)
  {
    termometr.request();
    WiFi.disconnect(); // удаляем  сохраненный в памяти модема SSID() и пароль
    wifi_station_set_auto_connect(false);
    wifi_station_set_reconnect_policy(false);
    ESP.deepSleep(750'000);
  }
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  HANDSHAKE(broadcastAddress, esp_channel);
  // Serial.printf("\n%x,%x,%x,%x,%x,%x,  chanel = %d",broadcastAddress[0], broadcastAddress[1], broadcastAddress[2],broadcastAddress[3], broadcastAddress[4], broadcastAddress[5], esp_channel);
#ifndef ENERGY_SAVE
  uint32_t bootcount;
  ESP.rtcUserMemoryRead(120, &bootcount, sizeof(bootcount));
  if (ESP.getResetInfoPtr()->reason == REASON_DEEP_SLEEP_AWAKE)
    bootcount++;
  else
    bootcount = 0;
  ESP.rtcUserMemoryWrite(120, &bootcount, sizeof(bootcount));

  if (bootcount % 2 == 0)
  {
    termometr.request();
    ESP.deepSleep(750'000);
  }
#endif

  float Temperature = termometr.getTemp();
  // Serial.printf("\ntemperature= %f", Temperature);

#define BETA_FUNCTION
#ifdef BETA_FUNCTION
  Temperature = roundf(Temperature * 10.0f) / 10.0f;
  // Добавил код энергосбережения без проверки в железе. Отправляет данніе только при изменении более чем на 0.1 градус:
  float lastTemperature;
  ESP.rtcUserMemoryRead(rtc_mem_offset, (uint32_t *)&lastTemperature, (size_t)sizeof(lastTemperature));

  if (!isfinite(lastTemperature) || fabs(Temperature - lastTemperature) >= TEMPERATURE_DIIF_THRESHOLD)
  {
    ESP.rtcUserMemoryWrite(rtc_mem_offset, (uint32_t *)&Temperature, (size_t)sizeof(Temperature));
  }
  else
  {
    termometr.request();
    Sleep_Minutes(TIME_INTERVAL_MEASURE_MINETS);
  }
  // Конец непроверенного кода
#endif

  Sensor.setMeasureValue(Temperature);
  // настраиваем ESPnow
  esp_now_init();
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);                               // Роль передатчика
  esp_now_register_send_cb(OnDataSent_cb);                                      // Назначаем обратную функцию
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, esp_channel, NULL, 0); // регистрируем пиры

  do
  {
    uint32_t startMillis = millis();
    esp_now_send(broadcastAddress, (uint8_t *)&Sensor.content, sizeof(Sensor.content)); // Отправляем данные на указанный мак broadcastAddress
    while (sendStatus == ESP_NOW_SEND_UNKNOW && millis() - startMillis < 50)            // Ждем 50 мс ответа.
      yield();
  } while ((sendStatus == ESP_NOW_SEND_FAIL || sendStatus == ESP_NOW_SEND_UNKNOW) && (millis() < 350)); // повторяем несколько раз отправку в случае неудачи.

  if (sendStatus == ESP_NOW_SEND_FAIL || sendStatus == ESP_NOW_SEND_UNKNOW)
  {
    HANDSHAKE(true);
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    if (sendStatus == ESP_NOW_SEND_UNKNOW)
      Sensor.sendStatus(ESP_NOW_SEND_FAIL);
#ifdef ENERGY_SAVE
    termometr.request();
#endif
    Sleep_Minutes(ERROR_REPEAT_MINUTES);
  }
  else
  {
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
#ifdef ENERGY_SAVE
    termometr.request();
#endif
    Sleep_Minutes(TIME_INTERVAL_MEASURE_MINETS);
  }
}

void loop() {}