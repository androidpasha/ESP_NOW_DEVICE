// Залил - 22.01.2026
//  Добавил разряд АКБ при заданном пороге. Правка в библиотеке BMx280 (закоментировал t_fine = 0; для температурной компенсации)

#include <ESP8266WiFi.h>
#include <espnow.h>
#include "ESP_NOW_DEVICE.h" //ЗАНИМАЕТ 6 блоков RTC памяти (24 байт) с начала для каждого экземпляра и 2 последних сектора Handshake (8 байт). Соответственно нельзя создавать больше 21 экземпляра CONTROLLER
#include "BMX280.h"

#define TIME_INTERVAL_MEASURE_MINUTES 5
#define error_repeat_minutes 1               // Интервал повторных отправок в случае неудачной доставки сообщения, в минутах
#define MAX_VCC 2900                         // Напряжение на ESP при полностью заряженном АКБ для его разрядки на случай перезаряда солнечной панелью
constexpr uint8_t swSCL = GPIO(5);           // 3 // Пины подключения датчика BMx280 IO3
constexpr uint8_t swSDA = GPIO(4);           // 1 // Пины подключения датчика BMx280
constexpr uint8_t MeasureLightPin = GPIO(2); // сюда фоторезистор и на +

volatile int8_t sendstatus = ESP_NOW_SEND_UNKNOW;
uint8_t broadcastAddress[6];
int8_t esp_channel;
CONTROLLER meteostation(Meteostation, error_repeat_minutes);
BMX280 BMP280(swSDA, swSCL);
MeteoststionVal measurementVal;

ADC_MODE(ADC_VCC);

volatile uint32_t RCtime1, RCtime2 = 0;

void IRAM_ATTR InterruptMeasureLightPin() { RCtime2 = micros(); } // ставим функцию в RAM по прерыванию в момент заряда конденсатора запоминаем время
void OnDataSent_cb(uint8_t *mac_addr, uint8_t sendstatus) { ::sendstatus = sendstatus; }

void MeasureLightVCC()
{
  /*-----Запуск измерения освещенности посредством времени RC цепи.--------*/
  pinMode(MeasureLightPin, OUTPUT); // разряжаем конденсатор
  delayMicroseconds(1500);
  RCtime1 = micros();                                                                         // запоминаем время начала заряда конденсатора
  pinMode(MeasureLightPin, INPUT);                                                            // начало заряда
  attachInterrupt(digitalPinToInterrupt(MeasureLightPin), InterruptMeasureLightPin, FALLING); // включаем прерывание которое сработает при заряде конденсатора до логической единицы
}

void setup()
{
  // Serial.begin(74880);
  MeasureLightVCC();
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  HANDSHAKE(broadcastAddress, esp_channel); // получаем адрес слейва и wifi канал

  esp_now_init();
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);                               // Роль передатчика
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, esp_channel, NULL, 0); // регистрируем пиры
  esp_now_register_send_cb(OnDataSent_cb);                                      // Назначаем обратную функцию
  if (BMP280.measureOk())
  {

    measurementVal.Temperature = (int16_t)(BMP280.readTemperature() * 100);
    measurementVal.Pressure = (uint32_t)BMP280.readPressure();

    if (!RCtime2)
    { // Если конденсатор не успел зарядится
      detachInterrupt(digitalPinToInterrupt(MeasureLightPin));
      RCtime2 = micros();
    }
    measurementVal.Lux = (uint16_t)(1000000 / (RCtime2 - RCtime1));
    meteostation.setMeasureValue(measurementVal);

    do
    {
      uint32_t startMillis = millis();
      esp_now_send(broadcastAddress, (uint8_t *)&meteostation.content, sizeof(meteostation.content)); // Отправляем данные на указанный мак broadcastAddress
      while (sendstatus == ESP_NOW_SEND_UNKNOW && millis() - startMillis < 50)                        // Ждем 50 мс ответа.
        yield();
    } while ((sendstatus == ESP_NOW_SEND_FAIL || sendstatus == ESP_NOW_SEND_UNKNOW) && (millis() < 350)); // повторяем несколько раз отправку в случае неудачи.
    if (sendstatus == ESP_NOW_SEND_FAIL)
      HANDSHAKE(true);
    meteostation.sendStatus(sendstatus);

    if (meteostation.content.Vcc > MAX_VCC) // Садим аккумулятор при перезаряде
    {
      WiFi.forceSleepWake();
      delay(1);
      WiFi.mode(WIFI_AP);
      WiFi.softAP("HIDDEN_AP", "12345678", 1, true, 1);
      pinMode(LED_BUILTIN, OUTPUT);
      digitalWrite(LED_BUILTIN, LOW);
      delay(TIME_INTERVAL_MEASURE_MINUTES * 60 * 1000);
      ESP.restart();
    }

    Sleep_Minutes(TIME_INTERVAL_MEASURE_MINUTES);
  }

  Sleep_Minutes(error_repeat_minutes);
}

void loop() {}