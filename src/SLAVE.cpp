#include <Arduino.h>
#include <constants.h>
#include "SLAVE.h"

enum
{
  PAYLOAD,
  TOPIC
};

SLAVE::SLAVE(uint8_t *incomingData)
{
  memcpy(&content, incomingData, sizeof(content));
}
SLAVE::SLAVE(Content &incomingData)
{
  content = incomingData;
}

uint8_t SLAVE::deviceTipe() // Отправляет номер типа устройства в соответствии с enum DEVICES
{
  if (content.CRC8 == content.calculateCRC8())
    return this->content.deviceType;
  else
    return 0xFF;
}

void SLAVE::resultToMQTTString(String &topic, String &payload)
{
  topic = MQTTtopicAndPayload[deviceTipe()][TOPIC];
  char str[200];
  switch (deviceTipe())
  {
  case Door_Ring:
  case Mail:
  case Water_pump:
  case Door_Open_Sensor:
    sprintf(str, MQTTtopicAndPayload[content.deviceType][PAYLOAD],
            content.Vcc / 1000.0,
            content.switch_awakening);
    break;
  case Temperature_Sensor:
    sprintf(str, MQTTtopicAndPayload[content.deviceType][PAYLOAD],
            content.Vcc / 1000.0,
            *reinterpret_cast<float *>(content.measureValues));
    break;
  case Humidity_Sensor:
    sprintf(str, MQTTtopicAndPayload[content.deviceType][PAYLOAD],
            content.Vcc / 1000.0,
            *reinterpret_cast<int32_t *>(content.measureValues));
    break;
  case Voltmetr:
    sprintf(str, MQTTtopicAndPayload[content.deviceType][PAYLOAD],
            content.Vcc / 1000.0,
            *reinterpret_cast<float *>(content.measureValues));
    break;
  case ESP_NOW_Gateway:
    GatewayVal Val;
    memcpy((uint8_t *)&Val, (uint8_t *)content.measureValues, sizeof(Val));
    sprintf(str, MQTTtopicAndPayload[content.deviceType][PAYLOAD],
            content.Vcc / 1000.0,
            Val.RSSI,
            Val.FreeHeap,
            Val.sizeOfUsers);
    break;
  case SoilMoistureSensor:
    sprintf(str, MQTTtopicAndPayload[content.deviceType][PAYLOAD],
            content.Vcc / 1000.0,
            *reinterpret_cast<uint32_t *>(content.measureValues));
    break;
  case Meteostation:
    MeteoststionVal meas;
    memcpy((uint8_t *)&meas, (uint8_t *)content.measureValues, sizeof(meas));
    sprintf(str, MQTTtopicAndPayload[content.deviceType][PAYLOAD],
            content.Vcc / 1000.0,
            meas.Temperature / 100.0,
            meas.Pressure,
            meas.Lux);
    break;
  case Counter_Gas:
    sprintf(str, MQTTtopicAndPayload[content.deviceType][PAYLOAD],
            content.Vcc / 1000.0,
            content.switch_awakening/100.0);
    break;
  default:
    strcpy(str, (char *)F(R"({"devicetipe": "error"})"));
    topic = F("error");
    break;
  } 
  payload = str;
}

void SLAVE ::resultToString(String &result) // Добавляет к строке описания событий датчиков
{
  uint8_t nowCRC8 = content.CRC8;
  if (nowCRC8 == content.calculateCRC8())
  {
    if (content.deviceType >= Temperature_Sensor)
    {
      result = "";
      return;
    }

    result = F("№");
    result += String(content.switch_awakening); // №0.
    result += F(". ");
    result += String(HelloMessage[this->content.deviceType]); // Датчик температуры.

    switch (valMeasurement[content.deviceType])
    {
    case INT_32:
      result += F("\nПоказания: ");
      result += String(*reinterpret_cast<int32_t *>(content.measureValues));
      break;
    case UIN_T32:
      result += F("\nПоказания: ");
      result += String(*reinterpret_cast<uint32_t *>(content.measureValues));
      break;
    case FLOAT:
      result += F("\nПоказания: ");
      result += String(*reinterpret_cast<float *>(content.measureValues));
      break;
    case UINT_8_ARR_8:
      result += F("\nПоказания: ");
      result += reinterpret_cast<const char *>(content.measureValues);
      result += ", ";
      break;
    default:
      break;
    }

    if (pgm_read_byte(&valMeasurement[content.deviceType]) == FLOAT)
    {
      result += F("\nПоказания: ");
      result += String(*reinterpret_cast<float *>(content.measureValues)); // Показания: 5
    }

    if (strlen(devDescriptors[content.deviceType][UNIT]))
    {
      result += " ";
      result += String(devDescriptors[content.deviceType][UNIT]); // `C.
      result += ".";
    }
    result += F("\nНапряжение батареек: ") + String(content.Vcc / 1000.0, 3) + F(" В.\n"); // Напряжение батареек 3.428 В
    if (content.Vcc < 2500)
      result += F("Пора заменить батарейки. Скоро перестану работать");
    if (content.delivery_Error > 0)
    {
      result += F("\nВнимание!\nДанные были актуальны ");
      result += String(content.delivery_Error * content.interval_deep_sleep);
      result += F(" мин. назад.\nКоличество срабатываний без оповещения: ");
      result += String(content.delivery_Error);
      result += F(".\nВсего сбоев передачи: ");
      result += String(content.deepSleep_awakening);
      result += F(".");
    }
  }
  else
  {
    result = F("\n") + String(HelloMessage[content.deviceType]);
    result += F("CRC8 ERROR!\n") + String(nowCRC8) + F(" NOT= ") + String(content.CRC8);
  }
}