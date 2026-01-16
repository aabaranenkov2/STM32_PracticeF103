#!/bin/bash

# Скрипт для исправления проблем с Nextion Editor в Wine

echo "════════════════════════════════════════════════════"
echo "  ИСПРАВЛЕНИЕ ПРОБЛЕМ С NEXTION EDITOR"
echo "════════════════════════════════════════════════════"
echo ""

# Проверка и установка winetricks
if ! command -v winetricks &> /dev/null; then
    echo "📋 Установка winetricks..."
    sudo apt update
    sudo apt install -y winetricks
    
    if [ $? -ne 0 ]; then
        echo "❌ Ошибка установки winetricks!"
        exit 1
    fi
    echo "   ✅ winetricks установлен"
else
    echo "✅ winetricks уже установлен"
fi

echo ""
echo "📋 Установка Wine Mono (.NET Framework)..."
echo "   Это может занять несколько минут..."
echo ""

# Установка Wine Mono
winetricks -q dotnet48

if [ $? -eq 0 ]; then
    echo "   ✅ Wine Mono установлен"
else
    echo "   ⚠️  Попробуем альтернативный вариант..."
    winetricks -q mono
fi

echo ""
echo "📋 Установка дополнительных компонентов..."
winetricks -q vcrun2015 vcrun2017

echo ""
echo "════════════════════════════════════════════════════"
echo "  ✅ ГОТОВО!"
echo "════════════════════════════════════════════════════"
echo ""
echo "📋 Теперь попробуйте запустить Nextion Editor:"
echo "   ./run_nextion.sh"
echo ""
echo "⚠️  Если проблемы остались:"
echo "   • Попробуйте: winetricks dotnet40"
echo "   • Или: winetricks dotnet35"
echo "   • Проверьте: winecfg (настройки Wine)"
echo ""

