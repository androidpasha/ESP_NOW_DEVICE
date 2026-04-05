#include <Arduino.h>
#include <MultiString.h>

MultiString::MultiString(const char *str, const char separator) : separator(separator)
{
    count = countOfSensor(str);
    names = new char *[count];
    fillArrays(str);
}

const size_t MultiString::getNumStr() { return count; }

const char *MultiString::operator[](size_t index) const
{
    return (index < count) ? names[index] : "";
}

MultiString::~MultiString()
{
    for (size_t i = 0; i < count; i++)
        delete[] names[i];
    delete[] names;
};

size_t MultiString::countOfSensor(const char *p)
{
    if (*p == '\0')
        return 1;
    int count = 0;
    while (*p != '\0')
    {
        if (*p == separator || *(p + 1) == '\0')
            count++;
        p++;
    }
    return count;
}

void MultiString::fillArrays(const char *p)
{
    int index = 0;
    const char *start = p;

    while (true)
    {
        // дошли до разделителя или конца строки
        if (*p == separator || *p == '\0')
        {
            int len = p - start;              // длина слова
            names[index] = new char[len + 1]; // +1 для '\0'
            memcpy(names[index], start, len);
            names[index][len] = '\0';
            index++;
            start = p + 1; // начало следующего слова
        }
        if (*p == '\0')
            break;
        p++;
    }
}