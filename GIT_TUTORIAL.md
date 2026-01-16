# Git и GitHub для новичков - Пошаговая инструкция

## 🎯 Цель

Перенести проект `STM32_PracticeF103` на GitHub, чтобы работать с него с разных машин.

---

## 📋 Шаг 1: Создание репозитория на GitHub

### 1.1. Войдите в GitHub

1. Откройте https://github.com
2. Войдите в свой аккаунт

### 1.2. Создайте новый репозиторий

1. Нажмите кнопку **"+"** в правом верхнем углу
2. Выберите **"New repository"**

### 1.3. Заполните форму:

- **Repository name:** `STM32_PracticeF103` (или любое другое имя)
- **Description:** `STM32F103C8T6 practice project with Nextion display and DS3231 RTC`
- **Visibility:** 
  - ✅ **Public** (видно всем, но можно сделать приватным позже)
  - Или **Private** (только вы видите)
- **НЕ ставьте галочки:**
  - ❌ Add a README file
  - ❌ Add .gitignore
  - ❌ Choose a license

3. Нажмите **"Create repository"**

### 1.4. Скопируйте URL репозитория

После создания вы увидите страницу с инструкциями. **Скопируйте URL** - он будет выглядеть так:
```
https://github.com/ваш_username/STM32_PracticeF103.git
```

**Сохраните этот URL - он понадобится!**

---

## 📋 Шаг 2: Настройка Git на Linux

### 2.1. Установка Git (если еще не установлен)

```bash
sudo apt update
sudo apt install git
```

### 2.2. Настройка пользователя Git

```bash
# Замените на ваши данные
git config --global user.name "Ваше Имя"
git config --global user.email "aabaranenkov2@gmail.com"
```

**Важно:** Email должен совпадать с email на GitHub!

### 2.3. Проверка настройки

```bash
git config --global user.name
git config --global user.email
```

Должны показаться ваши имя и email.

---

## 📋 Шаг 3: Подготовка проекта

### 3.1. Перейдите в папку проекта

```bash
cd /media/alex60/INFO/Project-Bots/bots/Fitnes/ProjectsRST32/STM32_PracticeF103
```

### 3.2. Создайте .gitignore файл

Этот файл говорит Git, какие файлы НЕ нужно загружать:

```bash
cat > .gitignore << 'EOF'
# PlatformIO build files
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

# OS files
.DS_Store
Thumbs.db
*~

# Nextion Editor project files (большие)
*.HMI

# Wine
.wine/

# Временные файлы
*.tmp
*.bak
*.swp
EOF
```

### 3.3. Инициализация Git репозитория

```bash
git init
```

Вы увидите: `Initialized empty Git repository in ...`

---

## 📋 Шаг 4: Первый коммит (сохранение)

### 4.1. Добавьте все файлы

```bash
git add .
```

Эта команда добавляет все файлы в "промежуточную область" (staging area).

### 4.2. Проверьте, что будет загружено

```bash
git status
```

Вы увидите список файлов, которые будут загружены (зеленым).

### 4.3. Создайте первый коммит

```bash
git commit -m "Initial commit: STM32 Practice project with Nextion and DS3231"
```

**Что такое коммит?** Это "снимок" вашего проекта в определенный момент времени.

---

## 📋 Шаг 5: Подключение к GitHub

### 5.1. Добавьте удаленный репозиторий

```bash
# Замените URL на ваш (который скопировали в шаге 1.4)
git remote add origin https://github.com/ваш_username/STM32_PracticeF103.git
```

### 5.2. Проверьте подключение

```bash
git remote -v
```

Должно показать ваш URL дважды (для fetch и push).

### 5.3. Переименуйте ветку в main (если нужно)

```bash
git branch -M main
```

---

## 📋 Шаг 6: Загрузка на GitHub

### 6.1. Отправьте код на GitHub

```bash
git push -u origin main
```

**Что произойдет:**
- Git попросит ввести логин и пароль GitHub
- Или может открыться браузер для авторизации
- Код загрузится на GitHub

### 6.2. Если запросит пароль

GitHub больше не принимает обычные пароли. Нужно использовать **Personal Access Token**:

1. Зайдите на GitHub → Settings → Developer settings → Personal access tokens → Tokens (classic)
2. Нажмите "Generate new token"
3. Выберите срок действия (например, 90 дней)
4. Отметьте галочку **"repo"** (полный доступ к репозиториям)
5. Нажмите "Generate token"
6. **Скопируйте токен** (он показывается только один раз!)
7. Используйте этот токен вместо пароля при `git push`

---

