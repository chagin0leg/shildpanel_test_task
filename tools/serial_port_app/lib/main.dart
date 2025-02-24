import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:flutter_libserialport/flutter_libserialport.dart';

void main() => runApp(const MyApp());

class MyApp extends StatelessWidget {
  const MyApp({super.key});
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'I2C Volume Level Keyboard Tester',
      theme: ThemeData(primarySwatch: Colors.blue),
      home: const MyHomePage(),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key});
  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  List<String> availablePorts = []; // Доступные последовательные порты
  String? selectedPort; // Выбранный последовательный порт
  bool isConnected = false; // Состояние подключения
  SerialPort? port; // Объект последовательного порта
  SerialPortReader? reader; // Читатель данных порта
  StreamSubscription<Uint8List>? readerSubscription; // Подписка на поток данных
  String logText = ""; // Текст лога
  final ScrollController _scrollController = ScrollController(); // Прокрутка
  int ledValue = 0; // Текущее состояние светодиодов (6 бит)
  bool volMPressed = false; // Состояния индикатора кнопки Vol-
  bool volPPressed = false; // Состояния индикатора кнопки Vol+
  Timer? portRefreshTimer; // Таймер для обновления списка портов

  @override
  void initState() {
    super.initState();
    updateAvailablePorts(); // Первоначальное получение списка портов
    portRefreshTimer = Timer.periodic(const Duration(seconds: 2), (timer) {
      updateAvailablePorts(); // Авто-обновление списка каждые 2 секунды
    });
  }

  // Обновление списка доступных портов
  void updateAvailablePorts() {
    List<String> ports = SerialPort.availablePorts;
    setState(() {
      availablePorts = ports;
      if (availablePorts.isNotEmpty) {
        if (selectedPort == null || !availablePorts.contains(selectedPort)) {
          selectedPort = availablePorts.first;
        }
      } else {
        selectedPort = null;
      }
    });
  }

  @override
  void dispose() {
    portRefreshTimer?.cancel(); // Отменяем таймер обновления портов
    readerSubscription?.cancel(); // Отменяем подписку на поток данных
    port?.close(); // Закрываем последовательный порт
    _scrollController.dispose(); // Освобождаем контроллер прокрутки
    super.dispose(); // Вызываем метод dispose родительского класса
  }

  // Подключение к выбранному последовательному порту
  void connectSerial() {
    if (selectedPort == null) {
      appendLog("Нет доступных COM портов для подключения!");
      return;
    }
    port = SerialPort(selectedPort!);
    if (!port!.openReadWrite()) {
      appendLog("Не удалось открыть порт: $selectedPort");
      return;
    }
    // Настройка порта (например, скорость 115200)
    final config = port!.config;
    config.baudRate = 115200;
    port!.config = config;

    reader = SerialPortReader(port!);
    readerSubscription = reader!.stream.listen((data) {
      try {
        // Используем latin1 для декодирования, чтобы не возникало ошибок с байтами > 127.
        String received = latin1.decode(data, allowInvalid: true);
        appendLog("Получено: $received");
        // Обработка индикаторов
        if (received.contains("Vol-: Кнопка нажата")) {
          setState(() => volMPressed = true);
        }
        if (received.contains("Vol-: Кнопка отпущена")) {
          setState(() => volMPressed = false);
        }
        if (received.contains("Vol+: Кнопка нажата")) {
          setState(() => volPPressed = true);
        }
        if (received.contains("Vol+: Кнопка отпущена")) {
          setState(() => volPPressed = false);
        }
      } catch (e) {
        appendLog("Ошибка декодирования: $e");
      }
    });
    setState(() => isConnected = true);
    appendLog("Подключено к порту: $selectedPort");
  }

  // Отключение от порта
  void disconnectSerial() {
    readerSubscription?.cancel(); // Отменяем подписку на поток данных
    port?.close(); // Закрываем последовательный порт
    setState(() => isConnected = false); // Обновляем состояние подключения
    appendLog("Отключено"); // Добавляем сообщение в лог
  }

  // Добавление строки в лог
  void appendLog(String text) {
    setState(() => logText += "$text\n");
    // Делаем небольшую задержку, чтобы гарантировать обновление виджета, затем прокручиваем вниз
    Future.delayed(const Duration(milliseconds: 100), () {
      _scrollController.animateTo(
        _scrollController.position.maxScrollExtent,
        duration: const Duration(milliseconds: 300),
        curve: Curves.easeOut,
      );
    });
  }

  // Отправка команды для управления светодиодами
  void sendLedCommand() {
    if (port == null || !isConnected) {
      appendLog("Порт не подключен!");
      return;
    }
    // Команда: первый байт – 0x40, второй – значение светодиодов (маска до 6 бит)
    final command = [0x40, ledValue & 0x3F];
    try {
      port!.write(Uint8List.fromList(command));
      appendLog("Отправлена команда: 0x${(ledValue & 0x3F).toRadixString(16)}");
    } catch (e) {
      appendLog("Ошибка передачи: $e");
    }
  }

  // Переключение состояния конкретного светодиода (индекс от 0 до 5)
  void toggleLed(int index) {
    setState(() => ledValue ^= (1 << index));
    sendLedCommand();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("I2C Volume Level Keyboard Tester")),
      body: Padding(
        padding: const EdgeInsets.all(8.0),
        child: Row(
          children: [
            SizedBox.fromSize(size: Size(16, 16)),
            // Первая колонка: 6 кнопок-светодиодов
            Expanded(
              child: Column(
                mainAxisAlignment:
                    MainAxisAlignment.center, // Центрирование по вертикали
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: List.generate(6, (index) {
                  bool isOn = (ledValue & (1 << index)) != 0;
                  return Padding(
                    padding: const EdgeInsets.symmetric(
                        vertical: 4.0), // Отступы между кнопками
                    child: SizedBox(
                      height: 32, // Фиксированная высота кнопки
                      child: ElevatedButton(
                        style: ElevatedButton.styleFrom(
                          backgroundColor: isOn ? Colors.green : Colors.grey,
                        ),
                        onPressed: () => toggleLed(index),
                        child: Text("LED ${index + 1}"),
                      ),
                    ),
                  );
                }),
              ),
            ),
            SizedBox.fromSize(size: Size(16, 16)),
            // Вторая колонка: 2 индикатора нажатия кнопок
            Expanded(
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  IndicatorWidget(label: "Vol-", pressed: volMPressed),
                  const SizedBox(height: 20),
                  IndicatorWidget(label: "Vol+", pressed: volPPressed),
                ],
              ),
            ),
            SizedBox.fromSize(size: Size(16, 16)),
            // Третья колонка: выбор порта, кнопка подключения и лог
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  availablePorts.isNotEmpty
                      ? DropdownButton<String>(
                          value: selectedPort,
                          items: availablePorts.map((port) {
                            return DropdownMenuItem(
                              value: port,
                              child: Text(port),
                            );
                          }).toList(),
                          onChanged: (value) {
                            setState(() {
                              selectedPort = value;
                            });
                          },
                        )
                      : const Center(
                          child: Text(
                            "Нет доступных COM портов",
                            style: TextStyle(color: Colors.red),
                          ),
                        ),
                  ElevatedButton(
                    onPressed: (isConnected || availablePorts.isEmpty)
                        ? disconnectSerial
                        : connectSerial,
                    child: Text(isConnected ? "Отключиться" : "Подключиться"),
                  ),
                  Expanded(
                    child: Container(
                      margin: const EdgeInsets.only(top: 8),
                      padding: const EdgeInsets.all(8),
                      color: Colors.black,
                      child: SingleChildScrollView(
                        controller: _scrollController,
                        child: Text(
                          logText,
                          style: const TextStyle(color: Colors.white),
                        ),
                      ),
                    ),
                  ),
                ],
              ),
            ),
            SizedBox.fromSize(size: Size(16, 16)),
          ],
        ),
      ),
    );
  }
}

// Виджет для индикатора (смена цвета в зависимости от состояния)
class IndicatorWidget extends StatelessWidget {
  final String label;
  final bool pressed;

  const IndicatorWidget(
      {super.key, required this.label, required this.pressed});

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: pressed ? Colors.red : Colors.green,
        borderRadius: BorderRadius.circular(8),
      ),
      child: Text(
        label,
        textAlign: TextAlign.center,
        style: const TextStyle(color: Colors.white, fontSize: 20),
      ),
    );
  }
}
