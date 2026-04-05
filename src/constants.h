#pragma once
#include <Arduino.h>

namespace constants
{
  // типы устройств
  enum DEVICES
  {
    Door_Open_Sensor,
    Door_Ring,
    Mail,
    Temperature_Sensor,
    Humidity_Sensor,
    Voltmetr,
    SoilMoistureSensor,
    Water_pump,
    Atmospheric_pressure,
    Illuminance,
    Meteostation,
    ESP_NOW_Gateway,
    Counter_Gas,

    DEVICES_COUNT // Всегда последний!
  };

  struct __attribute__((__packed__)) MeteoststionVal // структура полезных данных для передачи. Обязательно 8 байт
  {
    int16_t Temperature;
    uint16_t Lux;
    uint32_t Pressure; // 97728
  };

  struct __attribute__((__packed__)) GatewayVal // структура полезных данных для передачи. Обязательно 8 байт
  {
    int8_t RSSI;
    uint32_t FreeHeap;
    size_t sizeOfUsers;
  };

  // Типы данных поддерживаемых датчиками(прописаны в union в структуре Content)
  enum VALMEASUREMENT
  {
    BOOL,
    INT_32,
    UIN_T32,
    FLOAT,
    UINT_8_ARR_8
  };

  // названия ячеек массива devDescriptors
  enum DESCRIPTORS
  {
    DEV_NAME,
    SENSOR_NAME,
    UNIT,
    CLASS,
    STATE_CLASS,
  };
  // Структура, отправляется по ESP-NOW. Должна быть не больше 250 байт и быть кратной 32 битам (4байта)
  struct __attribute__((__packed__)) Content
  {
    uint8_t CRC8;
    uint8_t deviceType;                // тип этого датчика
    uint32_t NumberOfTransmitted : 24; // Счетчик переданных пакетов
    uint32_t delivery_Error : 24;      // Общее количество неудачных отправок
    uint32_t switch_awakening : 24;    // Количество срабатывания датчика
    uint32_t deepSleep_awakening : 24; // Количество перезагрузок вызванное deepSleep
    uint16_t interval_deep_sleep;      // Интервал повторных отправок в случае неудачной доставки сообщения, в минутах
    uint16_t SendDataTime;             // Время работы ESP при последней отправке
    uint16_t Vcc;                      // Напряжение питания
    uint8_t measureValues[224];

    uint8_t calculateCRC8()
    {
      const uint8_t *addr = (const uint8_t *)this + sizeof(CRC8);
      size_t length = sizeof(*this) - sizeof(CRC8);
      uint8_t crc = 0;
      while (length--)
      {
        uint8_t inbyte = *addr++;
        for (uint8_t i = 8; i; i--)
        {
          uint8_t mix = (crc ^ inbyte) & 0x01;
          crc >>= 1;
          if (mix)
            crc ^= 0x8C;
          inbyte >>= 1;
        }
      }
      return crc;
    }

    Content *operator&() // Перегрузка оператора взятия адреса для авторасчета crc8 перед отправкой по esp-now
    {
      CRC8 = calculateCRC8(); // пересчитать CRC перед выдачей адреса
      return this;            // вернуть корректный указатель
    }
  }; // размер 152 байт из 250 возможных. Количество бит должно быть кратно 8.

  extern const char *msgRequestMac;
  // sensorName, Датчик + devName, ед.изм, класс HA
  extern const char *const devDescriptors[DEVICES_COUNT][5] PROGMEM;

  // Типы данных измеряемых величин по датчикам
  extern const uint8_t valMeasurement[DEVICES_COUNT] PROGMEM;

  // MQTT шаблоны отправки данных датчиков и топики
  extern const char *const MQTTtopicAndPayload[DEVICES_COUNT][2];

  // MQTT discovery шаблоны для автонастройки в HA
  extern const char discovery_header[] PROGMEM;
  extern const char discovery_body[] PROGMEM;
  extern const char discovery_sensor[] PROGMEM;
  extern const char discovery_tail[] PROGMEM;

  // MQTT discovery шаблон топика
  extern const char configTopic[] PROGMEM;

  // Сообщение приветствия для телеграмм
  extern const char *const HelloMessage[DEVICES_COUNT];
}

// using namespace constants;