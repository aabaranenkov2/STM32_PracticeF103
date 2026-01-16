#!/bin/bash

# Скрипт для исправления OLE ошибок в Wine

echo "════════════════════════════════════════════════════"
echo "  ИСПРАВЛЕНИЕ OLE ОШИБОК В WINE"
echo "════════════════════════════════════════════════════"
echo ""

echo "⚠️  Важно: Ошибки OLE в Wine - это нормально!"
echo "   Если Nextion Editor работает - можно игнорировать."
echo ""

# Проверка winetricks
if ! command -v winetricks &> /dev/null; then
    echo "📋 Установка winetricks..."
    sudo apt update
    sudo apt install -y winetricks
fi

echo "📋 Установка компонентов для улучшения совместимости..."
echo ""

# Установка компонентов для OLE/COM
echo "1. Установка OLE/COM компонентов..."
winetricks -q ole32 oleaut32

# Установка Windows компонентов
echo ""
echo "2. Установка Windows компонентов..."
winetricks -q comctl32 comdlg32 shell32 shlwapi

# Установка Visual C++ (может помочь с COM)
echo ""
echo "3. Установка Visual C++ Redistributables..."
winetricks -q vcrun2015 vcrun2017 vcrun2019

# Настройка Wine для лучшей совместимости
echo ""
echo "4. Настройка Wine..."
export WINEPREFIX="$HOME/.wine"

# Установка Windows версии (может помочь)
echo ""
echo "5. Установка Windows версии (Windows 10)..."
winetricks -q win10

echo ""
echo "════════════════════════════════════════════════════"
echo "  ✅ ГОТОВО!"
echo "════════════════════════════════════════════════════"
echo ""
echo "📋 Рекомендации:"
echo ""
echo "1. Перезапустите Nextion Editor:"
echo "   ./run_nextion.sh"
echo ""
echo "2. Если ошибки остались (это нормально):"
echo "   • Игнорируйте их, если программа работает"
echo "   • Ошибки OLE не критичны для Nextion Editor"
echo "   • Главное - элементы работают и проект компилируется"
echo ""
echo "3. Для уменьшения вывода ошибок в терминал:"
echo "   wine ./NextionEditor.exe 2>/dev/null"
echo "   (но тогда не увидите важные ошибки)"
echo ""
echo "4. Альтернатива - перенаправить ошибки в файл:"
echo "   wine ./NextionEditor.exe 2>wine_errors.log"
echo ""

