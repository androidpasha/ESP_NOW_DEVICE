

void prinUserInfo()
{
  const char *_true{"true"};
  const char *_false{"false"};
  for (size_t i = 0; i < Users.size(); i++)
    Serial.printf(FSH("\nchatId: %lli   Predictions: %s   First start: %s   Not disturb: %s   Root: %s   User day: %02d   Free heap: %u\n"),
                  Users[i].chatId,
                  Users[i].Predictions ? _true : _false,
                  Users[i].FirstStart ? _true : _false,
                  Users[i].Do_not_disturb ? _true : _false,
                  Users[i].MasterUser ? _true : _false,
                  Users[i].Day,
                  ESP.getFreeHeap());
  Serial.print(F("\n---------------------------------------------\n"));
}

bool TelegramPrint(const String &Message, int64_t ID) // сделать отчет о доставке!!!
{
  Serial.printf(FSH("Исходящее telegram сообщение: %s; msgID = %d\n"), Message.c_str(), bot.lastBotMessage());
  if (bot.sendMessage(fb::Message(Message, ID), true))
  {
    delay(1000); // не частим с отправками
    return true;
  }
  else
  {
  delay(10000);
  bot.end();
  bot.begin();
  return false;
  }
}

void telegramLoop()
{
  bot.tick();
  for (size_t i = 0; i < Users.size(); i++)
  {
    if (Users[i].FirstStart == true && Users[i].Do_not_disturb == false && system_get_rst_info()->reason != 3) // Отправляем сообщение о появлении электричества
      Users[i].FirstStart = !TelegramPrint(F("Появилось электричество."), Users[i].chatId);

    bool deeliveryReport = false;
    if (Users[i].Do_not_disturb == false && !TelegramQueue.empty())
    {
      String message;
      SLAVE parsingData(TelegramQueue.front());
      parsingData.resultToString(message);
      if (!message.isEmpty())
        if (TelegramPrint(message, Users[i].chatId) == true)
          deeliveryReport = true;
    }

    size_t last_User_disturb;
    for (size_t i = 0; i < Users.size(); i++)
    {
      if (Users[i].Do_not_disturb == false)
        last_User_disturb = i;
    }

    if ((i == last_User_disturb) and TelegramQueue.size() > 0 and deeliveryReport == true)
    {
      TelegramQueue.pop();
      Serial.printf("telegramLoop. TelegramQueue.pop.TelegramQueue.size() = %d\n", TelegramQueue.size());
    }

    if (Users[i].Predictions == true)
    {
      if (TelegramPrint(String(F("Предсказание на сегодня:\n")) + readFile(dictionary_path, random(1, dictionary_lines)), Users[i].chatId) == true)
      {
        Users[i].Predictions = false;
        bool saveUSRinfo = true;
        for (size_t ii = 0; ii < Users.size(); ii++)
        { // Проверяем все ли получили предсказания
          if (Users[ii].Predictions == true)
          {
            saveUSRinfo = false;
            break;
          }
        }
        if (saveUSRinfo)
        {                                    // Если все получили предсказания то сохраняем данные(один раз в день)
          if (WriteLFS(settingsFile, Users)) // Сохраняю настройки в ФС
            Serial.print(F("Номер сегодняшнего дня сохранен в ФС"));
          else
            Serial.print(F("Не удалось сохранить настройки в энергонезависимую память."));
        }
      }
    }
  }
}

