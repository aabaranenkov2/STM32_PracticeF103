#!/bin/bash
echo "════════════════════════════════════════════════════"
echo "  ОЧИСТКА ПРОЕКТА STM32_PracticeF103"
echo "════════════════════════════════════════════════════"
echo ""
echo "⚠️  Этот скрипт удалит старые/дублирующиеся файлы"
echo ""

# Переход в директорию скрипта
cd "$(dirname "$0")"

# Список файлов для удаления (старые/дублирующиеся скрипты)
OLD_SCRIPTS=(
    "final_flash.sh"
    "compile_and_flash.sh"
    "debug_compile.sh"
    "flash_firmware.sh"
    "force_recompile.sh"
    "simple_flash.sh"
    "test_connection.sh"
    "try_slow_connection.sh"
    "try_with_reset.sh"
    "unlock_and_flash.sh"
    "unlock_with_reset.sh"
    "verify_and_flash.sh"
)

# Список старых/дублирующихся документов
OLD_DOCS=(
    "CHECK_CONNECTION.md"
    "NEXT_STEPS.md"
    "PHYSICAL_CHECK.md"
    "QUICK_FIX.md"
    "SUCCESS_GUIDE.md"
    "SUCCESS.md"
    "TROUBLESHOOTING.md"
    "WORKING.md"
)

# Проверка README.md (удалить только если есть README_FLASH.md)
if [ -f "README.md" ] && [ -f "README_FLASH.md" ]; then
    OLD_DOCS+=("README.md")
fi

# Показать что будет удалено
echo "📋 Файлы для удаления:"
echo ""
echo "Скрипты (${#OLD_SCRIPTS[@]} файлов):"
for file in "${OLD_SCRIPTS[@]}"; do
    if [ -f "$file" ]; then
        echo "  ❌ $file"
    fi
done

echo ""
echo "Документы (${#OLD_DOCS[@]} файлов):"
for file in "${OLD_DOCS[@]}"; do
    if [ -f "$file" ]; then
        echo "  ❌ $file"
    fi
done

echo ""
echo "════════════════════════════════════════════════════"
echo "📁 Файлы которые ОСТАНУТСЯ:"
echo ""
echo "Основные файлы:"
echo "  ✅ src/main.c              - основной код"
echo "  ✅ platformio.ini           - конфигурация проекта"
echo ""
echo "Рабочие скрипты:"
echo "  ✅ flash_stflash.sh        - прошивка (ОСНОВНОЙ)"
echo "  ✅ check_stlink.sh         - проверка подключения"
echo "  ✅ unlock_chip.sh          - разблокировка"
echo "  ✅ simple_test.sh          - полная диагностика"
if [ -f "flash_pio.sh" ]; then
    echo "  ✅ flash_pio.sh           - прошивка через PlatformIO (альтернатива)"
fi
echo ""
echo "Документация:"
echo "  ✅ QUICK_START.md          - быстрый старт"
echo "  ✅ README_FLASH.md         - подробная инструкция"
echo ""
echo "════════════════════════════════════════════════════"
echo ""

# Подтверждение
read -p "Продолжить удаление? (y/n): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Отменено."
    exit 0
fi

echo ""
echo "🗑️  Удаление файлов..."
echo ""

# Удаление старых скриптов
DELETED_COUNT=0
for file in "${OLD_SCRIPTS[@]}"; do
    if [ -f "$file" ]; then
        rm -f "$file"
        if [ $? -eq 0 ]; then
            echo "  ✅ Удален: $file"
            ((DELETED_COUNT++))
        else
            echo "  ❌ Ошибка при удалении: $file"
        fi
    fi
done

# Удаление старых документов
for file in "${OLD_DOCS[@]}"; do
    if [ -f "$file" ]; then
        rm -f "$file"
        if [ $? -eq 0 ]; then
            echo "  ✅ Удален: $file"
            ((DELETED_COUNT++))
        else
            echo "  ❌ Ошибка при удалении: $file"
        fi
    fi
done

echo ""
echo "════════════════════════════════════════════════════"
echo "✅ Очистка завершена!"
echo "   Удалено файлов: $DELETED_COUNT"
echo ""
echo "📁 Оставшиеся файлы:"
ls -1 *.sh *.md platformio.ini 2>/dev/null | sed 's/^/   /'
echo ""
echo "════════════════════════════════════════════════════"


