#!/bin/bash

# Скрипт для настройки аутентификации GitHub

echo "════════════════════════════════════════════════════"
echo "  НАСТРОЙКА АУТЕНТИФИКАЦИИ GITHUB"
echo "════════════════════════════════════════════════════"
echo ""

echo "📋 Вариант 1: Настроить credential helper (рекомендуется)"
echo ""
echo "Это сохранит ваш токен, чтобы не вводить каждый раз."
echo ""

# Настройка credential helper
git config --global credential.helper store

echo "✅ Credential helper настроен"
echo ""

echo "📋 Теперь выполните:"
echo "   git push -u origin main"
echo ""
echo "Когда запросит:"
echo "   Username: aabaranenkov2"
echo "   Password: ВСТАВЬТЕ ВАШ PERSONAL ACCESS TOKEN"
echo ""
echo "Токен будет сохранен и больше не нужно будет вводить!"
echo ""

echo "════════════════════════════════════════════════════"
echo "  КАК СОЗДАТЬ TOKEN"
echo "════════════════════════════════════════════════════"
echo ""
echo "1. Зайдите: https://github.com/settings/tokens"
echo "2. Generate new token → Generate new token (classic)"
echo "3. Имя: STM32_Project"
echo "4. Срок: 90 дней (или больше)"
echo "5. Отметьте: repo (полный доступ)"
echo "6. Generate token"
echo "7. СКОПИРУЙТЕ ТОКЕН (показывается только раз!)"
echo ""

