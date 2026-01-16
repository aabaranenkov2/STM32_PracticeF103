# 🚀 БЫСТРЫЙ СТАРТ - STM32F103C8T6

## ✅ Статус: ВСЁ РАБОТАЕТ!

Контроллер успешно прошивается и работает!

---

## 📝 Как изменить код и прошить:

### Вариант 1: Автоматический скрипт (РЕКОМЕНДУЕТСЯ)
```bash
cd /media/alex60/INFO/Project-Bots/bots/Fitnes/ProjectsRST32/STM32_PracticeF103

# 1. Отредактируйте src/main.c
# 2. Запустите:
./flash_stflash.sh
```

### Вариант 2: Вручную
```bash
cd /media/alex60/INFO/Project-Bots/bots/Fitnes/ProjectsRST32/STM32_PracticeF103

# 1. Активируйте виртуальное окружение
source /media/alex60/INFO/Project-Bots/bots/Fitnes/venv/bin/activate

# 2. Скомпилируйте
python3 -m platformio run

# 3. Прошейте
st-flash erase
st-flash write .pio/build/bluepill_f103c8/firmware.bin 0x8000000
```

---

## 🔧 Полезные команды:

### Проверка подключения
```bash
./check_stlink.sh
```

### Разблокировка (если нужно)
```bash
./unlock_chip.sh
```

### Полная диагностика
```bash
./simple_test.sh
```

---

## 📂 Структура проекта:

```
STM32_PracticeF103/
├── src/
│   └── main.c              # Основной код (редактируйте здесь)
├── platformio.ini          # Конфигурация PlatformIO
├── flash_stflash.sh        # Скрипт прошивки (используйте этот)
├── check_stlink.sh         # Проверка подключения
└── README_FLASH.md         # Подробная инструкция
```

---

## 💡 Примеры кода:

### Изменить частоту мигания:
В `src/main.c` измените значения `HAL_Delay()`:
```c
HAL_Delay(500);  // 0.5 секунды вместо 1 секунды
```

### Добавить несколько миганий:
```c
// 3 быстрых мигания, затем пауза
for (int i = 0; i < 3; i++) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_Delay(100);
}
HAL_Delay(1000);
```

---

## ⚠️ Важные моменты:

1. **BOOT0 должен быть на GND** (0) для нормальной работы
2. **LED на PC13** - активен LOW (GPIO_PIN_RESET = включен)
3. **После прошивки** контроллер перезагружается автоматически
4. **Если не мигает** - нажмите RESET на плате

---

## 🎯 Следующие шаги:

1. ✅ Контроллер работает
2. ✅ Прошивка работает
3. 📝 Можно начинать добавлять функциональность:
   - Работа с датчиками (I2C, SPI, UART)
   - Таймеры и прерывания
   - АЦП для аналоговых датчиков
   - И т.д.

---

## 📞 Если что-то не работает:

1. Проверьте подключение: `./check_stlink.sh`
2. Разблокируйте: `./unlock_chip.sh`
3. Попробуйте другой метод прошивки
4. Смотрите `README_FLASH.md` для подробной диагностики

---

**Удачи в разработке! 🚀**


