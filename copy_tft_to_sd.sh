#!/bin/bash

# Скрипт для копирования TFT файла на microSD карту

MICROSD_DEVICE="/dev/sda1"
MOUNT_POINT="/media/alex60/mirex"

echo "════════════════════════════════════════════════════"
echo "  КОПИРОВАНИЕ TFT ФАЙЛА НА MICROSD"
echo "════════════════════════════════════════════════════"
echo ""

# Проверка аргументов
if [ -z "$1" ]; then
    echo "❌ Использование: $0 <путь_к_tft_файлу>"
    echo ""
    echo "Пример:"
    echo "  $0 /home/user/project.tft"
    echo ""
    echo "Или интерактивный режим:"
    read -p "Введите путь к TFT файлу: " TFT_FILE
else
    TFT_FILE="$1"
fi

# Проверка файла
if [ ! -f "$TFT_FILE" ]; then
    echo "❌ Файл не найден: $TFT_FILE"
    exit 1
fi

echo "📋 TFT файл: $TFT_FILE"
echo "   Размер: $(du -h "$TFT_FILE" | cut -f1)"
echo ""

# Проверка устройства
if [ ! -b "$MICROSD_DEVICE" ]; then
    echo "❌ Устройство $MICROSD_DEVICE не найдено!"
    echo "   Проверьте: lsblk"
    exit 1
fi

# Размонтирование (если смонтировано)
if mountpoint -q "$MOUNT_POINT" 2>/dev/null || mount | grep -q "$MICROSD_DEVICE"; then
    echo "📋 Размонтирование карты..."
    sudo umount "$MICROSD_DEVICE" 2>/dev/null || sudo umount "$MOUNT_POINT" 2>/dev/null
    sleep 1
fi

# Монтирование
echo "📋 Монтирование карты..."
sudo mkdir -p "$MOUNT_POINT"
sudo mount "$MICROSD_DEVICE" "$MOUNT_POINT"

if [ $? -ne 0 ]; then
    echo "❌ Ошибка монтирования!"
    exit 1
fi

# Копирование файла
echo "📋 Копирование TFT файла как 'update.tft'..."
sudo cp "$TFT_FILE" "$MOUNT_POINT/update.tft"

if [ $? -eq 0 ]; then
    echo "   ✅ Файл скопирован успешно!"
    echo ""
    echo "📋 Проверка:"
    ls -lh "$MOUNT_POINT/update.tft"
    echo ""
    echo "📋 Содержимое карты:"
    ls -lh "$MOUNT_POINT/"
else
    echo "   ❌ Ошибка копирования!"
    sudo umount "$MOUNT_POINT"
    exit 1
fi

# Размонтирование
echo ""
echo "📋 Размонтирование карты..."
sudo umount "$MOUNT_POINT"

echo ""
echo "════════════════════════════════════════════════════"
echo "  ✅ ГОТОВО!"
echo "════════════════════════════════════════════════════"
echo ""
echo "📋 Следующие шаги:"
echo "   1. Извлеките microSD карту из кардридера"
echo "   2. Вставьте карту в дисплей Nextion"
echo "   3. Подключите питание 5V к дисплею"
echo "   4. Дисплей автоматически загрузит прошивку"
echo ""

