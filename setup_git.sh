#!/bin/bash

# Скрипт для быстрой настройки Git репозитория

PROJECT_DIR="/media/alex60/INFO/Project-Bots/bots/Fitnes/ProjectsRST32/STM32_PracticeF103"

echo "════════════════════════════════════════════════════"
echo "  НАСТРОЙКА GIT РЕПОЗИТОРИЯ"
echo "════════════════════════════════════════════════════"
echo ""

cd "$PROJECT_DIR"

# Проверка Git
if ! command -v git &> /dev/null; then
    echo "📋 Установка Git..."
    sudo apt update
    sudo apt install -y git
fi

# Создание .gitignore
echo "📋 Создание .gitignore..."
cat > .gitignore << 'EOF'
# PlatformIO
.pio/
.vscode/
*.elf
*.bin
*.hex
*.map

# Logs
*.log
wine_errors.log
nextion_debug.log
nextion_debug.log.*

# OS files
.DS_Store
Thumbs.db
*~

# Nextion Editor project files (большие, не нужны в Git)
*.HMI

# Но оставляем .tft файлы для синхронизации
# *.tft

# Wine
.wine/

# Временные файлы
*.tmp
*.bak
*.swp
EOF

echo "   ✅ .gitignore создан"

# Инициализация Git (если еще не инициализирован)
if [ ! -d ".git" ]; then
    echo ""
    echo "📋 Инициализация Git репозитория..."
    git init
    
    # Настройка пользователя (если еще не настроено)
    if [ -z "$(git config user.name)" ]; then
        read -p "Введите ваше имя для Git: " git_name
        git config user.name "$git_name"
    fi
    
    if [ -z "$(git config user.email)" ]; then
        read -p "Введите ваш email для Git: " git_email
        git config user.email "$git_email"
    fi
    
    echo "   ✅ Git инициализирован"
else
    echo ""
    echo "✅ Git репозиторий уже существует"
fi

# Добавление файлов
echo ""
echo "📋 Добавление файлов в Git..."
git add .

# Проверка статуса
echo ""
echo "📋 Статус репозитория:"
git status --short | head -20

echo ""
echo "════════════════════════════════════════════════════"
echo "  ✅ ГОТОВО!"
echo "════════════════════════════════════════════════════"
echo ""
echo "📋 Следующие шаги:"
echo ""
echo "1. Создайте репозиторий на GitHub/GitLab:"
echo "   • Зайдите на github.com или gitlab.com"
echo "   • Создайте новый репозиторий"
echo "   • Скопируйте URL репозитория"
echo ""
echo "2. Подключите удаленный репозиторий:"
echo "   git remote add origin https://github.com/username/repo.git"
echo ""
echo "3. Сделайте первый коммит:"
echo "   git commit -m 'Initial commit: STM32 Practice project'"
echo ""
echo "4. Отправьте на сервер:"
echo "   git push -u origin main"
echo ""
echo "5. На Windows:"
echo "   git clone https://github.com/username/repo.git"
echo ""

