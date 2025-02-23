#ifdef TEST_DEVICE_BUILD

#include <Arduino.h>
#include <Wire.h>

#define I2C_DELAY delay(75)
#define PCF8574_ADDR 0x20

void setup()
{
    delay(500); // Задержка 500 миллисекунд для стабилизации системы

    Wire.begin(2, 0);                     // Инициализация I2C интерфейса с использованием пинов SDA = 2 и SCL = 0
    Wire.beginTransmission(PCF8574_ADDR); // Начало передачи данных на устройство с адресом PCF8574_ADDR
    Wire.endTransmission();               // Завершение передачи данных (пинг)
    delay(250);                           // Ожидание 250 миллисекунд

    Wire.beginTransmission(PCF8574_ADDR); // Начало новой передачи данных на устройство с адресом PCF8574_ADDR
    Wire.write(0b11111110);               // Отправка начального значения 0b11111110 на устройство
    Wire.endTransmission();               // Завершение передачи данных
    delay(250);                           // Ожидание 250 миллисекунд
}

void loop()
{
    static uint8_t data = 0b11111110;     // Инициализация переменной data с начальным значением 0b11111110
    Wire.beginTransmission(PCF8574_ADDR); // Начало передачи данных на устройство с адресом PCF8574_ADDR
    Wire.write(data ^= 0x01);             // Инверсия младшего бита переменной data и отправка
    Wire.endTransmission();               // Завершение передачи данных
    delay(1000);                          // Ожидание 1000 миллисекунд (1 секунду) перед следующей итерацией
}

#endif // TEST_DEVICE_BUILD