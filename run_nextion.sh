#!/bin/bash

# Скрипт для запуска Nextion Editor

NEXTION_DIR="/media/alex60/INFO/Project-Bots/bots/Fitnes/ProjectsRST32/STM32_PracticeF103/nextion-setup-vLTS"
EXE_FILE="NextionEditor.exe"

echo "════════════════════════════════════════════════════"
echo "  ЗАПУСК NEXTION EDITOR"
echo "════════════════════════════════════════════════════"
echo ""

# Проверка Wine
if ! command -v wine &> /dev/null; then
    echo "❌ Wine не установлен!"
    echo "   Установите: sudo apt install wine"
    exit 1
fi

# Проверка файла
if [ ! -f "$NEXTION_DIR/$EXE_FILE" ]; then
    echo "❌ Файл не найден: $NEXTION_DIR/$EXE_FILE"
    exit 1
fi

echo "📋 Директория: $NEXTION_DIR"
echo "📋 Файл: $EXE_FILE"
echo ""
echo "🚀 Запуск Nextion Editor через Wine..."
echo "   (Первый запуск может занять время)"
echo ""

# Остановка предыдущих процессов
pkill -f "wine.*NextionEditor" 2>/dev/null
sleep 1

# Переход в директорию и запуск
cd "$NEXTION_DIR"

# Перенаправление ошибок в файл (опционально)
read -p "Скрыть ошибки Wine в терминале? (yes/no, по умолчанию no): " hide_errors

if [ "$hide_errors" = "yes" ]; then
    echo "   Ошибки будут сохранены в wine_errors.log"
    wine "./$EXE_FILE" 2>wine_errors.log &
    WINE_PID=$!
else
    wine "./$EXE_FILE" &
    WINE_PID=$!
fi

# Проверка через 2 секунды
sleep 2
if ! ps -p $WINE_PID > /dev/null 2>&1; then
    echo ""
    echo "⚠️  Процесс завершился слишком быстро!"
    echo "   Возможно, программа крашится при запуске."
    echo ""
    echo "📋 Попробуйте:"
    echo "   1. Режим отладки: ./run_nextion_debug.sh"
    echo "   2. Установить зависимости: ./fix_wine_ole.sh"
    echo "   3. Проверить Wine: winecfg"
fi

echo "   ✅ Nextion Editor запущен!"
echo ""
echo "⚠️  Примечание:"
echo "   • Ошибки OLE/DLL в Wine - это нормально и не критично"
echo "   • Если программа работает - можно игнорировать ошибки"
echo "   • Для уменьшения ошибок: ./fix_wine_ole.sh"
echo "   • Для настройки Wine: winecfg"
echo ""

