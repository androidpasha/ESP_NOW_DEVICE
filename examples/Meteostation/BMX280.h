#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>

#ifndef GPIO
#define GPIO(n) (GPIO_ID_PIN0 + (n))
#endif
// #include <esp8266RTCmemory.h>

class BMX280
{
private:
    int32_t t_fine = 0;
    // esp8266RTCmemory RTC;
    const uint WireClock = 100'000;

    struct calibDTA
    { // Структура калибровочные биты
        uint16_t dig_T1;
        int16_t dig_T2, dig_T3; // температура
        uint16_t dig_P1;
        int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9; // давление
        uint8_t dig_H1, dig_H3;
        int16_t dig_H2, dig_H4, dig_H5;
        int8_t dig_H6; // влажность
        uint8_t chip_id;
    } calbiration;

    // Запись одного байта по указанному регистру
    void setRegister(uint8_t reg, const uint8_t value)
    {
        Wire.beginTransmission(0x76);
        Wire.write(reg);
        Wire.write(value);
        Wire.endTransmission(true);
    }

    // Последовательное чтение регистров от 1го до 4х
    int32_t getRegister(uint8_t reg, int len)
    {
        if (len < 1 || len > 4)
            return -1;
        Wire.beginTransmission(0x76);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(0x76, len);
        int32_t value = 0;
        for (byte i = 0; i < len; i++)
        {
            value <<= 8;
            value |= (uint8_t)Wire.read();
        }
        Wire.endTransmission(true);
        return value;
    }

    void getCalibData()
    { // читаю калибровочные биты
        Wire.beginTransmission(0x76);
        Wire.write(0x88);
        Wire.endTransmission(false);
        Wire.requestFrom(0x76, 26);                            // 26 байт калибровок температуры и давления
        calbiration.dig_T1 = Wire.read() | (Wire.read() << 8); // 88/89
        calbiration.dig_T2 = Wire.read() | (Wire.read() << 8); // 8A/8B
        calbiration.dig_T3 = Wire.read() | (Wire.read() << 8); // 8C/8D
        calbiration.dig_P1 = Wire.read() | (Wire.read() << 8); // 8E/8F
        calbiration.dig_P2 = Wire.read() | (Wire.read() << 8); // 90/91
        calbiration.dig_P3 = Wire.read() | (Wire.read() << 8); // 92/93
        calbiration.dig_P4 = Wire.read() | (Wire.read() << 8); // 94/95
        calbiration.dig_P5 = Wire.read() | (Wire.read() << 8); // 96/97
        calbiration.dig_P6 = Wire.read() | (Wire.read() << 8); // 98/99
        calbiration.dig_P7 = Wire.read() | (Wire.read() << 8); // 9A/9B
        calbiration.dig_P8 = Wire.read() | (Wire.read() << 8); // 9C/9D
        calbiration.dig_P9 = Wire.read() | (Wire.read() << 8); // 9E/9F
        Wire.read();                                           // A0 пропускаю
        calbiration.dig_H1 = Wire.read();                      // A1

        Wire.beginTransmission(0x76);
        Wire.write(0xE1);
        Wire.endTransmission(false);

        if (calbiration.chip_id == 0x60)
        {
            Wire.requestFrom(0x76, 7);                           // 7байт калибровок влажности
            calbiration.dig_H2 = Wire.read() | Wire.read() << 8; // E1/E2
            calbiration.dig_H3 = Wire.read();                    // E3
            calbiration.dig_H4 = Wire.read() << 4;               // E4
            {
                uint8_t E5 = 0;
                E5 |= (uint8_t)Wire.read(); // E5
                calbiration.dig_H4 |= E5 & 0b00000111;
                calbiration.dig_H5 = ((E5 & 0b11110000) >> 4) | (Wire.read() << 4);
            } // E6
            calbiration.dig_H6 = Wire.read(); // E7
        }

        // RTC.write(calbiration, sizeof(calbiration));
    }

public:
    BMX280(uint8_t swSDA, uint8_t swSCL) /*: RTC(sizeof(calbiration))*/
    {
        Wire.setClock(WireClock);
        Wire.begin(swSDA, swSCL);

        // if (ESP.getResetInfoPtr()->reason == REASON_DEFAULT_RST){
        getCalibData();
        // Serial.printf("\nReal T1:%d T2:%d T3:%d\n", calbiration.dig_T1, calbiration.dig_T2, calbiration.dig_T3 );
        // }
        // else{
        //     RTC.read(calbiration, sizeof(calbiration));

        //     Serial.printf("\nRTC  T1:%d T2:%d T3:%d\n", calbiration.dig_T1, calbiration.dig_T2, calbiration.dig_T3 );
        // }

        if (BMxbegin())
            startMeasurement();
    }

