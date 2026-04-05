void WifiAP()
{
  WiFi.mode(WIFI_AP);                              // WIFI_AP_STA
  WiFi.softAP(ssidAP);                             // Создаём точку доступа
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP()); // Переходим с любой страницы на IP точки доступа ESP8266
  webServer.begin();                               // Инициализируем Web-сервер
  Serial.print(F("\nТочка доступа включена. IP: "));
  Serial.println(WiFi.softAPIP());
  WebServer();       // Функция обработки запросов
  WebserverProces(); // Опрос запросов
}


bool wifiConnect()
{
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  File file = LittleFS.open(wifiSettingsFile, "r");
  if (file)
  {
    Serial.println(F("Читаю /config.json:"));
    JsonDocument json;
    deserializeJson(json, file);
    strcpy(settings.Local_ssid, json["SSID"].as<String>().c_str());
    strcpy(settings.Local_password, json["Password"].as<String>().c_str());
    strcpy(settings.botToken, json["Token"].as<String>().c_str());
    strcpy(settings.MACAddress, json["MAC"].as<String>().c_str());
    // settings.message = json["Message"].as<String>();
    strcpy(settings.mqtt_server, json["mqtt_server"].as<String>().c_str());
    settings.mqtt_port = json["mqtt_port"].as<int>();
    strcpy(settings.mqtt_user, json["mqtt_user"].as<String>().c_str());
    strcpy(settings.mqtt_pass, json["mqtt_pass"].as<String>().c_str());

    serializeJsonPretty(json, Serial);
    Serial.println();
  }


  uint32_t nextMillis = millis() + timeoutSTA * 1e3; // таймаут подключения к wifi
  WiFi.begin(settings.Local_ssid, settings.Local_password);
  Serial.println(F("\nПодключение к WiFi"));
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1e3);
    Serial.print(F("."));
    if (millis() > nextMillis)
    {
      ESP.restart(); // перезагружаемся если не было соединения.
      return false;
    }
  }
  Serial.println(F("\nWiFi подключен!"));
  return true;
}
