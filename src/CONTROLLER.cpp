#include <Arduino.h>
#include <CONTROLLER.h>
CONTROLLER::CONTROLLER(uint8_t device_type, uint8_t interval_deep_sleep, bool deepSleep_enable) // 244 экземпляра max
{
  objectID = generateObjectID(); // Создаем идентификатор экземпляра класса от 0 до количества экземпляров
  RTCread();                     // Читаем RTC память выделенную для каждого экземпляра отдельно.
  this->content.NumberOfTransmitted = this->rtcData.NumberOfTransmitted;
  this->content.delivery_Error = this->rtcData.delivery_Error;
  this->content.switch_awakening = this->rtcData.switch_awakening;
  this->content.deepSleep_awakening = this->rtcData.deepSleep_awakening;
  this->content.SendDataTime = this->rtcData.SendDataTime;
  this->content.Vcc = ESP.getVcc();
  this->content.deviceType = device_type;
  this->content.interval_deep_sleep = interval_deep_sleep;
  this->content.CRC8 = this->content.calculateCRC8();
  this->deepSleep_enable = deepSleep_enable;
}

uint8_t CONTROLLER::generateObjectID() // Генерируем номер объекта 0....1....2...3....
{
  static uint8_t id = 0;
  return id++;
}

void CONTROLLER::sendStatus(bool sendStatus)
{
  this->rtcData.NumberOfTransmitted++;    // Счетчик переданных пакетов
  if (sendStatus == ESP_NOW_SEND_SUCCESS) // Если сообщение доставлено
  {
    this->rtcData.switch_awakening++; // Количество срабатываний датчика
    this->rtcData.delivery_Error = 0;

    if (this->rtcData.deepSleepReason != objectID) // Если в прошлый раз уснули без ошибок передачи
    {
      this->rtcData.SendDataTime = millis(); // Время работы ESP при последней отправке
    }
    else // Если в прошлый раз не смогли передать данные
    {
      this->rtcData.SendDataTime += millis();
    }
    this->rtcData.deepSleepReason = 0xFF;
    this->RTCupdate();
    if (deepSleep_enable == true)
      ESP.deepSleep(0, RF_NO_CAL); // Спим до внешнего прерывания.
  }

  if (sendStatus == ESP_NOW_SEND_FAIL)
  { // Если сообщение не доставлено
    this->rtcData.deepSleep_awakening++;
    rtcData.delivery_Error++;
    this->rtcData.deepSleepReason = objectID; //
    RTCupdate();
    if (deepSleep_enable == true)
      ESP.deepSleep(this->content.interval_deep_sleep, RF_NO_CAL); // Спим.
  }
}

bool CONTROLLER::deepSleepReason()
{
  return (rtcData.deepSleepReason == objectID); // Если прошлая попытка отправки неудачная то отправляем true. Иначе false
}

void CONTROLLER::RTCread()
{
  uint32_t offset = (sizeof(rtcData) / sizeof(int)) * objectID;
  if (ESP.rtcUserMemoryRead(offset, (uint32_t *)&rtcData, (uint32_t)sizeof(rtcData)))
  {
    if (rtcData.calculateCRC8() != rtcData.CRC8)
    { // При первом включении и при битых данных в RTC памяти
      rtcData.deepSleepReason = 0xFF;
      this->rtcData.NumberOfTransmitted = 0;
      this->rtcData.delivery_Error = 0;
      this->rtcData.switch_awakening = 0;
      this->rtcData.deepSleep_awakening = 0;
      this->rtcData.SendDataTime = millis();
      this->rtcData.CRC8 = rtcData.calculateCRC8();
      RTCupdate();
    }
  }
}

void CONTROLLER::RTCupdate()
{
  rtcData.CRC8 = rtcData.calculateCRC8();
  uint32_t offset = (sizeof(rtcData) / sizeof(int)) * objectID;
  ESP.rtcUserMemoryWrite(offset, (uint32_t *)&rtcData, sizeof(rtcData)); // Write data to RTC memory
}