    // Инициализация BMx, определение типа датчика, настройка на однократное измерение
    bool BMxbegin()
    {
        calbiration.chip_id = (uint8_t)getRegister(0xD0, 1);
        if (calbiration.chip_id < 0x56 || calbiration.chip_id > 0x60 || calbiration.chip_id == 0x59)
        {
            return false; // если датчик не обнаружен то перезагружаемся
        }
        if (calbiration.chip_id == 0x60)
            setRegister(0xF2, B00000001); // Измерять влажность если BME
        setRegister(0xF5, B11100000);     // измерение раз в 4 сек(оно отключено в 0хF4) и фильтрация по 4м измерениям
        // getCalibData();
        return true;
    }

    void startMeasurement()
    {
        setRegister(0xF4, B01001101); // Запуск измерения температуры и давления//010(17бит температура)011(18бит давление)01(однократное измерение и переход в спячку 00)//было 10110111
    }

    bool measureOk()
    { // Ждем окончания измерений
        uint32_t lastmillis = millis();
        while (getRegister(0xF3, 1) & 0b00001001)
        {
            if (millis() - lastmillis > 25)
                return false; // стандартное время измерений 16мс даем 25 на всякий случай
        }
        return true;
    }

    float readTemperature()
    {
        // читаю значение АЦП BMx
        int32_t adc_T = (uint32_t)getRegister(0xFA, 3);
        adc_T >>= 4; // младший регистр содержит только 4 бита. Сдвигаем все данные на 4 бита
        // Расчет по даташиту
        float var1 = ((((adc_T >> 3) - ((int32_t)calbiration.dig_T1 << 1))) * ((int32_t)calbiration.dig_T2)) >> 11;
        float var2 = (((((adc_T >> 4) - ((int32_t)calbiration.dig_T1)) * ((adc_T >> 4) - ((int32_t)calbiration.dig_T1))) >> 12) * ((int32_t)calbiration.dig_T3)) >> 14;
        t_fine = var1 + var2;
        float T = (t_fine * 5 + 128) >> 8;
        return T / 100;
    }

    float readPressure()
    {
        // t_fine = 0;
        int64_t var1, var2, p;
        int32_t adc_P = getRegister(0xF7, 3);
        if (adc_P == 0x800000) // value in case pressure measurement was disabled
            return NAN;
        adc_P >>= 4;
        var1 = ((int64_t)t_fine) - 128000;
        var2 = var1 * var1 * (int64_t)calbiration.dig_P6;
        var2 = var2 + ((var1 * (int64_t)calbiration.dig_P5) << 17);
        var2 = var2 + (((int64_t)calbiration.dig_P4) << 35);
        var1 = ((var1 * var1 * (int64_t)calbiration.dig_P3) >> 8) +
               ((var1 * (int64_t)calbiration.dig_P2) << 12);
        var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calbiration.dig_P1) >> 33;
        if (var1 == 0)
            return 0; // avoid exception caused by division by zero
        p = 1048576 - adc_P;
        p = (((p << 31) - var2) * 3125) / var1;
        var1 = (((int64_t)calbiration.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
        var2 = (((int64_t)calbiration.dig_P8) * p) >> 19;
        p = ((p + var1 + var2) >> 8) + (((int64_t)calbiration.dig_P7) << 4);
        return (float)p / 256;
    }

    float getHumidity()
    {
        // t_fine = 0;
        if (calbiration.chip_id != 0x60)
            return NAN;
        int32_t adc_H = getRegister(0xFD, 2);
        if (adc_H == 0x8000)
            return NAN; // value in case humidity measurement was disabled
        int32_t v_x1_u32r;
        v_x1_u32r = (t_fine - ((int32_t)76800));
        v_x1_u32r = (((((adc_H << 14) - (((int32_t)calbiration.dig_H4) << 20) -
                    (((int32_t)calbiration.dig_H5) * v_x1_u32r)) +
                    ((int32_t)16384)) >> 15) *
                    (((((((v_x1_u32r * ((int32_t)calbiration.dig_H6)) >> 10) *
                    (((v_x1_u32r * ((int32_t)calbiration.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
                    ((int32_t)2097152)) * ((int32_t)calbiration.dig_H2) + 8192) >> 14));

        v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                    ((int32_t)calbiration.dig_H1)) >> 4));

        v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
        v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
        float h = (v_x1_u32r >> 12);
        return h / 1024.0;
    }
};