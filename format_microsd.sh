#!/bin/bash

# Скрипт для форматирования microSD в FAT32 и копирования TFT файла

set -e  # Остановить при ошибке

MICROSD_DEVICE="/dev/sda1"
MOUNT_POINT="/media/alex60/mirex"
TFT_FILE=""

echo "════════════════════════════════════════════════════"
echo "  ФОРМАТИРОВАНИЕ MICROSD ДЛЯ NEXTION"
echo "════════════════════════════════════════════════════"
echo ""

# Проверка, что устройство существует
if [ ! -b "$MICROSD_DEVICE" ]; then
    echo "❌ Ошибка: Устройство $MICROSD_DEVICE не найдено!"
    echo "   Проверьте: lsblk"
    exit 1
fi

echo "📋 Информация об устройстве:"
echo "   Устройство: $MICROSD_DEVICE"
lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE | grep sda
echo ""

# Проверка, смонтировано ли устройство
if mountpoint -q "$MOUNT_POINT" 2>/dev/null || mount | grep -q "$MICROSD_DEVICE"; then
    echo "📋 Размонтирование устройства..."
    sudo umount "$MICROSD_DEVICE" 2>/dev/null || sudo umount "$MOUNT_POINT" 2>/dev/null
    echo "   ✅ Размонтировано"
    sleep 1
fi

# Предупреждение
echo "⚠️  ВНИМАНИЕ: Все данные на карте будут удалены!"
read -p "   Продолжить? (yes/no): " confirm

if [ "$confirm" != "yes" ]; then
    echo "❌ Отменено пользователем"
    exit 0
fi

echo ""
echo "📋 Форматирование в FAT32..."
echo "   Это может занять несколько секунд..."

# Форматирование
sudo mkfs.vfat -F 32 -n "NEXTION" "$MICROSD_DEVICE"

if [ $? -eq 0 ]; then
    echo "   ✅ Форматирование завершено успешно!"
else
    echo "   ❌ Ошибка форматирования!"
    exit 1
fi

echo ""
echo "📋 Проверка результата..."
lsblk -f "$MICROSD_DEVICE" | grep -A 1 sda1
echo ""

# Спрашиваем, нужно ли копировать TFT файл
read -p "📋 Скопировать TFT файл на карту? (yes/no): " copy_tft

if [ "$copy_tft" = "yes" ]; then
    read -p "   Введите путь к TFT файлу: " TFT_FILE
    
    if [ -z "$TFT_FILE" ] || [ ! -f "$TFT_FILE" ]; then
        echo "   ❌ Файл не найден: $TFT_FILE"
        exit 1
    fi
    
    # Монтируем карту
    echo ""
    echo "📋 Монтирование карты..."
    sudo mkdir -p "$MOUNT_POINT"
    sudo mount "$MICROSD_DEVICE" "$MOUNT_POINT"
    
    # Копируем файл с переименованием
    echo "📋 Копирование TFT файла как 'update.tft'..."
    sudo cp "$TFT_FILE" "$MOUNT_POINT/update.tft"
    
    if [ $? -eq 0 ]; then
        echo "   ✅ Файл скопирован успешно!"
        echo ""
        echo "📋 Проверка:"
        ls -lh "$MOUNT_POINT/update.tft"
    else
        echo "   ❌ Ошибка копирования!"
    fi
    
    # Размонтируем
    echo ""
    echo "📋 Размонтирование карты..."
    sudo umount "$MOUNT_POINT"
    echo "   ✅ Готово!"
fi

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

