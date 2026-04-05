void jsonRequest();
void handleForm();
void WebserverProces()
{                                                           // Опрос запросов
  uint32_t nextMillis = millis() + timeoutWaitClient * 1e3; // 30 секунд ждем клиентов
  do
  {
    dnsServer.processNextRequest(); // Переходим с любой страницы на IP точки доступа ESP8266
    webServer.handleClient();       // Обработчик HTTP-событий (отлавливает HTTP-запросы к устройству и обрабатывает их в соответствии с выше описанным алгоритмом)
    if (WiFi.softAPgetStationNum() != 0)
      nextMillis = millis() + 1e3; // Если есть подключенные клиенты то не выходим из цикла
  } while (millis() < nextMillis and millis() < timeoutAPclient * 60 * 1e3); // таймаут в 5 минут
}

void handleCaptivePortal()
{
  // Редирект на главную страницу
  webServer.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
  webServer.send(302, "text/plain", "");
}

bool handleFileRequest(String path)
{
  if (path == "/")
    path = "/index.html"; // файл по умолчанию

  if (LittleFS.exists(path))
  {
    String contentType = "text/plain";
    if (path.endsWith(".html"))
      contentType = "text/html";
    else if (path.endsWith(".css"))
      contentType = "text/css";
    else if (path.endsWith(".js"))
      contentType = "application/javascript";
    else if (path.endsWith(".png"))
      contentType = "image/png";
    else if (path.endsWith(".jpg"))
      contentType = "image/jpeg";
    else if (path.endsWith(".ico"))
      contentType = "image/x-icon";

    File file = LittleFS.open(path, "r");
    webServer.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void WebServer()
{
  // Обработка HTTP-запросов
  webServer.on(F("/jsonRequest"), jsonRequest); // На этот запрос отдаем указания как заполнять веб страницу
  webServer.on(F("/jsonPOST"), handleForm);     // Тут принимаем пользовательские данные от страницы
  webServer.onNotFound([]()
                       {
    if (!handleFileRequest(webServer.uri()))
    {
      handleCaptivePortal();
    } });
}

void jsonRequest()
{
  File file = LittleFS.open(wifiSettingsFile, "r");
  if (file)
  {
    webServer.streamFile(file, F("text/json")); //  Выводим содержимое файла по HTTP, указывая заголовок типа содержимого contentType
    file.close();
  }
  else
  {
    webServer.send(204, F("Content-Type: text/plain"), F("404. Page not found:("));
  }
}

void handleForm()
{
  int httpCode = 200;
  if (webServer.method() == HTTP_POST)
  {
    File file = LittleFS.open(wifiSettingsFile, "w");
    if (file)
    {
      file.print(webServer.arg(0));
      file.close();
    }
    else
      httpCode = 404;
    webServer.send(httpCode);
    delay(1e3); // время на отправку ответа
    if (httpCode == 200)
      WiFi.mode(WIFI_STA); // WIFI_AP_STA
  }
}
