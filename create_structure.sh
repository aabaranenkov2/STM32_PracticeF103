#!/bin/bash
echo "════════════════════════════════════════════════════"
echo "  СОЗДАНИЕ МОДУЛЬНОЙ СТРУКТУРЫ ПРОЕКТА"
echo "════════════════════════════════════════════════════"
echo ""

cd "$(dirname "$0")"

echo "📁 Создание директорий..."
echo ""

# Создание структуры include/
mkdir -p include/sensors
mkdir -p include/utils
mkdir -p include/config

# Создание структуры src/
mkdir -p src/sensors
mkdir -p src/utils

# Создание lib/ и test/
mkdir -p lib
mkdir -p test

echo "✅ Директории созданы:"
echo "   include/sensors/"
echo "   include/utils/"
echo "   include/config/"
echo "   src/sensors/"
echo "   src/utils/"
echo "   lib/"
echo "   test/"
echo ""

# Создание README файлов
echo "📝 Создание README файлов..."

cat > include/README << 'EOF'
# Заголовочные файлы (Headers)

Здесь находятся заголовочные файлы (.h) с объявлениями функций и структур.

## Структура:
- `sensors/` - заголовки драйверов датчиков
- `utils/` - утилиты и вспомогательные функции
- `config/` - конфигурационные файлы
EOF

cat > lib/README << 'EOF'
# Внешние библиотеки

Здесь можно разместить внешние библиотеки, если они не устанавливаются через PlatformIO.

## Использование:
Добавьте библиотеки в `platformio.ini`:
```ini
lib_dirs = lib
lib_deps = 
    библиотека1
    библиотека2
```
EOF

cat > test/README << 'EOF'
# Unit-тесты

Здесь находятся тесты для проверки работы кода.

## Запуск тестов:
```bash
pio test
```
EOF

echo "✅ README файлы созданы"
echo ""

echo "════════════════════════════════════════════════════"
echo "✅ Структура создана!"
echo ""
echo "📋 Текущая структура проекта:"
tree -L 2 -d 2>/dev/null || find . -type d -maxdepth 2 | grep -v "^\./\." | sort
echo ""
echo "💡 Теперь можно:"
echo "   1. Создать заголовочные файлы в include/"
echo "   2. Реализовать функции в src/"
echo "   3. Добавить библиотеки в lib/"
echo "   4. Написать тесты в test/"
echo ""
echo "════════════════════════════════════════════════════"


