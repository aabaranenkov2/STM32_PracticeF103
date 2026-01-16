#!/bin/bash

# Скрипт для проверки зависимостей Wine

echo "════════════════════════════════════════════════════"
echo "  ПРОВЕРКА ЗАВИСИМОСТЕЙ WINE"
echo "════════════════════════════════════════════════════"
echo ""

export WINEPREFIX="$HOME/.wine"

# Проверка .NET Framework
echo "📋 Проверка .NET Framework..."
if [ -d "$WINEPREFIX/drive_c/windows/Microsoft.NET" ]; then
    echo "   ✅ .NET Framework установлен"
    ls -la "$WINEPREFIX/drive_c/windows/Microsoft.NET" 2>/dev/null | head -5
else
    echo "   ❌ .NET Framework НЕ установлен!"
    echo "   Установите: winetricks dotnet48"
fi

echo ""

# Проверка Wine версии
echo "📋 Версия Wine:"
wine --version

echo ""

# Проверка winetricks
if command -v winetricks &> /dev/null; then
    echo "✅ winetricks установлен"
else
    echo "❌ winetricks НЕ установлен!"
    echo "   Установите: sudo apt install winetricks"
fi

echo ""

# Проверка Wine конфигурации
echo "📋 Проверка Wine конфигурации..."
if [ -f "$WINEPREFIX/system.reg" ]; then
    echo "   ✅ Wine prefix существует: $WINEPREFIX"
    
    # Проверка версии Windows
    if grep -q "Windows 10" "$WINEPREFIX/system.reg" 2>/dev/null; then
        echo "   ✅ Windows версия: Windows 10"
    elif grep -q "Windows 7" "$WINEPREFIX/system.reg" 2>/dev/null; then
        echo "   ⚠️  Windows версия: Windows 7 (рекомендуется Windows 10)"
    else
        echo "   ⚠️  Windows версия: неизвестна"
    fi
else
    echo "   ❌ Wine prefix не найден!"
    echo "   Запустите: winecfg"
fi

echo ""
echo "════════════════════════════════════════════════════"
echo "  РЕКОМЕНДАЦИИ"
echo "════════════════════════════════════════════════════"
echo ""

if [ ! -d "$WINEPREFIX/drive_c/windows/Microsoft.NET" ]; then
    echo "🔧 Установите .NET Framework:"
    echo "   winetricks dotnet48"
    echo ""
fi

echo "🔧 Если проблемы остаются:"
echo "   1. Переустановите Wine prefix:"
echo "      rm -rf ~/.wine"
echo "      winecfg"
echo "      winetricks dotnet48"
echo ""
echo "   2. Установите все зависимости:"
echo "      ./fix_wine_ole.sh"
echo "      ./install_nextion_dlls.sh"
echo ""

