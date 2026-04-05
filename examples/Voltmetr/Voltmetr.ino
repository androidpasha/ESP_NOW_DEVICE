//Вариант датчика напряжения питания ESP. Данные отправляются при внешнем сбросе RST. Потом уходим в глубокий сон.
//При неудачной отправке пытаемся отправить данные каждые repeat_minutes
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ESP_NOW_DEVICE.h>
uint8_t broadcastAddress[6];            // мак адрес приемника что воткнут в розетку. Заполняется автоматически.
int8_t esp_channel;                     // канал роутера wifi. Заполняется автоматически.
enum SETTINGS                                                      // Общие настройки
{
  repeat_minutes = 1, // Интервал повторных отправок в случае неудачной доставки сообщения, в минутах
  deepSleep_enable = false, //true: засыпать после отправки сообщения. false: библиотека в сон не входит.
  DEVICE_TYPE = Voltmetr //Варианты:  Door_Open_Sensor, Door_Ring, Mail, Temperature_Sensor, Humidity_Sensor, VCC_logger. Можно добавить свой тип в constants.h
};
CONTROLLER VCCmeter(DEVICE_TYPE, repeat_minutes, deepSleep_enable);//Создаем объект класа датчика с типом VCC_logger. Можно без последней переменной deepSleep_enable, тогда укладывать спать библиотека не будет.
ADC_MODE(ADC_VCC);
void OnDataSent_cb(uint8_t *mac_addr, uint8_t sendStatus);

void setup() {
  Serial.begin(115200);
  double VCC =  ESP.getVcc() / 1000.0;
  Serial.printf("\nНаряжение=%f", VCC);
  VCCmeter.setMeasureValue((double)VCC);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  HANDSHAKE(broadcastAddress, esp_channel);
  // настраиваем ESPnow
  if (esp_now_init() != 0)
    ESP.restart(); //при сбое перезагрузка
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);//Роль передатчика
  esp_now_register_send_cb(OnDataSent_cb);//Назначаем обратную функцию
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, esp_channel, NULL, 0); // регистрируем пиры
  esp_now_send(broadcastAddress, (uint8_t *)&VCCmeter.content, sizeof(VCCmeter.content));//Отправляем данные на указанный мак broadcastAddress

}
void loop() {}

void OnDataSent_cb(uint8_t *mac_addr, uint8_t sendStatus)
{
  VCCmeter.sendStatus(sendStatus);//Обратный вызов. Отчет о доставке сообщения. Необходимо библиотеке сообщить статус доставки для подсчетов.
  if (sendStatus == ESP_NOW_SEND_SUCCESS) {
    ESP.deepSleep(0); //Спим до внешнего прерывания.
  }
  if (sendStatus == ESP_NOW_SEND_FAIL) {
    ESP.deepSleep(repeat_minutes); //Спим.
  }
}