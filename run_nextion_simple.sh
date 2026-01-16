#!/bin/bash

# Простой запуск Nextion Editor без лишнего логирования

NEXTION_DIR="/media/alex60/INFO/Project-Bots/bots/Fitnes/ProjectsRST32/STM32_PracticeF103/nextion-setup-vLTS"
EXE_FILE="NextionEditor.exe"

echo "════════════════════════════════════════════════════"
echo "  ПРОСТОЙ ЗАПУСК NEXTION EDITOR"
echo "════════════════════════════════════════════════════"
echo ""

# Остановка предыдущих процессов
pkill -f "wine.*NextionEditor" 2>/dev/null
sleep 1

cd "$NEXTION_DIR"

# Запуск с минимальным логированием (только ошибки)
export WINEDEBUG=err+all
export WINEPREFIX="$HOME/.wine"

echo "🚀 Запуск Nextion Editor..."
echo "   (Ошибки будут показаны в терминале)"
echo ""

# Запуск в foreground для видимости ошибок
wine "./$EXE_FILE" 2>&1 | grep -i -E "error|fail|exception|warn" | head -20

# Если процесс завершился, покажем код выхода
EXIT_CODE=$?
if [ $EXIT_CODE -ne 0 ]; then
    echo ""
    echo "❌ Процесс завершился с кодом: $EXIT_CODE"
    echo ""
    echo "📋 Попробуйте:"
    echo "   1. Установить зависимости: ./install_nextion_dlls.sh"
    echo "   2. Проверить Wine: winecfg"
    echo "   3. Попробовать другую версию Wine"
fi

