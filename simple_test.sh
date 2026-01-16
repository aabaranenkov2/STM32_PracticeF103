#!/bin/bash
echo "════════════════════════════════════════════════════"
echo "  ПОЛНАЯ ДИАГНОСТИКА И ПРОШИВКА"
echo "════════════════════════════════════════════════════"
echo ""

cd "$(dirname "$0")"

# Шаг 1: Проверка подключения
echo "1️⃣ Проверка ST-Link..."
./check_stlink.sh

if [ $? -ne 0 ]; then
    echo "❌ Проблема с подключением!"
    exit 1
fi

echo ""
read -p "Нажмите ENTER для продолжения..."
echo ""

# Шаг 2: Разблокировка (опционально)
echo "2️⃣ Разблокировка контроллера (опционально)..."
read -p "Выполнить разблокировку? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    ./unlock_chip.sh
    echo ""
    read -p "Нажмите ENTER для продолжения..."
fi

echo ""

# Шаг 3: Компиляция
echo "3️⃣ Компиляция проекта..."
pio run 2>&1 | tail -15

if [ $? -ne 0 ]; then
    echo "❌ Ошибка компиляции!"
    exit 1
fi

FIRMWARE=".pio/build/bluepill_f103c8/firmware.bin"
if [ ! -f "$FIRMWARE" ]; then
    echo "❌ Файл прошивки не найден!"
    exit 1
fi

echo "✅ Прошивка скомпилирована"
echo ""

# Шаг 4: Выбор метода прошивки
echo "4️⃣ Выбор метода прошивки:"
echo "   1) st-flash (прямой метод)"
echo "   2) PlatformIO (встроенный)"
echo "   3) OpenOCD (ручной)"
echo ""
read -p "Выберите метод (1/2/3): " method

case $method in
    1)
        echo ""
        ./flash_stflash.sh
        ;;
    2)
        echo ""
        ./flash_pio.sh
        ;;
    3)
        echo ""
        echo "🚀 Прошивка через OpenOCD..."
        openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
            -c "init; halt; stm32f1x mass_erase 0; program $FIRMWARE verify; reset run; exit" 2>&1 | \
            grep -E "(Info|Error|Programming|verified)"
        ;;
    *)
        echo "❌ Неверный выбор"
        exit 1
        ;;
esac

echo ""
echo "════════════════════════════════════════════════════"


