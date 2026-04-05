#include <PubSubClient.h>
#include <ESP_NOW_DEVICE.h>
// #include <constants.h> // Текстовые шаблоны
#include <MultiString.h>

PubSubClient mqttClient(client);

bool mqttSetup()
{
  mqttClient.setServer(settings.mqtt_server, settings.mqtt_port);
  String clientId = F("ESP8266") + String(WiFi.macAddress());
  for (u8 i = 0; i < 5; i++) // 5 попыток соединится
  {
    if (mqttClient.connect(clientId.c_str(), settings.mqtt_user, settings.mqtt_pass))
      return true;

    delay(1000);
  }
  return false;
}

void macToStr(const uint8_t *mac, char *out)
{
  sprintf(out, "%02X%02X%02X%02X%02X%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool addNewMQTTdev(uint8_t deviceType, const uint8_t *mac)
{
  const uint8_t &T = deviceType;
  bool result = false;
  char id[13];
  macToStr(mac, id);

  //  ----- Собираю mqtt discovery
  MultiString haSensorsNames(devDescriptors[T][SENSOR_NAME]);
  MultiString haClass(devDescriptors[T][CLASS]);
  MultiString haUnit(devDescriptors[T][UNIT]);
  MultiString haState_class(devDescriptors[T][STATE_CLASS]);

  size_t sensor_size = 0;

  for (size_t i = 0; i < haSensorsNames.getNumStr(); i++)
    sensor_size += strlen(discovery_sensor) + strlen(id) * 2 + strlen(haSensorsNames[i]) + strlen(haClass[i]) + strlen(haUnit[i]) + strlen(haState_class[i]) - 5;

  sensor_size -= haSensorsNames.getNumStr();

  size_t header_size = strlen(discovery_header) + strlen(devDescriptors[T][DEV_NAME]) + strlen(FIRMWARE_VERSION) + strlen(id) * 2 + 1 - 6;
  size_t body_size = strlen(discovery_body) + strlen(devDescriptors[T][DEV_NAME]) + strlen(devDescriptors[T][STATE_CLASS])  + strlen(id) * 2 + 1 - 6;
  size_t tail_size = strlen(discovery_tail) + strlen(MQTTtopicAndPayload[T][1]) + strlen(id) + 1 - 4;
  size_t payload_size = header_size + body_size + sensor_size + tail_size - 3;

  char *payload = new char[payload_size]{'\0'};
  sprintf(payload, discovery_header, id, devDescriptors[T][DEV_NAME], FIRMWARE_VERSION, id);
  sprintf(payload + strlen(payload), discovery_body, devDescriptors[T][DEV_NAME], id, id);

  for (size_t i = 0; i < haSensorsNames.getNumStr(); i++)
    sprintf(payload + strlen(payload), discovery_sensor, i + 1, id, haSensorsNames[i], haClass[i], haUnit[i], haState_class[i], i + 1, i + 1, id);

  sprintf(payload + strlen(payload), discovery_tail, MQTTtopicAndPayload[T][1], id);
  Serial.println(payload);
  // ---------Discovery собран. Нужно потом очистить память   delete[] payload;

  size_t fm_ConfigTopic_size = strlen(configTopic) + strlen(id) + 1;
  char *fm_ConfigTopic = new char[fm_ConfigTopic_size];
  sprintf(fm_ConfigTopic, configTopic, id);

  mqttClient.setServer(settings.mqtt_server, settings.mqtt_port);
  u16 old_buffer_size = mqttClient.getBufferSize();
  mqttClient.setBufferSize(payload_size + 50);
  for (uint8_t i = 0; i < 5; i++) // 5 попыток соединится
  {
    if (mqttClient.connect(id, settings.mqtt_user, settings.mqtt_pass))
      result = mqttClient.publish(fm_ConfigTopic, payload, true);
    if (result)
      break;
    delay(1000);
  }
  delete[] fm_ConfigTopic;
  delete[] payload;
  mqttClient.setBufferSize(old_buffer_size);

  return result;
}

void sisInfoToMQTT(uint32_t period_ms)
{
  // Состояние шлюза
  static uint32_t lastMillis = 0;
  if (millis() > lastMillis + period_ms)
  {
    static bool firststart = true;
    CONTROLLER Gateway(ESP_NOW_Gateway, 1);
    GatewayVal values{WiFi.RSSI(), ESP.getFreeHeap(), Users.size()};
    Gateway.setMeasureValue(values);
    DewData GatewayData{Gateway.content, {0}, firststart};
    wifi_get_macaddr(STATION_IF, GatewayData.mac);
    if (MQTTqueue.size() < MAX_QUEUE_SIZE)
    {
      MQTTqueue.push(GatewayData);
      lastMillis = millis();
      firststart = false;
    }
  }
}

void MQTTloop()
{
  if (mqtt_enable)
  {
    sisInfoToMQTT(60'000);
    static u32 lastReconnectTime = millis();
    if (mqttClient.connected() == false)
    {
      if (millis() - lastReconnectTime > 60000)
      {
        String clientId = F("ESP8266") + String(WiFi.macAddress());
        mqttClient.connect(clientId.c_str(), settings.mqtt_user, settings.mqtt_pass);
        lastReconnectTime = millis(); // обновляем таймер
      }
    }
    else if (!MQTTqueue.empty())
    {
      if (MQTTqueue.front().newDev == true)
        addNewMQTTdev(MQTTqueue.front().content.deviceType, MQTTqueue.front().mac);

      String topic, payload;
      SLAVE parsingData(MQTTqueue.front().content);

      parsingData.resultToMQTTString(topic, payload);
      char mac[13];
      macToStr(MQTTqueue.front().mac, mac);
      topic += String(mac);

      if (mqttClient.publish(topic.c_str(), payload.c_str()))
      {
        Serial.printf(FSH("mqtt publish. Topic: %s.\nPayload: %s.\n"), topic.c_str(), payload.c_str());
        MQTTqueue.pop();
      }
    }
    else
      mqttClient.loop();
  }
}
