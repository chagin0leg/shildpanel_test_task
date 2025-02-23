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
 *
 * Аппаратная часть:
 * - Светодиоды подключены к пинам 4–9.
 * - Кнопки подключены к пинам 10 и 11 с внутренней подтяжкой.
 *
 * @note В будущем планируется настройка автоматической сборки и автодокументации с использованием Doxygen.
 */

#include <Arduino.h>
#include <Wire.h>
#include "ButtonHandler.h"

// I2C-константы
const uint8_t I2C_SLAVE_ADDRESS = 0x20; ///< Адрес I2C-слейва.
const uint8_t CMD_WRITE_LED = 0x40;     ///< Код команды для записи состояния светодиодов.
const uint8_t CMD_READ = 0x41;          ///< Код команды для чтения состояния.

// Определения пинов для светодиодов (6 шт.)
const uint8_t LED1_PIN = 4; ///< Пин светодиода LED1 (бит 0).
const uint8_t LED2_PIN = 5; ///< Пин светодиода LED2 (бит 1).
const uint8_t LED3_PIN = 6; ///< Пин светодиода LED3 (бит 2).
const uint8_t LED4_PIN = 7; ///< Пин светодиода LED4 (бит 3).
const uint8_t LED5_PIN = 8; ///< Пин светодиода LED5 (бит 4).
const uint8_t LED6_PIN = 9; ///< Пин светодиода LED6 (бит 5).

// Определения пинов для кнопок
static const uint8_t VOL_PLUS_PIN = 10;  ///< Пин кнопки "Громкость +".
static const uint8_t VOL_MINUS_PIN = 11; ///< Пин кнопки "Громкость -".

// Глобальные переменные для состояния светодиодов
static volatile uint8_t ledState = 0;            ///< Хранит состояние 6 светодиодов (биты [5:0]).
static volatile bool lastCommandReadLED = false; ///< Флаг, указывающий, что следующая операция чтения должна вернуть состояние светодиодов.

// Переменные для устранения дребезга и измерения длительности нажатия
static const uint64_t debounceDelay = 50 * 1000;       ///< Задержка для устранения дребезга (50 мс)
static const uint64_t longPressThreshold = 500 * 1000; ///< Порог длительного нажатия (500 мс)

static ButtonHandler volPlusButton(VOL_PLUS_PIN, debounceDelay, longPressThreshold);
static ButtonHandler volMinusButton(VOL_MINUS_PIN, debounceDelay, longPressThreshold);

// Прототипы функций

void receiveEvent(int);
void requestEvent(void);
uint64_t get_tick(void);

void setup()
{
    // Инициализация пинов светодиодов как выходов
    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    pinMode(LED3_PIN, OUTPUT);
    pinMode(LED4_PIN, OUTPUT);
    pinMode(LED5_PIN, OUTPUT);
    pinMode(LED6_PIN, OUTPUT);

    // Инициализация пинов кнопок как входов с внутренней подтяжкой
    pinMode(VOL_PLUS_PIN, INPUT_PULLUP);
    pinMode(VOL_MINUS_PIN, INPUT_PULLUP);

    // Выключение всех светодиодов
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    digitalWrite(LED3_PIN, LOW);
    digitalWrite(LED4_PIN, LOW);
    digitalWrite(LED5_PIN, LOW);
    digitalWrite(LED6_PIN, LOW);

    // Инициализация I2C и регистрация обработчиков событий
    Wire.begin(I2C_SLAVE_ADDRESS);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
}

void loop()
{
    uint64_t ticks = get_tick();
    volPlusButton.updateState(ticks);
    volMinusButton.updateState(ticks);
}

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
    if (received_bytes < 2)
        return; // Если получено меньше двух байтов, выходим

    uint8_t command = Wire.read();
    if (command != CMD_WRITE_LED)
        return; // Обработка только команды записи светодиодов

    uint8_t data = Wire.read();
    // Сохраняем состояние светодиодов (используем только биты [5:0])
    ledState = data & 0x3F;
    // Если бит [7] установлен, следующая операция чтения вернет состояние светодиодов
    lastCommandReadLED = (data & 0x80) ? true : false;

    // Обновление выходов для светодиодов
    digitalWrite(LED1_PIN, (ledState & 0x01) ? HIGH : LOW);
    digitalWrite(LED2_PIN, (ledState & 0x02) ? HIGH : LOW);
    digitalWrite(LED3_PIN, (ledState & 0x04) ? HIGH : LOW);
    digitalWrite(LED4_PIN, (ledState & 0x08) ? HIGH : LOW);
    digitalWrite(LED5_PIN, (ledState & 0x10) ? HIGH : LOW);
    digitalWrite(LED6_PIN, (ledState & 0x20) ? HIGH : LOW);
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
        // Устанавливаем бит 7
        response = ledState | 0x80;
        // Сброс флага после чтения
        lastCommandReadLED = false;
    }
    else // Формирование байта состояния кнопок:
    {
        // Для "Громкость +": бит 3 – текущее состояние, бит 4 – кратковременное нажатие, бит 5 – длительное нажатие
        (volPlusButton.isPressedNow()) ? response |= (1 << 3) : 0;
        (volPlusButton.isShortPress()) ? response |= (1 << 4) : 0;
        (volPlusButton.isLongPress()) ? response |= (1 << 5) : 0;

        // Для "Громкость -": бит 0 – текущее состояние, бит 1 – кратковременное нажатие, бит 2 – длительное нажатие
        (volMinusButton.isPressedNow()) ? response |= (1 << 0) : 0;
        (volMinusButton.isShortPress()) ? response |= (1 << 1) : 0;
        (volMinusButton.isLongPress()) ? response |= (1 << 2) : 0;
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