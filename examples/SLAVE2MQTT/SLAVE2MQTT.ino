// Для загрузки файлов Нажать на морду муравья -> PROJECT TASKS -> esp01_1m -> Platform -> Upload Filesystem Image
// Залил 26.12.2025.
// добавил state_class в constant.cpp для плавности граффиков
//Добавил FIRMWARE_VERSION в discovery

#define FIRMWARE_VERSION "0.10b"
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <time.h>
#include <coredecls.h> //  для обратной функции при смене системного времени settimeofday_cb(time_is_set);
#include <FastBot2.h>
#include <ESP_NOW_DEVICE.h>
#include <vector>
// #include <MeasureTime.h>

#define FSH (const char *)F

#define timeoutSTA 20       // Секунд на попытку подключения к роутеру
#define timeoutAPclient 5   // Минут при подключенном клиенте
#define timeoutWaitClient 25 // Секунд ожидания клиента точкой доступа

const byte DNS_PORT = 53;
DNSServer dnsServer;                           // DNS сервер для перехода на главную страницу с любой другой
ESP8266WebServer webServer(80);                // Определяем объект и порт сервера для работы с HTTP
const char *wifiSettingsFile = "/config.json"; // Файл настроек сети, бота, WOL
const char *ssidAP = "esp-now gateway";        // Название генерируемой точки доступа

struct
{                                                                        // Структура, хранящаяся в RTC памяти
  char Local_ssid[32], Local_password[32], botToken[51], MACAddress[17]; // Переменные для подключения к wifi сети, WOL
  // String message;                                                        // Тревожное сообщение
  char mqtt_server[32];
  u16 mqtt_port;
  char mqtt_user[20], mqtt_pass[32];
} settings;

ADC_MODE(ADC_VCC);
// FS
const char *settingsFile PROGMEM = "/settings.bin";       // в этом файле храним настройки пользователей telegram
const char *dictionary_path PROGMEM = "/predictions.txt"; // Файл с предсказаниями на каждый день

FastBot2 bot;

WiFiClient client;
// Настройка зоны времени
const int8_t timezone = 2;
const uint8_t daySaveTime = 1; // Летнее время 3600, иначе 0

struct userData
{                              // класс пользователей
  int64_t chatId = 0;          // чат id
  bool FirstStart = true;      // Флаги первой загрузки
  bool Predictions = false;    // Флаги предсказаний
  bool Do_not_disturb = false; // Флаги "Не беспокоить"
  bool MasterUser = false;
  uint16_t Day = 0; // День года.
};
// Переменные и объекты
std::vector<userData> Users;
bool mqtt_enable = false;      // Переменная проверки настроек mqtt. Не менять!
uint8_t deviseTipe = 0xFF;     // тип датчиков которые будут присылать esp-now состояние
uint16_t dictionary_lines = 0; // Размер словаря предсказаний
time_t startTime = 0;
void time_is_set(bool from_sntp);
// Файлы проекта
#include "this_Time.h"
#include "WebServer.h"
#include "WIFI.h"
#include "WOL.h"
#include "LFS.h"
#include "esp-now.h"
#include "Telegramm.h"
#include "mqtt.h"

void setup()
{
  Serial.begin(74880);
  delay(1000);
  Serial.printf(FSH("\n\nEsp-now to MQTT - Telegram gateway.\nVersion %s, build %s %s\n\n"), FIRMWARE_VERSION, __DATE__, __TIME__);

  if (!LittleFS.begin())
    ESP.restart();
  // Выставляем режим работы WiFi
  WifiAP();
  wifiConnect();
  esp_now_setup();
  ReadLFS(settingsFile, Users); // Читаем параметры пользователей из ФС, количество пользователей
  for (uint8_t i = 0; i < Users.size(); i++)
    Users[i].FirstStart = true; // Обновляем флаги первого включения и предсказаний для каждого пользователя
  prinUserInfo();
  configTime(3600 * timezone, daySaveTime * 3600, F("time.google.com"), F("time.windows.com"), F("pool.ntp.org"));
  settimeofday_cb(time_is_set); // Обратная функция при обновлении времени
  // Определяем количество предсказаний в словаре
  dictionary_lines = number_of_lines(dictionary_path);
  // Конфиг телеграм бота
  setlocale(LC_ALL, "ru");
  bot.attachUpdate(TelegramDialog);
  bot.setToken(settings.botToken);
  bot.setPollMode(fb::Poll::Long, 40000);

  // Настройка MQTT
  if (settings.mqtt_server[0] != '\0' && settings.mqtt_port != 0 && settings.mqtt_user[0] != '\0')
  {
    mqtt_enable = true;
    Serial.printf(FSH("mqtt setup %s\n"), mqttSetup() ? F("OK") : F("fail"));
  }
  else
    Serial.printf(FSH("MQTT settings wrong! mqtt_server: %s, mqtt_port %d, mqtt_user %s, mqtt_pass: %s"), settings.mqtt_server, settings.mqtt_port, settings.mqtt_user, settings.mqtt_pass);
}

void loop()
{

  MQTTloop();
  telegramLoop();
}
