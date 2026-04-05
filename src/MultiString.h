#include <Arduino.h>
class MultiString
{
private:
    char **names;
    size_t count;
    char separator;

public:
    MultiString() = delete;
    MultiString(const char *str, const char separator = ';') ;
    const size_t getNumStr();
    const char *operator[](size_t index) const;
    ~MultiString();

private:
    size_t countOfSensor(const char *p);
    void fillArrays(const char *p);
};