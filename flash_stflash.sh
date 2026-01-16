#!/bin/bash
echo "════════════════════════════════════════════════════"
echo "  ПРОШИВКА ЧЕРЕЗ st-flash (ПРЯМОЙ МЕТОД)"
echo "════════════════════════════════════════════════════"
echo ""

# Проверка st-flash
if ! command -v st-flash &> /dev/null; then
    echo "❌ st-flash не установлен!"
    echo "Установите: sudo apt install stlink-tools"
    exit 1
fi

# Компиляция
echo "📦 Компиляция проекта..."
cd "$(dirname "$0")"
pio run 2>&1 | tail -10

if [ $? -ne 0 ]; then
    echo "❌ Ошибка компиляции!"
    exit 1
fi

FIRMWARE=".pio/build/bluepill_f103c8/firmware.bin"

if [ ! -f "$FIRMWARE" ]; then
    echo "❌ Файл прошивки не найден: $FIRMWARE"
    exit 1
fi

echo "✅ Прошивка скомпилирована"
SIZE=$(ls -lh "$FIRMWARE" | awk '{print $5}')
echo "   Размер: $SIZE"
echo ""

# Проверка подключения
echo "🔍 Проверка подключения..."
st-info --probe 2>&1 | head -5

echo ""
read -p "Нажмите ENTER для начала прошивки..."
echo ""

# Прошивка через st-flash
echo "🚀 Прошивка через st-flash..."
echo ""

# Сначала стираем
echo "Шаг 1: Стирание памяти..."
st-flash erase 2>&1

sleep 1

# Затем прошиваем
echo ""
echo "Шаг 2: Загрузка прошивки..."
st-flash write "$FIRMWARE" 0x8000000 2>&1

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ ПРОШИВКА УСПЕШНА!"
    echo ""
    echo "Контроллер должен автоматически перезагрузиться"
    echo "LED должен начать мигать"
else
    echo ""
    echo "❌ Ошибка прошивки"
    echo ""
    echo "Попробуйте:"
    echo "1. Проверить подключение ST-Link"
    echo "2. Запустить ./unlock_chip.sh"
    echo "3. Проверить BOOT0 (должен быть на GND)"
fi

echo ""
echo "════════════════════════════════════════════════════"


