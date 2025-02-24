#ifndef TEST_DEVICE_BUILD

/**
 * @file main.cpp
 * @brief I2C-slave для клавиатуры управления громкостью с индикацией.
 *
 * Данный файл содержит реализацию прошивки для микроконтроллера STM32F103CBT6 (Arduino),
 * который работает как ведомое устройство на шине I2C. Устройство реализует протокол, в котором:
 * - Команда записи (0x40) задаёт состояние 6 светодиодов.
 *   Биты [5:0] управляют светодиодами, бит [7] указывает, что следующая операция чтения
 *   должна вернуть состояние светодиодов.
 * - Команда чтения (0x41) возвращает:
 *   - Состояние светодиодов (с установленным битом 7), если в предыдущей записи был запрошен режим чтения LED.
 *   - Или состояние кнопок с учётом фильтрации дребезга и определением кратковременного/длительного нажатия.
 *
 * Клавиатура имеет две кнопки ("Громкость +" и "Громкость -") с обработкой дребезга и
 * определением времени нажатия (порог 500 мс). Состояние кнопок возвращается при чтении по I2C.
 */

#include <Arduino.h>
#include <Wire.h>
#include "ButtonHandler.h"

static const uint8_t I2C_SLAVE_ADDRESS = 0x20;                    ///< Адрес I2C-слейва.
static const uint8_t CMD_WRITE_LED = 0x40;                        ///< Код команды для записи состояния светодиодов.
static const uint8_t LED_PINS[] = {PA0, PA1, PA2, PA3, PA4, PA5}; ///< Пины светодиодов
static const uint8_t BTN_PIN[] = {PA6, PA7};                      ///< Пин кнопки "Громкость +".
static volatile uint8_t ledState = 0;                             ///< Хранит состояние 6 светодиодов (биты [5:0]).
static volatile bool lastCommandReadLED = false;                  ///< Флаг, указывающий, что следующая операция чтения должна вернуть состояние светодиодов.
static const uint64_t debounceDelay = 50 * 1000;                  ///< Задержка для устранения дребезга (50 мс)
static const uint64_t longPressThreshold = 500 * 1000;            ///< Порог длительного нажатия (500 мс)

static ButtonHandler volPlusButton(BTN_PIN[0], debounceDelay, longPressThreshold);
static ButtonHandler volMinusButton(BTN_PIN[1], debounceDelay, longPressThreshold);

/**
 * @brief Обработчик приема данных по I2C.
 *
 * Функция вызывается при получении данных от ведущего по шине I2C.
 * Ожидается два байта: первый — команда (0x40), второй — данные для светодиодов.
 * Если бит [7] во втором байте установлен, то следующая операция чтения вернет состояние светодиодов.
 *
 * @param received_bytes Количество полученных байтов.
 */
void receiveEvent(int received_bytes)
{
    if (received_bytes < 2 || Wire.read() != CMD_WRITE_LED)
        return; // Если получено меньше двух байтов или команда не для записи светодиодов, выходим

    uint8_t data = Wire.read();
    ledState = data & 0x3F;
    lastCommandReadLED = (data & 0x80);

    for (uint8_t i = 0; i < sizeof LED_PINS; ++i) // Обновление выходов для светодиодов
        pinMode(LED_PINS[i], OUTPUT), digitalWrite(LED_PINS[i], (ledState & (1 << i)) ? HIGH : LOW);
}

/**
 * @brief Обработчик запроса данных по I2C.
 *
 * Функция вызывается, когда ведущий запрашивает данные.
 * Если был запрошен режим чтения состояния светодиодов, возвращается состояние светодиодов с установленным битом 7.
 * В противном случае возвращается состояние кнопок с информацией о кратковременных и длительных нажатиях.
 */
void requestEvent()
{
    uint8_t response = 0;
    if (lastCommandReadLED) // Если активирован режим чтения светодиодов, возвращаем состояние светодиодов
    {
        response = ledState | 0x80; // Устанавливаем бит 7
        lastCommandReadLED = false; // Сброс флага после чтения
    }
    else // Формирование байта состояния кнопок:
    {
        response |= (volMinusButton.isPressedNow() << 0) | // "Громкость -": бит 0 – текущее состояние,
                    (volMinusButton.isShortPress() << 1) | //                бит 1 – кратковременное нажатие,
                    (volMinusButton.isLongPress() << 2);   //                бит 2 – длительное нажатие
        response |= (volPlusButton.isPressedNow() << 3) |  // "Громкость +": бит 3 – текущее состояние,
                    (volPlusButton.isShortPress() << 4) |  //                бит 4 – кратковременное нажатие,
                    (volPlusButton.isLongPress() << 5);    //                бит 5 – длительное нажатие
    }
    Wire.write(response);
}

/**
 * @brief Возвращает текущее время в микросекундах с момента запуска микроконтроллера.
 *
 * Функция `get_tick()` возвращает 64-битное значение времени, учитывающее переполнения
 * 32-битного счетчика, возвращаемого функцией `micros()`. Это позволяет корректно
 * отслеживать время на протяжении более чем 71.58 минут, что является пределом для
 * 32-битного значения.
 *
 * @return uint64_t Текущее время в микросекундах с момента запуска.
 */
uint64_t get_tick(void)
{
    static uint32_t overflow = 0;
    static uint32_t lastMicros = 0;
    uint32_t currentMicros = micros();
    overflow += (currentMicros < lastMicros);
    return ((uint64_t)overflow << 32) | (lastMicros = currentMicros);
}

void setup()
{
    Wire.begin(I2C_SLAVE_ADDRESS); // I2C-1 standard pins: PB7(sda) PB6(scl)
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
}

void loop()
{
    uint64_t ticks = get_tick();
    volPlusButton.updateState(ticks);
    volMinusButton.updateState(ticks);
}

#else
#include "TestDevice.h"
#endif // TEST_DEVICE_BUILD