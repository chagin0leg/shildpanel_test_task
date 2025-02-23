#ifdef TEST_DEVICE_BUILD

#include <Arduino.h>
#include <Wire.h>

#define SLAVE_ADDRESS 0x20 // Адрес ведомого устройства
#define CMD_WRITE_LED 0x40 // Команда для записи состояния светодиодов

// Функция опроса ведомого устройства для получения состояния кнопок
void pollSlave()
{
    static uint8_t lastButtonState = 0; // Храним предыдущее состояние кнопок
    Wire.requestFrom(SLAVE_ADDRESS, 1); // Запрос 1 байта
    if (Wire.available())
    {
        uint8_t data = Wire.read();
        // Если установлен бит 7, то это состояние светодиодов, а не кнопок – пропускаем.
        if (!(data & 0x80))
        {
            // Если состояние изменилось, выводим его в монитор
            if (data != lastButtonState)
            {
                Serial.print("Value: 0b"), Serial.print(data, BIN), Serial.print("\t");

                if ((data & 0x01) != (lastButtonState & 0x01))
                    Serial.println((data & 0x01) ? "Vol-: Кнопка нажата" : "Vol-: Кнопка отпущена");
                if ((data & 0x02) != (lastButtonState & 0x02))
                    Serial.println("Vol-: Кратковременное нажатие");
                if ((data & 0x04) != (lastButtonState & 0x04))
                    Serial.println("Vol-: Длительное нажатие");
                if ((data & 0x08) != (lastButtonState & 0x08))
                    Serial.println((data & 0x08) ? "Vol+: Кнопка нажата" : "Vol+: Кнопка отпущена");
                if ((data & 0x10) != (lastButtonState & 0x10))
                    Serial.println("Vol+: Кратковременное нажатие");
                if ((data & 0x20) != (lastButtonState & 0x20))
                    Serial.println("Vol+: Длительное нажатие");

                lastButtonState = data;
            }
        }
    }
}

void setup()
{
    Serial.begin(115200); // Инициализация последовательного порта 115200 8N1
    Wire.begin();         // Инициализация I2C в режиме мастера
    delay(1000);
}

void loop()
{
    static unsigned long nextPollTime = millis(); // Таймер для опроса (20 Гц)
    (millis() > nextPollTime && (nextPollTime += 50)) ? pollSlave() : void();

    if (Serial.available() > 0)
    {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input.length() > 0)
        {
            // Интерпретация введённого значения.
            // Поддерживается ввод в десятичном формате или в виде шестнадцатеричного значения (начинается с "0x").
            uint8_t ledValue = 0;
            (input.startsWith("0x") || input.startsWith("0X"))
                ? ledValue = (uint8_t)strtol(input.c_str(), NULL, 16)
                : ledValue = (uint8_t)input.toInt();
            ledValue &= 0x3F; // Используем только младшие 6 бит (LED_PINS соответствуют битам [5:0])

            // Отправка команды на ведомое устройство:
            // Первый байт – команда (0x40), второй – данные для светодиодов.
            Wire.beginTransmission(SLAVE_ADDRESS);
            Wire.write(CMD_WRITE_LED);
            Wire.write(ledValue);
            uint8_t error = Wire.endTransmission();
            if (error == 0)
            {
                Serial.print("Отправлена команда LED: 0x");
                Serial.println(ledValue, HEX);
            }
            else
            {
                Serial.print("Ошибка передачи по I2C: ");
                Serial.println(error, DEC);
            }
        }
    }
}

#endif // TEST_DEVICE_BUILD