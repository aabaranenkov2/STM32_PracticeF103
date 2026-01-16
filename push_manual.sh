#!/bin/bash

# Простой скрипт для ручного ввода токена

echo "════════════════════════════════════════════════════"
echo "  ОТПРАВКА КОДА НА GITHUB (РУЧНОЙ ВВОД)"
echo "════════════════════════════════════════════════════"
echo ""

echo "📋 Скопируйте ваш Personal Access Token"
echo "   (Создать: https://github.com/settings/tokens)"
echo ""
echo "📋 Затем выполните команды:"
echo ""
echo "1. Установите токен в переменную:"
echo "   export GITHUB_TOKEN='ваш_токен_здесь'"
echo ""
echo "2. Или выполните команду напрямую:"
echo "   git remote set-url origin https://ваш_токен@github.com/aabaranenkov2/STM32_PracticeF103.git"
echo "   git push -u origin main"
echo ""
echo "════════════════════════════════════════════════════"
echo "  ПРИМЕР"
echo "════════════════════════════════════════════════════"
echo ""
echo "Если ваш токен: ghp_abc123xyz456"
echo ""
echo "Выполните:"
echo "   git remote set-url origin https://ghp_abc123xyz456@github.com/aabaranenkov2/STM32_PracticeF103.git"
echo "   git push -u origin main"
echo ""

