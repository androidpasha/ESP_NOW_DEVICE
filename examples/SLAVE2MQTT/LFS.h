#define USE_VECTOR
// Функция врзвращает указанную по номеру строку
template <typename T>
String readFile(const char *path, T line)
{
  File file = LittleFS.open(path, "r");

  if (!file)
    return F("Failed to open file for reading");
  while (file.available() && line > 1)
  { // Пропускаем лишние строки в количестве line-1.
    line -= file.read() == '\n';
  };

  String read_text = "";
  char last_simbol = '\0';
  while (file.available() && (last_simbol != '\n'))
  {
    last_simbol = file.read();
    read_text += last_simbol;
  };
  file.close();
  return read_text;
}

// Функция возвращает количество строк в указанном файле
uint16_t number_of_lines(const char *path)
{
  File file = LittleFS.open(path, "r");
  if (!file)
    return 0;
  u16 num = 1;
  while (file.available())
    num += (file.read() == '\n');
  file.close();
  return num;
}

// Функция записывает в ФС указанный объект (структуру)
#ifndef USE_VECTOR
template <typename T1, typename T2>
bool WriteLFS(const char *path, T1 *&DATA, const T2 &lenght)
{
  File file = LittleFS.open(path, "w");
  if (!file)
    return false;
  file.write((uint8_t *)DATA, sizeof(T1) * lenght);
  file.close();
  Serial.print(F("Запись в ФС проведена успешно."));
  return true;
}
#endif

template <typename T>
bool WriteLFS(const char *path, const std::vector<T> &DATA)
{
  File file = LittleFS.open(path, "w");
  if (!file)
    return false;

  size_t lenght = DATA.size(); // сохраняем длину
  if (lenght > 0)
  {
    file.write(reinterpret_cast<const uint8_t *>(DATA.data()), sizeof(T) * lenght);
  }

  file.close();
  Serial.println(F("Запись в ФС проведена успешно."));
  return true;
}

#ifndef USE_VECTOR
// Функция читает из ФС указанный объект (структуру)
template <typename T1, typename T2>
bool ReadLFS(const char *path, T1 *&DATA, T2 &lenght)
{
  File file = LittleFS.open(path, "r");
  if (!file or !file.size())
    return false;
  lenght = file.size() / sizeof(T1);
  delete[] DATA;
  DATA = new T1[lenght]; // Создаем массив необходимого размера
  file.read((uint8_t *)DATA, file.size());
  file.close();
  return true;
}
#endif

// Функция читает из ФС указанный объект (структуру) в std::vector<T>
template <typename T>
bool ReadLFS(const char *path, std::vector<T> &DATA)
{
  File file = LittleFS.open(path, "r");
  if (!file || !file.size())
    return false;

  size_t lenght = file.size() / sizeof(T);
  DATA.resize(lenght); // выделяем память в векторе

  file.read(reinterpret_cast<uint8_t *>(DATA.data()), file.size());
  file.close();
  return true;
}

// Функция возвращает размер файла
template <typename T>
bool fileSize(const char *path, T &lenght)
{
  File file = LittleFS.open(path, "r");
  if (!file)
    return false;
  lenght = file.size();
  file.close();
  return true;
}