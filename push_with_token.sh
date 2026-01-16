#!/bin/bash

# Скрипт для отправки кода с токеном

echo "════════════════════════════════════════════════════"
echo "  ОТПРАВКА КОДА НА GITHUB С ТОКЕНОМ"
echo "════════════════════════════════════════════════════"
echo ""

# Проверка, есть ли токен в переменной окружения
if [ -z "$GITHUB_TOKEN" ]; then
    echo "📋 Введите ваш Personal Access Token:"
    echo "   (Токен можно создать здесь: https://github.com/settings/tokens)"
    echo "   (Токен начинается с ghp_...)"
    echo ""
    echo -n "Token: "
    read GITHUB_TOKEN
    echo ""
    
    if [ -z "$GITHUB_TOKEN" ]; then
        echo "❌ Токен не введен!"
        exit 1
    fi
fi

if [ -z "$GITHUB_TOKEN" ]; then
    echo "❌ Токен не введен!"
    exit 1
fi

# Изменение URL с токеном
echo "📋 Настройка URL репозитория с токеном..."
git remote set-url origin https://${GITHUB_TOKEN}@github.com/aabaranenkov2/STM32_PracticeF103.git

if [ $? -eq 0 ]; then
    echo "   ✅ URL настроен"
else
    echo "   ❌ Ошибка настройки URL"
    exit 1
fi

# Отправка кода
echo ""
echo "📋 Отправка кода на GitHub..."
git push -u origin main

if [ $? -eq 0 ]; then
    echo ""
    echo "════════════════════════════════════════════════════"
    echo "  ✅ УСПЕШНО ОТПРАВЛЕНО!"
    echo "════════════════════════════════════════════════════"
    echo ""
    echo "📋 Проверьте на GitHub:"
    echo "   https://github.com/aabaranenkov2/STM32_PracticeF103"
    echo ""
    echo "⚠️  ВАЖНО: Токен сохранен в URL репозитория!"
    echo "   Для безопасности можно удалить токен из URL после отправки:"
    echo "   git remote set-url origin https://github.com/aabaranenkov2/STM32_PracticeF103.git"
    echo ""
else
    echo ""
    echo "❌ Ошибка отправки!"
    echo "   Проверьте правильность токена"
    exit 1
fi

