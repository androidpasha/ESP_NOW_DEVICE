void time_is_set(bool from_sntp /* <= this parameter is optional */)
{
  Serial.println(F("\nNTP время обновлено"));
  time_t unixTime = 0;
  time(&unixTime); // Записываем системное время в переменную unixTime в секундах с 1970г
  Serial.println(ctime(&unixTime));  // Fri Apr  1 02:01:00 2022\n
 

  if (!startTime)
  { //Если время включения еще не устанавливалось то установим
    time(&startTime);
    startTime -= millis() / 1000ul; // Отнимем в прошлое после включения до момента синхронизации
  }
}

String workTime()
{
  time_t sec = 0;
  time(&sec);
  sec -= startTime; // полное количество секунд
  char workTime[64];
snprintf(workTime, sizeof(workTime),
         "%lldд %02lldч %02lldм",
         (sec / 3600ll / 24ll),
         (sec / 3600ll) % 24ll,
         (sec % 3600ll) / 60ll);
  return workTime;
}

template <class T>
bool newDay(T *inDay)
{ // Если наступил новый день с 5 утра возвращаем true
  time_t unixTime = 0;
  time(&unixTime);
  if (*inDay != localtime(&unixTime)->tm_yday and localtime(&unixTime)->tm_hour > 4)
  {
    *inDay = localtime(&unixTime)->tm_yday;
    return true;
  }
  return false;
}
