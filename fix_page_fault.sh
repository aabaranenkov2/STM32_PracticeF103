#!/bin/bash

# Скрипт для исправления page fault ошибки

echo "════════════════════════════════════════════════════"
echo "  ИСПРАВЛЕНИЕ PAGE FAULT ОШИБКИ"
echo "════════════════════════════════════════════════════"
echo ""

export WINEPREFIX="$HOME/.wine"

echo "📋 Настройка Wine для .NET приложений..."
echo ""

# Настройка библиотек через winetricks
echo "1. Настройка библиотек..."
winetricks -q mscoree=native msvcr120=native

# Установка Visual C++ Redistributables
echo ""
echo "2. Установка Visual C++ Redistributables..."
winetricks -q vcrun2015 vcrun2017 vcrun2019

# Установка .NET Framework 4.0 (более стабильный)
echo ""
echo "3. Установка .NET Framework 4.0..."
winetricks -q dotnet40

# Дополнительные компоненты
echo ""
echo "4. Установка дополнительных компонентов..."
winetricks -q gdiplus msxml3 msxml6

echo ""
echo "════════════════════════════════════════════════════"
echo "  ✅ ГОТОВО!"
echo "════════════════════════════════════════════════════"
echo ""
echo "📋 Теперь попробуйте запустить:"
echo "   ./run_nextion.sh"
echo ""
echo "⚠️  Если проблема осталась:"
echo "   • Это известная проблема Wine с некоторыми .NET приложениями"
echo "   • Рассмотрите использование виртуальной машины Windows"
echo "   • Или попросите кого-то с Windows создать TFT файл"
echo ""