void TelegramDialog(fb::Update &u)
{
  struct
  {
    int64_t chatId;
    String text;
    String firstName;
    String lastName;
  } msg{u.message().from().id(),
        u.message().text().decodeUnicode(),
        u.message().from().firstName().decodeUnicode(),
        u.message().from().lastName().decodeUnicode()};

  Serial.print(F("\nВходящее telegram сообщение: "));
  Serial.println(msg.text);
  if (msg.text.equals(F("/start")))
  { // Добавление нового пользователя
    bool newUser = true;
    for (size_t i = 0; i < Users.size(); i++)
      if (msg.chatId == Users[i].chatId)
        newUser = false;
        
    if (newUser == true)
    {
      Users.push_back({msg.chatId});
      Serial.printf(FSH("Add chatId: %lli"), Users[Users.size() - 1].chatId);
      Serial.printf(FSH("Add chatId: %lli"), msg.chatId);
      if (WriteLFS(settingsFile, Users))
      { // Сохраняю настройки в ФС
        String report = F("\nПривет, ");
        report += msg.firstName + F(" ") + msg.lastName + F(".");
        report += F(" Вы успешно подключились к системе оповещения о приходе гостей. При каждом открытии двери вы будете получать уведомление. С утра, после открытия двери, вы будете получать предсказание на день.\n\nСписок поддерживаемых команд:\nНовая подписка /start\nОтключить уведомления: /Do_not_disturb\nВключить уведомления: /Enable_notifi\nВключить ПК: /PC_ON\nИнформации об устройстве: /info\nОтписаться: /unsubscribe.");
        TelegramPrint(report, Users[Users.size() - 1].chatId);
      }
      else
      {
        Serial.print(F("\nНе удалось сохранить нового пользователя в энергонезависимую память."));
      }
    }
    else
    {
      TelegramPrint(F("Вы уже подписались!\nСписок поддерживаемых команд:\nНовая подписка /start\nОтключить уведомления: /Do_not_disturb\nВключить уведомления: /Enable_notifi\nВключить ПК: /PC_ON\nИнформации об устройстве: /info\nОтписаться: /unsubscribe."), msg.chatId);
    }
    prinUserInfo();
    return;
  }

  if (msg.text.equals(F("/unsubscribe")))
  {
    for (size_t i = 0; i < Users.size(); i++)
    {
      if (msg.chatId == Users[i].chatId)
      {
        Serial.printf(FSH("ii=%u,Deleted chatId: %lli"), i, Users[i].chatId);
        Users.erase(Users.begin() + i); // удаляем пользователя
        String report;
        if (WriteLFS(settingsFile, Users))
        { // Сохраняю настройки в ФС
          report = msg.firstName + F(" ") + msg.lastName + F(", вы отписались. Досвидания. Для подписки используйте команду /start");
          TelegramPrint(report, msg.chatId);
          break;
        }
        else
        {
          report = F("Не удалось удалить пользователя из энергонезависимой памятьи.");
          TelegramPrint(report, msg.chatId);
        }
      }
    }
    prinUserInfo();
    return;
  }

  for (size_t i = 0; i < Users.size(); i++)
  {
    if (msg.chatId == Users[i].chatId)
    {
      if (msg.text.equals(F("/PC_ON")))
      {
        wakeMyPC();
        TelegramPrint(F("Команда включения отправлена на компьютер!"), Users[i].chatId);
      }

      else if (msg.text.equals(F("/Do_not_disturb")))
      {
        Users[i].Do_not_disturb = true;
        if (WriteLFS(settingsFile, Users))
        { // Сохраняю настройки в ФС
          TelegramPrint(F("Вы не будете получать уведомления о приходе гостей. Предсказания будут приходить. Для включения уведомлений нажмите: /Enable_notifi"), Users[i].chatId);
        }
        else
        {
          TelegramPrint(F("Не удалось сохранить настройки в энергонезависимую память."), Users[i].chatId);
        }
      }

      else if (msg.text.equals(F("/Enable_notifi")))
      {
        Users[i].Do_not_disturb = false;
        if (WriteLFS(settingsFile, Users))
        { // Сохраняю настройки в ФС
          TelegramPrint(F("Уведомления о приходе гостей включены! Для отключения отправьте команду /Do_not_disturb"), Users[i].chatId);
        }
        else
        {
          TelegramPrint(F("Не удалось сохранить настройки в энергонезависимую память."), Users[i].chatId);
        }
      }
      else if (msg.text.equals(F("/info")))
      {
        String msg = F("Cистемная информация:");
        msg += F("\nКоличество подписчиков: ");
        msg += Users.size();
        msg += F(";\nВремя работы: ");
        msg += workTime();
        msg += F(";\nНапряжение питания: ");
        msg += String(ESP.getVcc() / 1000.00, 2);
        msg += F("В");
        msg += F(";\nУровень сигнала wifi: ");
        msg += WiFi.RSSI();
        msg += F("dbi;");
        msg += F("\nmac адрес: ");
        msg += WiFi.macAddress();
        msg += F(";\nОЗУ свободно: ");
        msg += ESP.getFreeHeap();
        msg += F(" байт.\nФайловая система:");
        FSInfo fs_info;
        LittleFS.info(fs_info);
        msg += F("\nВсего:         ");
        msg += fs_info.totalBytes;
        msg += F(" байт;\nЗанято:       ");
        msg += fs_info.usedBytes;
        msg += F(" байт;\nСвободно: ");
        msg += fs_info.totalBytes - fs_info.usedBytes;
        msg += F(" байт.");
        msg += F("\nОтписаться: /unsubscribe");
        TelegramPrint(msg, Users[i].chatId);
      }
      else if (msg.text.equals(F("/restart")))
      {
        String msg = F("Выполняется перезагрузка...");
        TelegramPrint(msg, Users[i].chatId);
        delay(5000);
        ESP.restart();
      }
      // else if (msg.text.equals(F("/backup")))
      // {
      //   TBMessage msg;
      //   msg.chatId = Users[i].chatId;
      //   sendDocument(msg, AsyncTelegram2::DocumentType::TEXT, dictionary_path, "dictionary");
      //   sendDocument(msg, AsyncTelegram2::DocumentType::BINARY, settingsFile, "UserData");
      //   sendDocument(msg, AsyncTelegram2::DocumentType::JSON, wifiSettingsFile, "HardwareData");
      // }
      else
      {
        TelegramPrint(F("Выбери команду:\n\n-Не беспокоить       /Do_not_disturb\n\n-Вкл уведомления /Enable_notifi\n\n-Включить ПК         /PC_ON\n\n-Перезагрузка         /restart\n\nИнформации об устройстве /info"), Users[i].chatId);
      }
    }
  }
}