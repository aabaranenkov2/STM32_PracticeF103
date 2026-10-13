#!/bin/bash
echo "=== Проверка PlatformIO ==="
echo ""
echo "1. Проверка platformio.ini:"
if [ -f "platformio.ini" ]; then
    echo "   ✓ platformio.ini найден"
else
    echo "   ✗ platformio.ini НЕ найден!"
fi
echo ""
echo "2. Проверка расширений Cursor:"
cursor --list-extensions | grep -E "cpptools|platformio" || echo "   Расширения не найдены"
echo ""
echo "3. Проверка PlatformIO в venv:"
source /media/alex60/INFO/Project-Bots/bots/Fitnes/venv/bin/activate
if command -v pio &> /dev/null; then
    echo "   ✓ PlatformIO установлен: $(pio --version)"
else
    echo "   ✗ PlatformIO не найден в PATH"
fi
echo ""
echo "4. Текущая директория:"
pwd
echo ""
echo "5. Проверка workspace файла:"
if [ -f "STM32_PracticeF103.code-workspace" ]; then
    echo "   ✓ Workspace файл найден"
else
    echo "   ✗ Workspace файл НЕ найден!"
fi
