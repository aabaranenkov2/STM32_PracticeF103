#!/bin/bash

# Скрипт для переформатирования SD карты для Nextion
# Использует FAT32 с оптимальными параметрами

set -e

SD_DEVICE="/dev/sda"
SD_PARTITION="/dev/sda1"
SD_MOUNT="/media/alex60/NEXTION"
BACKUP_FILE="/tmp/update.tft.backup"

echo "════════════════════════════════════════════════════"
echo "  ПЕРЕФОРМАТИРОВАНИЕ SD КАРТЫ ДЛЯ NEXTION"
echo "════════════════════════════════════════════════════"
echo ""

# Проверка монтирования
if mountpoint -q "$SD_MOUNT"; then
    echo "📋 Создание резервной копии TFT файла..."
    if [ -f "$SD_MOUNT/update.tft" ]; then
        cp "$SD_MOUNT/update.tft" "$BACKUP_FILE"
        echo "✅ Резервная копия создана: $BACKUP_FILE"
    fi
    echo ""
    echo "📋 Размонтирование SD карты..."
    sudo umount "$SD_PARTITION" || true
    sleep 1
fi

# Проверка устройства
if [ ! -e "$SD_DEVICE" ]; then
    echo "❌ ОШИБКА: Устройство $SD_DEVICE не найдено"
    exit 1
fi

echo "⚠️  ВНИМАНИЕ: Все данные на SD карте будут удалены!"
echo ""
read -p "Продолжить? (y/n): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    exit 1
fi

echo ""
echo "📋 Проверка размеров..."
TOTAL_SIZE=$(lsblk -b -d -o SIZE -n "$SD_DEVICE")
TOTAL_SIZE_GB=$((TOTAL_SIZE / 1024 / 1024 / 1024))
echo "   Размер карты: ${TOTAL_SIZE_GB}GB"

if [ $TOTAL_SIZE_GB -gt 32 ]; then
    echo "⚠️  ВНИМАНИЕ: Карта больше 32GB"
    echo "   Nextion может не поддерживать карты больше 32GB"
    echo "   Рекомендуется использовать карту до 32GB"
    echo ""
    echo "📋 Будем создавать раздел размером 2GB для совместимости"
    PARTITION_SIZE_MB=2048
else
    PARTITION_SIZE_MB=0  # 0 = использовать весь диск
fi

echo ""
echo "📋 Удаление старой таблицы разделов..."
sudo wipefs -a "$SD_DEVICE" 2>/dev/null || true
sleep 1

echo ""
echo "📋 Создание новой таблицы разделов..."

if [ $PARTITION_SIZE_MB -gt 0 ]; then
    # Создать раздел размером 2GB
    echo "   Создание раздела размером ${PARTITION_SIZE_MB}MB..."
    sudo parted "$SD_DEVICE" --script mklabel msdos
    sudo parted "$SD_DEVICE" --script mkpart primary fat32 1MiB ${PARTITION_SIZE_MB}MiB
else
    # Использовать весь диск
    sudo parted "$SD_DEVICE" --script mklabel msdos
    sudo parted "$SD_DEVICE" --script mkpart primary fat32 1MiB 100%
fi

sleep 2

# Определить новое имя раздела
SD_PARTITION=$(lsblk -n -o NAME "$SD_DEVICE" | grep -v "^${SD_DEVICE##*/}$" | head -1)
if [ -n "$SD_PARTITION" ]; then
    SD_PARTITION="/dev/$SD_PARTITION"
else
    SD_PARTITION="${SD_DEVICE}1"
fi

echo "   Создан раздел: $SD_PARTITION"
sleep 1

echo ""
echo "📋 Форматирование в FAT32 с оптимальными параметрами..."
echo "   Размер кластера: 32KB (оптимально для Nextion)"

# Форматирование с размером кластера 32KB (для совместимости)
sudo mkfs.vfat -F 32 -S 512 -s 64 "$SD_PARTITION" || {
    echo "⚠️  Попытка 1 не удалась, пробую с параметрами по умолчанию..."
    sudo mkfs.vfat -F 32 "$SD_PARTITION" || {
        echo "❌ Ошибка форматирования"
        exit 1
    }
}

echo "✅ Форматирование завершено"
sleep 1

echo ""
echo "📋 Монтирование SD карты..."
sudo mkdir -p "$SD_MOUNT"
sudo mount "$SD_PARTITION" "$SD_MOUNT" || {
    echo "❌ Ошибка монтирования"
    exit 1
}

echo "✅ SD карта смонтирована"

# Восстановление TFT файла
if [ -f "$BACKUP_FILE" ]; then
    echo ""
    echo "📋 Восстановление TFT файла..."
    sudo cp "$BACKUP_FILE" "$SD_MOUNT/update.tft"
    sudo chmod 644 "$SD_MOUNT/update.tft"
    echo "✅ TFT файл восстановлен: update.tft"
    rm -f "$BACKUP_FILE"
fi

# Проверка результата
echo ""
echo "📋 Проверка результата..."
FSTYPE=$(lsblk -o FSTYPE -n "$SD_PARTITION")
PARTITION_SIZE=$(lsblk -o SIZE -n "$SD_PARTITION")
echo "   Файловая система: $FSTYPE"
echo "   Размер раздела: $PARTITION_SIZE"

if [ -f "$SD_MOUNT/update.tft" ]; then
    FILE_SIZE=$(stat -c%s "$SD_MOUNT/update.tft" 2>/dev/null || echo "0")
    echo "   Файл update.tft: $FILE_SIZE байт"
    echo "✅ Все готово!"
else
    echo "⚠️  Файл update.tft не найден (нужно скопировать вручную)"
fi

echo ""
echo "════════════════════════════════════════════════════"
echo "  ✅ SD КАРТА ПЕРЕФОРМАТИРОВАНА"
echo "════════════════════════════════════════════════════"
echo ""
echo "📋 ИНСТРУКЦИЯ ПО ЗАГРУЗКЕ:"
echo ""
echo "1. ❌ ОТКЛЮЧИТЕ питание Nextion"
echo ""
echo "2. 💾 ВСТАВЬТЕ microSD карту в Nextion"
echo ""
echo "3. ⚡ ПОДКЛЮЧИТЕ питание 5V к Nextion"
echo ""
echo "4. ⏳ ДОЖДИТЕСЬ загрузки (10-60 секунд)"
echo ""
echo "════════════════════════════════════════════════════"
echo ""

