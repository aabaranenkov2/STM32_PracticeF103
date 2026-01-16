#!/bin/bash
echo "════════════════════════════════════════════════════"
echo "  ПРОВЕРКА ST-LINK И ПОДКЛЮЧЕНИЯ"
echo "════════════════════════════════════════════════════"
echo ""

# 1. Проверка USB устройства
echo "1️⃣ Проверка USB устройств..."
if lsusb | grep -i "stm\|st-link" > /dev/null; then
    echo "   ✅ ST-Link найден в USB:"
    lsusb | grep -i "stm\|st-link"
else
    echo "   ❌ ST-Link НЕ найден в USB!"
    echo "   Проверьте подключение кабеля"
    exit 1
fi

echo ""

# 2. Проверка st-info
echo "2️⃣ Проверка st-info..."
if command -v st-info &> /dev/null; then
    echo "   ✅ st-info установлен"
    echo ""
    echo "   Попытка подключения к STM32..."
    st-info --probe 2>&1 | head -10
else
    echo "   ⚠️  st-info не установлен"
    echo "   Установите: sudo apt install stlink-tools"
fi

echo ""

# 3. Проверка OpenOCD
echo "3️⃣ Проверка OpenOCD..."
if command -v openocd &> /dev/null; then
    echo "   ✅ OpenOCD установлен"
    VERSION=$(openocd -v 2>&1 | head -1)
    echo "   Версия: $VERSION"
else
    echo "   ⚠️  OpenOCD не установлен"
    echo "   Установите: sudo apt install openocd"
fi

echo ""

# 4. Попытка подключения через OpenOCD
echo "4️⃣ Попытка подключения через OpenOCD..."
timeout 3 openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
    -c "init; halt; stm32f1x options_read 0; exit" 2>&1 | \
    grep -E "(Info|Error|Warn|target|Cortex|options)" || echo "   ⚠️  Не удалось подключиться"

echo ""
echo "════════════════════════════════════════════════════"


