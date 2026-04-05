#pragma once
#include <constants.h>
using constants::Content;


enum DELIVERY_STATUS // Перечисление статусов доставки сообщения ESP-NOW
{
  ESP_NOW_SEND_SUCCESS,
  ESP_NOW_SEND_FAIL,
  ESP_NOW_SEND_UNKNOW
};

class CONTROLLER // Класс передатчика (сенсора)
{
public:
  Content content;
  CONTROLLER() = delete;
  CONTROLLER(uint8_t device_type, uint8_t interval_deep_sleep, bool deepSleep_enable = false);

  template <typename T>
  bool setMeasureValue(const T &value)
  {
    if (sizeof(T) > sizeof(this->content.measureValues))
      return false;
    memcpy(this->content.measureValues, &value, sizeof(T));
    content.CRC8 = content.calculateCRC8();
    return true;
  }

  template <typename T, size_t N>
  bool setMeasureValue(const T (&bytes)[N])
  {
    if (N * sizeof(T) > sizeof(this->content.measureValues))
      return false;
    memcpy(this->content.measureValues, bytes, N * sizeof(T));
    content.calculateCRC8();
    return true;
  }

  void sendStatus(bool sendStatus); // Получаем информацию о доставке сообщения
  void RTCupdate();
  bool deepSleepReason();

private:
  struct rtcData
  { // Структура которая будет хранится в памяти RTC. Каждому экземпляру своя отдельная структура.
    uint8_t CRC8;
    uint8_t deepSleepReason;      // Храним причину ухода в сон.
    uint32_t NumberOfTransmitted; // Счетчик переданных пакетов
    uint32_t delivery_Error;      // Общее количество неудачных отправок
    uint32_t switch_awakening;    // Количество срабатывания датчика
    uint32_t deepSleep_awakening; // Количество перезагрузок вызванное deepSleep
    uint16_t SendDataTime;        // Время работы ESP при последней отправке
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
  } rtcData; // Кратное 32! Блоки RTC по 4 байта

  bool deepSleep_enable;
  uint8_t objectID;
  uint8_t generateObjectID();

  void RTCread();
};
