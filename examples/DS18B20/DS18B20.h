#pragma once
#include <Arduino.h>
#include <OneWire.h>
#ifndef GPIO
#define GPIO(n) (GPIO_ID_PIN0 + (n))
#endif

class DS18B20
{
public:
    DS18B20(uint8_t pin): pin(pin)
    {
        nextMillis = 0;
        lastTemperature = 0;
    }

    void request()
    {                                     // Запрос измерения
        if (this->nextMillis <= millis()) // Запрос не чаще 750  мс
        {
            OneWire DS18B20(pin);
            DS18B20.reset();        // сброс линии
            DS18B20.skip();         // Пропуск поиска адреса датчика т.к. датчик один
            DS18B20.write(0x44, 1); // запускаем конверсию и включаем паразитное питание
            this->nextMillis = millis() + 750;
        }
    }

    float getTemp()
    { // Чтение результата
        OneWire DS18B20(pin);
        DS18B20.reset();
        DS18B20.skip();
        DS18B20.write(0xBE); // запрос на чтение scratchpad-памяти

        byte scratchpad[9];
        for (uint8_t i = 0; i < 9; i++)
            scratchpad[i] = DS18B20.read(); // считываем scratchpad-память 9 байтов

        if (this->nextMillis <= millis() and DS18B20.crc8(scratchpad, 8) == scratchpad[8])
        {
            this->lastTemperature = ((scratchpad[1] << 8) | scratchpad[0]) / 16.0 // Расчет температуры
                                    - 0x1000 * (scratchpad[1] >> 7);              // для отрицательной температуры
        }
        return this->lastTemperature;
    }

private:
    uint8_t pin;
    float lastTemperature;
    uint32_t nextMillis;
};