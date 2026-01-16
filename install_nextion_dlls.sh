#!/bin/bash

# Скрипт для установки недостающих DLL для Nextion Editor

echo "════════════════════════════════════════════════════"
echo "  УСТАНОВКА DLL ДЛЯ NEXTION EDITOR"
echo "════════════════════════════════════════════════════"
echo ""

# Проверка winetricks
if ! command -v winetricks &> /dev/null; then
    echo "📋 Установка winetricks..."
    sudo apt update
    sudo apt install -y winetricks
    echo "   ✅ winetricks установлен"
fi

echo "📋 Установка необходимых DLL и компонентов..."
echo "   Это может занять несколько минут..."
echo ""

# Основные DLL для .NET приложений
echo "1. Установка Visual C++ Redistributables..."
winetricks -q vcrun2015 vcrun2017 vcrun2019

# Дополнительные компоненты
echo ""
echo "2. Установка дополнительных компонентов..."
winetricks -q gdiplus msxml3 msxml6

# Windows компоненты
echo ""
echo "3. Установка Windows компонентов..."
winetricks -q comctl32 comdlg32 shell32

# DirectX (может понадобиться для графики)
echo ""
echo "4. Установка DirectX..."
winetricks -q d3dx9 d3dx10 d3dx11_43

# .NET Framework (если еще не установлен)
echo ""
echo "5. Проверка .NET Framework..."
if ! winetricks -q dotnet48 2>/dev/null; then
    echo "   Установка .NET Framework 4.8..."
    winetricks dotnet48
else
    echo "   ✅ .NET Framework уже установлен"
fi

echo ""
echo "════════════════════════════════════════════════════"
echo "  ✅ УСТАНОВКА ЗАВЕРШЕНА!"
echo "════════════════════════════════════════════════════"
echo ""
echo "📋 Теперь перезапустите Nextion Editor:"
echo "   ./run_nextion.sh"
echo ""
echo "⚠️  Примечание:"
echo "   • Ошибки DLL могут все еще появляться"
echo "   • Если элементы работают - это нормально"
echo "   • Wine не идеально эмулирует Windows DLL"
echo ""
echo "📋 Если проблемы остались:"
echo "   • Попробуйте: winetricks allfonts"
echo "   • Или: winetricks corefonts"
echo "   • Проверьте: winecfg (настройки Wine)"
echo ""

