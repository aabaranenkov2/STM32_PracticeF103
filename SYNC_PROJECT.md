# Синхронизация проекта между Linux и Windows

## 🎯 Цель

Работать над проектом на двух машинах:
- **Linux** (основная) - разработка кода STM32
- **Windows** (ноутбук) - создание интерфейса Nextion

---

## ✅ Решение 1: Git (Рекомендуется)

### Преимущества:
- ✅ Версионирование кода
- ✅ История изменений
- ✅ Легкая синхронизация
- ✅ Работает на обеих системах
- ✅ Бесплатно (GitHub, GitLab)

### Настройка на Linux:

```bash
cd /media/alex60/INFO/Project-Bots/bots/Fitnes/ProjectsRST32/STM32_PracticeF103

# Инициализация Git (если еще не сделано)
git init

# Создать .gitignore
cat > .gitignore << 'EOF'
# Build files
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

# Nextion Editor files (опционально)
*.HMI
# Но оставляем .tft файлы для синхронизации
EOF

# Добавить файлы
git add .

# Первый коммит
git commit -m "Initial commit: STM32 Practice project with Nextion and DS3231"

# Создать репозиторий на GitHub/GitLab (или использовать локальный)
# Затем:
git remote add origin https://github.com/yourusername/stm32-practice.git
git push -u origin main
```

### Настройка на Windows:

```bash
# Установите Git for Windows: https://git-scm.com/download/win

# Клонируйте репозиторий
cd C:\Projects
git clone https://github.com/yourusername/stm32-practice.git
cd stm32-practice

# Или если уже есть локальный репозиторий, синхронизируйте через USB/сеть
```

### Работа с Git:

**На Linux (после изменений):**
```bash
git add .
git commit -m "Описание изменений"
git push
```

**На Windows (перед работой):**
```bash
git pull
# Работайте...
git add .
git commit -m "Создан TFT файл для Nextion"
git push
```

**На Linux (после работы на Windows):**
```bash
git pull
```

---

## ✅ Решение 2: Облачное хранилище

### Вариант A: GitHub (бесплатно, публичный репозиторий)

1. **Создайте аккаунт на GitHub**
2. **Создайте репозиторий**
3. **Используйте Git** (см. выше)

### Вариант B: GitLab (бесплатно, приватные репозитории)

1. **Создайте аккаунт на GitLab**
2. **Создайте приватный репозиторий**
3. **Используйте Git**

### Вариант C: OneDrive / Google Drive / Dropbox

**На Linux:**
```bash
# Установите клиент облачного хранилища
# Например, для OneDrive:
# sudo apt install onedrive

# Или используйте rclone:
sudo apt install rclone
rclone config  # Настройте подключение
```

**На Windows:**
- Установите клиент OneDrive/Google Drive/Dropbox
- Синхронизируйте папку проекта

**⚠️ Внимание:** Не рекомендуется для кода (нет версионирования), но удобно для файлов.

---

## ✅ Решение 3: Сетевая папка (Samba)

### Настройка на Linux:

```bash
# Установка Samba
sudo apt install samba samba-common

# Настройка
sudo nano /etc/samba/smb.conf

# Добавьте в конец файла:
[STM32_Project]
   path = /media/alex60/INFO/Project-Bots/bots/Fitnes/ProjectsRST32
   browseable = yes
   writable = yes
   guest ok = no
   valid users = alex60

# Создайте пользователя Samba
sudo smbpasswd -a alex60

# Перезапустите Samba
sudo systemctl restart smbd
```

### На Windows:

1. **Откройте Проводник**
2. **В адресной строке:** `\\IP_АДРЕС_LINUX\STM32_Project`
3. **Введите логин/пароль** (alex60)
4. **Создайте ярлык** для быстрого доступа

---

## ✅ Решение 4: USB / Внешний диск

### Простой способ:

1. **Скопируйте проект на USB/внешний диск**
2. **Работайте на Windows**
3. **Скопируйте обратно на Linux**

### Автоматизация через скрипт:

**На Linux:**
```bash
# Создайте скрипт для синхронизации
cat > sync_to_usb.sh << 'EOF'
#!/bin/bash
USB_MOUNT="/media/alex60/USB"
PROJECT_DIR="/media/alex60/INFO/Project-Bots/bots/Fitnes/ProjectsRST32/STM32_PracticeF103"

if [ -d "$USB_MOUNT" ]; then
    rsync -av --delete "$PROJECT_DIR/" "$USB_MOUNT/STM32_PracticeF103/"
    echo "✅ Синхронизация завершена"
else
    echo "❌ USB не найден"
fi
EOF

chmod +x sync_to_usb.sh
```

**На Windows:**
- Используйте аналогичный скрипт или просто копируйте файлы

---

## ✅ Решение 5: rsync через SSH

### Настройка SSH на Linux:

```bash
# Установка SSH сервера
sudo apt install openssh-server
sudo systemctl enable ssh
sudo systemctl start ssh

# Узнайте IP адрес
ip addr show | grep "inet "
```

### На Windows:

1. **Установите WinSCP** или **FileZilla**
2. **Подключитесь по SSH:**
   - Хост: IP адрес Linux машины
   - Порт: 22
   - Пользователь: alex60
   - Пароль: ваш пароль

3. **Синхронизируйте папку проекта**

---

## 📋 Рекомендуемый рабочий процесс

### Вариант A: Git (лучший для кода)

```
Linux: Разработка кода → git commit → git push
Windows: git pull → Создание TFT → git commit → git push
Linux: git pull → Получение TFT файла
```

### Вариант B: Облачное хранилище (проще для файлов)

```
Linux: Работа → Загрузка в облако
Windows: Скачивание → Работа → Загрузка в облако
Linux: Скачивание → Продолжение работы
```

### Вариант C: USB (самый простой)

```
Linux: Копирование на USB
Windows: Работа с USB
Linux: Копирование обратно
```

---

## 🎯 Моя рекомендация

**Для вашего случая:**

1. **Используйте Git + GitHub/GitLab** для кода
   - Версионирование
   - История изменений
   - Легкая синхронизация

2. **Используйте USB/облако** для TFT файлов
   - TFT файлы большие
   - Не нужна версия для каждого изменения
   - Просто копировать

3. **Создайте структуру:**
   ```
   STM32_PracticeF103/
   ├── src/              (код - в Git)
   ├── include/          (код - в Git)
   ├── platformio.ini    (код - в Git)
   ├── *.tft             (TFT файлы - в облако/USB)
   └── *.HMI             (проекты Nextion - в облако/USB)
   ```

---

## 🔧 Быстрая настройка Git

```bash
cd /media/alex60/INFO/Project-Bots/bots/Fitnes/ProjectsRST32/STM32_PracticeF103

# Создайте .gitignore
cat > .gitignore << 'EOF'
.pio/
*.log
*.HMI
wine_errors.log
nextion_debug.log
EOF

# Инициализация
git init
git add .
git commit -m "Initial commit"

# Создайте репозиторий на GitHub и:
# git remote add origin https://github.com/yourusername/repo.git
# git push -u origin main
```

---

## ✅ Чек-лист

- [ ] Выбрать метод синхронизации (Git/Облако/USB)
- [ ] Настроить на Linux
- [ ] Настроить на Windows
- [ ] Протестировать синхронизацию
- [ ] Создать рабочий процесс

---

**Готово! Теперь можно работать на обеих машинах!** 🎉

