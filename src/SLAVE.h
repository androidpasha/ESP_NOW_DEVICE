#pragma once
using namespace constants;
class SLAVE // Класс устройств умного дома
{
public:
  Content content;
  SLAVE(uint8_t *incomingData);
  SLAVE(Content &incomingData);
  void resultToString(String &out); // Добавляет к строке описания событий датчиков
  uint8_t deviceTipe();
  void resultToMQTTString(String &payload, String &topic);

private:
};