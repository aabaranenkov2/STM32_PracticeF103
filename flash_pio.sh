#!/bin/bash
echo "════════════════════════════════════════════════════"
echo "  ПРОШИВКА ЧЕРЕЗ PlatformIO (ВСТРОЕННЫЙ МЕТОД)"
echo "════════════════════════════════════════════════════"
echo ""

cd "$(dirname "$0")"

# Проверка PlatformIO
if ! command -v pio &> /dev/null; then
    echo "❌ PlatformIO не найден!"
    exit 1
fi

echo "📦 Компиляция и прошивка через PlatformIO..."
echo ""

# PlatformIO сам компилирует и прошивает
pio run --target upload 2>&1 | tee /tmp/pio_flash.log

EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "════════════════════════════════════════════════════"

if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ ПРОШИВКА УСПЕШНА!"
    echo ""
    echo "Контроллер должен автоматически перезагрузиться"
else
    echo "❌ Ошибка прошивки"
    echo ""
    echo "Последние строки лога:"
    tail -20 /tmp/pio_flash.log
    echo ""
    echo "Попробуйте:"
    echo "1. ./check_stlink.sh - проверить подключение"
    echo "2. ./unlock_chip.sh - разблокировать контроллер"
    echo "3. ./flash_stflash.sh - попробовать через st-flash"
fi

echo "════════════════════════════════════════════════════"


