#include "constants.h"
namespace constants
{
  //собщение запроса mac для HANDSHAKE
  const char *msgRequestMac = "get_mac";

  const char *const devDescriptors[DEVICES_COUNT][5] PROGMEM = {
      // sensor name            , device name       , ед.изм      , dev class                   , "state_class"
      {"Датчик открытий"        , "Счетчик"         ,  ""         , "enum"                      ,"TOTAL"                 },
      {"Дверной звонок"         , "Нажатий"         ,  ""         , "enum"                      ,"TOTAL" },
      {"Датчик почты"           , "Почта"           ,  ""         , "enum"                      ,"TOTAL" },
      {"Датчик температуры"     , "Температура"     ,  "°C"       , "temperature"               ,"measurement"      },
      {"Датчик влажности"       , "Влажность"       ,  "%"        , "humidity"                  ,"measurement"      },
      {"Вольтметр"              , "Напряжение"      ,  "V"        , "voltage"                   ,"measurement"      },
      {"Датчик влажности почвы" , "Влажность почвы" ,  "%"        , "moisture"                  ,"measurement"},
      {"Устройство автополива"  , "Поливов"         ,  ""         , "enum"                      ,"TOTAL"},
      {"Атмосферное давление"   , "Давление"        ,  "Pa"       , "atmospheric_pressure"      ,"measurement"},
      {"Датчик света"           , "Освещенность"    ,  "lx"       , "illuminance"               ,"measurement"},
      {"Метеостанция"           , "Температура;Давление;Освещенность"    ,  "°C;Pa" , "temperature;atmospheric_pressure;illuminance", "measurement;measurement;measurement"},  
      {"esp-now gateway"        , "WiFi RSSI;Free heap;Подписчиков телеграм"    ,  "dB;B" , "signal_strength;data_size;enum", "measurement;measurement;TOTAL"},    
      {"Счетчик газа"           , "Счетчик"         ,  "m³"       , "gas"                      ,"total_increasing"},
  };
  //Типы данных измеряемых величин по датчикам
  // !!!!!!!!!! От этого массива нужно уходить. Вместо него использовать третий столбец devDescriptors
  const uint8_t valMeasurement[DEVICES_COUNT] PROGMEM = {BOOL, BOOL, BOOL, FLOAT, UIN_T32, FLOAT, UIN_T32, BOOL, BOOL, UIN_T32};

  // MQTT шаблоны отправки данных датчиков и топики
  const char *const MQTTtopicAndPayload[DEVICES_COUNT][2] = {
      {R"({"V": %.3f, "Val1": "%u"})"    ,     R"(esp/door_open_sensor/)"  },
      {R"({"V": %.3f, "Val1": "%u"})"    ,     R"(esp/door_ring/)"         },
      {R"({"V": %.3f, "Val1": "%u"})"    ,     R"(esp/mail/)"              },
      {R"({"V": %.3f, "Val1": "%.2f"})"  ,     R"(esp/temperature/)"       },
      {R"({"V": %.3f, "Val1": "%.1f"})"  ,     R"(esp/humidity/)"          },
      {R"({"V": %.3f, "Val1": "%.3f"})"  ,     R"(esp/voltmetr/)"          },
      {R"({"V": %.3f, "Val1": "%u"})"    ,     R"(esp/soil_moisture/)"     },
      {R"({"V": %.3f, "Val1": "%u"})"    ,     R"(esp/water_pump/)"        },
      {R"({"V": %.3f, "Val1": "%u"})"    ,     R"(esp/atm_pressure/)"      },
      {R"({"V": %.3f, "Val1": "%u"})"    ,     R"(esp/illuminance/)"       },
      {R"({"V": %.3f, "Val1": "%.2f", "Val2": "%u", "Val3": "%u"})", R"(esp/meteostation/)"},
      {R"({"V": %.3f, "Val1": "%d", "Val2": "%lu", "Val3": "%u"})", R"(esp/gateway/)"},
      {R"({"V": %.3f, "Val1": "%f"})"    ,     R"(esp/gas/)"            },
  };

  // MQTT discovery шаблоны
  const char discovery_header[] PROGMEM = {
      R"({
    "dev": {
    "ids": "ESP-now%s",
    "mdl": "Gateway",
    "name": "%s",
    "sw": "%s",
    "sn": "%s"
  },
  )"};

  const char discovery_body[] PROGMEM = {
      R"("o": {
    "name": "%s"
  },
  "cmps": {
    "V%s": {
      "name": "Напряжение",
      "p": "sensor",
      "dev_cla": "voltage",
      "unit_of_meas": "V",
      "state_class": "measurement",
      "val_tpl": "{{value_json.V }}",
      "uniq_id": "V%s"
    })"};

  const char discovery_sensor[] PROGMEM = {
      R"(,
      "sensor%d_%s": {
      "name": "%s",
      "p": "sensor",
      "dev_cla": "%s",
      "unit_of_meas": "%s",
      "state_class": "%s",
      "val_tpl": "{{value_json.Val%d }}",
      "uniq_id": "sensor%d_%s"
    })"};

  const char discovery_tail[] PROGMEM = {
      R"(
  },
  "stat_t": "%s%s",
  "qos": 1
  })"};

  // MQTT discovery шаблон топика
  const char configTopic[] PROGMEM = "homeassistant/device/%s/config";

  // Сообщение приветствия для телеграмм
  const char *const HelloMessage[DEVICES_COUNT] = {
      "Пришли гости!",
      "Дверной звонок звонит!",
      "Пришла почта!",
      "Датчик температуры.",
      "Вольтметр.",
      // "Появилось єлектричество",
      "Датчик влажности почвы.",
      "Автополив.",
      "Атмосферное давление",
      "Освещенность",
      "Метеостанция",
      "ESP-NOW шлюз",
      "Счетчик"
  };

  // void test_constants_h(){
  //   devDescriptors[8][4];

  //   constexpr size_t devDescriptors_rows = sizeof(devDescriptors) / sizeof(devDescriptors[0]);
  //   constexpr size_t MQTTtopicAndPayload_rows = sizeof(MQTTtopicAndPayload) / sizeof(MQTTtopicAndPayload[0]);
  //   constexpr size_t HelloMessage_rows = sizeof(HelloMessage) / sizeof(HelloMessage[0]);
  //   constexpr size_t measuring_rows = sizeof(measuring);
  //   constexpr size_t measuringFloat_rows = sizeof(measuringFloat);

  //   static_assert(
  //       (devDescriptors_rows == DEVICES_COUNT),
  //       "Ошибка: devDescriptors_rows != DEVICES_COUNT");
  //   static_assert(
  //       (MQTTtopicAndPayload_rows == DEVICES_COUNT),
  //       "Ошибка: MQTTtopicAndPayload_rows != DEVICES_COUNT");
  //   static_assert(
  //       (HelloMessage_rows == DEVICES_COUNT),
  //       "Ошибка: HelloMessage_rows != DEVICES_COUNT");
  //   static_assert(
  //       (measuring_rows == DEVICES_COUNT),
  //       "Ошибка: measuring_rows != DEVICES_COUNT");
  //   static_assert(
  //       (measuringFloat_rows == DEVICES_COUNT),
  //       "Ошибка: measuringFloat_rows != DEVICES_COUNT");
  // };
}