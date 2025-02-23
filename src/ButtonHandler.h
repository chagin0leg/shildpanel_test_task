#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

/**
 * @brief Класс для обработки нажатий кнопок с устранением дребезга и определением длительности нажатия.
 */
class ButtonHandler
{
public:
    /**
     * @brief Конструктор класса ButtonHandler.
     * @param pin Пин, к которому подключена кнопка.
     * @param debounceDelay Задержка для устранения дребезга в микросекундах.
     * @param longPressThreshold Порог длительного нажатия в микросекундах.
     */
    ButtonHandler(uint8_t pin, uint32_t debounceDelay, uint32_t longPressThreshold)
        : pin(pin), debounceDelay(debounceDelay), longPressThreshold(longPressThreshold) {}

    /**
     * @brief Обновляет состояние кнопки.
     * Функция считывает текущее состояние кнопки, устраняет дребезг и определяет,
     * является ли нажатие кратковременным или длительным.
     * @param ticks Текущее время в микросекундах.
     */
    void updateState(uint64_t ticks)
    {
        pinMode(pin, INPUT_PULLUP);
        bool pin_value = digitalRead(pin) ? true : false;
        if (pin_value != last_pin_value)
            lastDebounceTime = ticks + debounceDelay;
        if (ticks >= lastDebounceTime && pressed_f != (pin_value == LOW))
            if (pressed_f = (pin_value == LOW))
                lastDebounceTime = ticks + longPressThreshold;
            else if (ticks >= lastDebounceTime)
                longPress_f = true;
            else
                shortPress_f = true;
        last_pin_value = pin_value;
    }

    bool isPressedNow() const { return pressed_f; }                                ///< @brief Проверяет, нажата ли кнопка. @return true, если кнопка нажата, иначе false.
    bool isShortPress() { return shortPress_f ? !(shortPress_f = false) : false; } ///< @brief Проверяет, было ли кратковременное нажатие кнопки. @return true, если было кратковременное нажатие, иначе false.
    bool isLongPress() { return longPress_f ? !(longPress_f = false) : false; }    ///< @brief Проверяет, было ли длительное нажатие кнопки. @return true, если было длительное нажатие, иначе false.

private:
    const uint8_t pin;                 ///< Пин, к которому подключена кнопка.
    const uint32_t debounceDelay;      ///< Задержка для устранения дребезга в микросекундах.
    const uint32_t longPressThreshold; ///< Порог длительного нажатия в микросекундах.
    uint64_t lastDebounceTime = 0;     ///< Время последнего изменения состояния кнопки.
    bool last_pin_value;               ///< Предыдущее состояние пина кнопки (1 бит).
    bool pressed_f;                    ///< Текущее состояние кнопки (нажата или нет) (1 бит).
    bool shortPress_f;                 ///< Флаг кратковременного нажатия кнопки (1 бит).
    bool longPress_f;                  ///< Флаг длительного нажатия кнопки (1 бит).
};

#endif // BUTTON_HANDLER_H 