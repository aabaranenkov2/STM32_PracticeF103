#!/bin/bash

# Скрипт для установки Nextion Editor через Wine на Linux

echo "════════════════════════════════════════════════════"
echo "  УСТАНОВКА NEXTION EDITOR ЧЕРЕЗ WINE"
echo "════════════════════════════════════════════════════"
echo ""

# Проверка Wine
if ! command -v wine &> /dev/null; then
    echo "📋 Wine не установлен. Устанавливаю..."
    sudo apt update
    sudo apt install -y wine wine64
    
    if [ $? -ne 0 ]; then
        echo "❌ Ошибка установки Wine!"
        exit 1
    fi
    echo "   ✅ Wine установлен"
else
    echo "✅ Wine уже установлен"
    wine --version
fi

echo ""
echo "📋 Выбор источника Nextion Editor..."
echo ""
echo "Выберите вариант:"
echo "  1) Указать путь к ZIP архиву (будет распакован)"
echo "  2) Указать путь к уже распакованной папке"
echo ""
read -p "Ваш выбор (1 или 2): " choice

INSTALL_DIR=""

if [ "$choice" = "1" ]; then
    # Вариант 1: Распаковка архива
    echo ""
    echo "📋 Поиск архива..."
    
    ZIP_FILE=""
    if [ -f "nextion-setup-vLTS.zip" ]; then
        ZIP_FILE="nextion-setup-vLTS.zip"
    elif [ -f "$HOME/Downloads/nextion-setup-vLTS.zip" ]; then
        ZIP_FILE="$HOME/Downloads/nextion-setup-vLTS.zip"
    else
        read -p "   Введите полный путь к ZIP архиву: " ZIP_FILE
    fi
    
    if [ ! -f "$ZIP_FILE" ]; then
        echo "❌ Файл не найден: $ZIP_FILE"
        exit 1
    fi
    
    echo "   ✅ Найден: $ZIP_FILE"
    
    # Создание директории для Nextion
    INSTALL_DIR="$HOME/nextion-editor"
    echo ""
    echo "📋 Распаковка архива в: $INSTALL_DIR"
    mkdir -p "$INSTALL_DIR"
    cd "$INSTALL_DIR"
    unzip -q "$ZIP_FILE" -d .
    
    if [ $? -ne 0 ]; then
        echo "❌ Ошибка распаковки!"
        exit 1
    fi
    
    echo "   ✅ Распаковано"
    
elif [ "$choice" = "2" ]; then
    # Вариант 2: Уже распакованная папка
    echo ""
    echo "📋 Укажите путь к распакованной папке..."
    
    # Предлагаем варианты
    if [ -d "nextion-setup-vLTS" ]; then
        DEFAULT_DIR="$(pwd)/nextion-setup-vLTS"
    elif [ -d "$HOME/nextion-editor" ]; then
        DEFAULT_DIR="$HOME/nextion-editor"
    else
        DEFAULT_DIR=""
    fi
    
    if [ -n "$DEFAULT_DIR" ]; then
        echo "   Найдена папка: $DEFAULT_DIR"
        read -p "   Использовать её? (yes/no): " use_default
        if [ "$use_default" = "yes" ]; then
            INSTALL_DIR="$DEFAULT_DIR"
        else
            read -p "   Введите полный путь к папке: " INSTALL_DIR
        fi
    else
        read -p "   Введите полный путь к папке: " INSTALL_DIR
    fi
    
    if [ ! -d "$INSTALL_DIR" ]; then
        echo "❌ Папка не найдена: $INSTALL_DIR"
        exit 1
    fi
    
    echo "   ✅ Папка найдена: $INSTALL_DIR"
    cd "$INSTALL_DIR"
    
else
    echo "❌ Неверный выбор!"
    exit 1
fi

# Поиск .exe файла
echo ""
echo "📋 Поиск исполняемого файла..."
EXE_FILE=$(find . -name "*.exe" -type f | head -1)

if [ -z "$EXE_FILE" ]; then
    echo "❌ .exe файл не найден!"
    echo ""
    echo "   Содержимое папки:"
    ls -la | head -20
    echo ""
    echo "   Попробуйте найти .exe файл вручную и запустить:"
    echo "   wine /путь/к/файлу.exe"
    exit 1
fi

# Убрать ./ из начала пути
EXE_FILE=$(echo "$EXE_FILE" | sed 's|^\./||')
echo "   ✅ Найден: $EXE_FILE"
echo ""
echo "════════════════════════════════════════════════════"
echo "  ЗАПУСК NEXTION EDITOR"
echo "════════════════════════════════════════════════════"
echo ""
echo "Для запуска Nextion Editor выполните:"
echo ""
echo "  cd $INSTALL_DIR"
echo "  wine ./$EXE_FILE"
echo ""
echo "Или создайте ярлык:"
echo ""
echo "  cat > ~/.local/share/applications/nextion-editor.desktop << 'DESKTOP'"
echo "  [Desktop Entry]"
echo "  Name=Nextion Editor"
echo "  Exec=wine $INSTALL_DIR/$EXE_FILE"
echo "  Type=Application"
echo "  Categories=Development;"
echo "  DESKTOP"
echo ""

# Спрашиваем, запустить ли сейчас
read -p "Запустить Nextion Editor сейчас? (yes/no): " run_now

if [ "$run_now" = "yes" ]; then
    echo ""
    echo "🚀 Запуск Nextion Editor..."
    cd "$INSTALL_DIR"
    wine "./$EXE_FILE" &
    echo "   ✅ Nextion Editor запущен!"
    echo ""
    echo "⚠️  Примечание:"
    echo "   • Первый запуск может занять время (настройка Wine)"
    echo "   • Могут появиться окна установки - следуйте инструкциям"
    echo "   • Если что-то не работает, попробуйте: winecfg"
fi

echo ""
echo "════════════════════════════════════════════════════"
echo "  ✅ ГОТОВО!"
echo "════════════════════════════════════════════════════"
echo ""
echo "📋 Полезные команды:"
echo "   • Настройка Wine: winecfg"
echo "   • Проверка версии: wine --version"
echo "   • Удаление Wine: sudo apt remove wine"
echo ""

