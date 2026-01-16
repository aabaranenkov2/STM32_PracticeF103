#!/bin/bash

# Скрипт для запуска Nextion Editor с отладкой

NEXTION_DIR="/media/alex60/INFO/Project-Bots/bots/Fitnes/ProjectsRST32/STM32_PracticeF103/nextion-setup-vLTS"
EXE_FILE="NextionEditor.exe"
LOG_FILE="nextion_debug.log"

echo "════════════════════════════════════════════════════"
echo "  ЗАПУСК NEXTION EDITOR (РЕЖИМ ОТЛАДКИ)"
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
echo "📋 Лог файл: $LOG_FILE"
echo ""

# Остановка предыдущих процессов
echo "📋 Проверка запущенных процессов..."
pkill -f "wine.*NextionEditor" 2>/dev/null
sleep 1

# Настройка Wine для отладки
export WINEDEBUG=+all
export WINEPREFIX="$HOME/.wine"

echo "📋 Запуск Nextion Editor..."
echo "   Все ошибки будут сохранены в $LOG_FILE"
echo "   Программа запустится в foreground режиме"
echo "   (Нажмите Ctrl+C для остановки)"
echo ""

# Переход в директорию и запуск
cd "$NEXTION_DIR"

# Запуск с полным логированием
wine "./$EXE_FILE" 2>&1 | tee "$LOG_FILE" &

WINE_PID=$!
echo "   PID процесса: $WINE_PID"
echo ""

# Ждем немного и проверяем
sleep 3

if ps -p $WINE_PID > /dev/null; then
    echo "   ✅ Процесс запущен!"
    echo ""
    echo "📋 Если окно не появляется:"
    echo "   • Проверьте логи: cat $LOG_FILE"
    echo "   • Попробуйте: WINEPREFIX=~/.wine winecfg"
    echo "   • Установите зависимости: ./fix_wine_ole.sh"
    echo ""
    echo "   Ожидание... (Ctrl+C для остановки)"
    wait $WINE_PID
else
    echo "   ❌ Процесс завершился!"
    echo ""
    echo "📋 Проверьте логи:"
    echo "   cat $LOG_FILE | tail -50"
    echo ""
    echo "📋 Возможные решения:"
    echo "   1. Установите зависимости: ./fix_wine_ole.sh"
    echo "   2. Проверьте Wine: winecfg"
    echo "   3. Попробуйте другую версию Wine"
fi