## 📋 Шаг 7: Проверка

### 7.1. Обновите страницу на GitHub

Зайдите на страницу вашего репозитория:
```
https://github.com/ваш_username/STM32_PracticeF103
```

Вы должны увидеть все файлы проекта!

---

## 📋 Шаг 8: Работа с Git (основные команды)

### 8.1. После изменений на Linux:

```bash
# 1. Посмотреть, что изменилось
git status

# 2. Добавить измененные файлы
git add .

# 3. Создать коммит (сохранение)
git commit -m "Описание того, что изменили"

# 4. Отправить на GitHub
git push
```

### 8.2. Получение изменений с GitHub:

```bash
git pull
```

Эта команда скачивает все изменения, которые были сделаны на другой машине.

---

## 📋 Шаг 9: Настройка на Windows (ноутбук)

### 9.1. Установите Git for Windows

1. Скачайте: https://git-scm.com/download/win
2. Установите (все настройки по умолчанию - OK)

### 9.2. Настройте пользователя

Откройте **Git Bash** (появится после установки) и выполните:

```bash
git config --global user.name "Ваше Имя"
git config --global user.email "ваш_email@example.com"
```

### 9.3. Клонируйте репозиторий

```bash
# Перейдите в папку, где хотите сохранить проект
cd /c/Users/ВашеИмя/Documents

# Клонируйте репозиторий
git clone https://github.com/ваш_username/STM32_PracticeF103.git

# Перейдите в папку проекта
cd STM32_PracticeF103
```

### 9.4. Работайте с проектом

- Создайте TFT файл в Nextion Editor
- Сохраните в папку проекта

### 9.5. Загрузите изменения на GitHub

```bash
# Добавить файлы
git add .

# Создать коммит
git commit -m "Создан TFT файл для Nextion"

# Отправить на GitHub
git push
```

### 9.6. Получить изменения с Linux

```bash
git pull
```

---

## 📋 Шаг 10: Рабочий процесс (пример)

### День 1: Работа на Linux

```bash
# Изменили код
nano src/main.c

# Сохранили изменения
git add .
git commit -m "Добавлена функция для DS3231"
git push
```

### День 2: Работа на Windows

```bash
# Получили последние изменения
git pull

# Создали TFT файл в Nextion Editor
# Сохранили в папку проекта

# Загрузили на GitHub
git add *.tft
git commit -m "Создан интерфейс Nextion"
git push
```

### День 3: Снова на Linux

```bash
# Получили TFT файл с Windows
git pull

# Теперь TFT файл на Linux!
ls *.tft
```

---

## ⚠️ Частые ошибки и решения

### Ошибка: "fatal: not a git repository"

**Решение:**
```bash
cd /media/alex60/INFO/Project-Bots/bots/Fitnes/ProjectsRST32/STM32_PracticeF103
git init
```

### Ошибка: "fatal: remote origin already exists"

**Решение:**
```bash
# Удалить старый remote
git remote remove origin

# Добавить заново
git remote add origin https://github.com/ваш_username/STM32_PracticeF103.git
```

### Ошибка: "Authentication failed"

**Решение:**
- Используйте Personal Access Token вместо пароля
- Или настройте SSH ключи (более сложно)

### Ошибка: "Updates were rejected"

**Решение:**
```bash
# Сначала получите изменения
git pull

# Затем отправьте свои
git push
```

---

## 📚 Полезные команды Git

```bash
# Посмотреть историю коммитов
git log

# Посмотреть, что изменилось
git diff

# Отменить изменения в файле (до git add)
git checkout -- filename

# Отменить git add (но оставить изменения)
git reset HEAD filename

# Посмотреть все ветки
git branch

# Создать новую ветку
git branch новая_ветка

# Переключиться на ветку
git checkout новая_ветка
```

---

## ✅ Чек-лист

- [ ] Создан репозиторий на GitHub
- [ ] Скопирован URL репозитория
- [ ] Установлен Git на Linux
- [ ] Настроен пользователь Git
- [ ] Создан .gitignore
- [ ] Инициализирован Git репозиторий
- [ ] Сделан первый коммит
- [ ] Подключен удаленный репозиторий
- [ ] Код загружен на GitHub
- [ ] Проверено на GitHub (файлы видны)
- [ ] Установлен Git на Windows
- [ ] Репозиторий клонирован на Windows

---

## 🎉 Готово!

Теперь вы можете работать с проектом на обеих машинах!

**Главное правило:** Перед началом работы всегда делайте `git pull`, после работы - `git push`.

---

**Удачи в работе с Git!** 🚀

