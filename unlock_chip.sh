#!/bin/bash
echo "════════════════════════════════════════════════════"
echo "  РАЗБЛОКИРОВКА STM32F103C8T6"
echo "════════════════════════════════════════════════════"
echo ""
echo "⚠️  ВНИМАНИЕ: Это стирает всю прошивку!"
echo ""

read -p "Продолжить? (y/n): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    exit 1
fi

echo ""
echo "🔓 Шаг 1: Попытка разблокировки через OpenOCD..."
echo ""

# Метод 1: OpenOCD mass erase
timeout 10 openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
    -c "init; halt; stm32f1x unlock 0; stm32f1x mass_erase 0; exit" 2>&1 | \
    tee /tmp/unlock.log

if grep -q "target halted\|mass erase" /tmp/unlock.log; then
    echo "   ✅ Разблокировка выполнена"
else
    echo "   ⚠️  Возможна ошибка, проверьте лог"
fi

echo ""
echo "🔓 Шаг 2: Альтернативный метод (st-flash)..."
if command -v st-flash &> /dev/null; then
    echo "   Попытка через st-flash..."
    st-flash erase 2>&1 | head -5
else
    echo "   ⚠️  st-flash не установлен"
fi

echo ""
echo "════════════════════════════════════════════════════"
echo "Попробуйте теперь прошить контроллер"
echo "════════════════════════════════════════════════════"


