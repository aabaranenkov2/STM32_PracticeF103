# Установка Nextion Editor на Linux

## 📋 Скачивание

### Официальный сайт:
- **Главная страница:** https://nextion.tech/nextion-editor/
- **Страница загрузок:** https://nextion.tech/downloads/

### Для Linux обычно доступны:
- **.tar.gz** архив (рекомендуется)
- **.AppImage** (портативная версия, не требует установки)
- **.deb** пакет (для Debian/Ubuntu)

---

## 🔍 Проверка скачанного файла

Если вы скачали `nextion-setup-vLTS.zip`, это скорее всего версия для Windows.

### Проверка содержимого:
```bash
# Посмотреть содержимое архива
unzip -l nextion-setup-vLTS.zip

# Если внутри есть .exe файлы - это Windows версия
```

---

## ✅ Правильная установка для Linux

### Вариант 1: .tar.gz архив (рекомендуется)

1. **Скачайте Linux версию:**
   ```bash
   # Перейдите на страницу загрузок
   # Выберите "Linux" версию
   # Обычно это файл типа: nextion-editor-vX.X.X-linux.tar.gz
   ```

2. **Распакуйте:**
   ```bash
   cd ~/Downloads  # или где вы скачали файл
   tar -xzf nextion-editor-*.tar.gz
   ```

3. **Запустите:**
   ```bash
   cd nextion-editor-*/
   ./nextion-editor
   ```

4. **Создайте ярлык (опционально):**
   ```bash
   # Создать .desktop файл
   cat > ~/.local/share/applications/nextion-editor.desktop << 'DESKTOP'
   [Desktop Entry]
   Name=Nextion Editor
   Exec=/path/to/nextion-editor/nextion-editor
   Icon=/path/to/nextion-editor/icon.png
   Type=Application
   Categories=Development;
   DESKTOP
   ```

### Вариант 2: .AppImage (портативная версия)

1. **Скачайте .AppImage файл**

2. **Сделайте исполняемым:**
   ```bash
   chmod +x nextion-editor-*.AppImage
   ```

3. **Запустите:**
   ```bash
   ./nextion-editor-*.AppImage
   ```

4. **Создайте ярлык:**
   ```bash
   # Переместите в удобное место
   mkdir -p ~/Applications
   mv nextion-editor-*.AppImage ~/Applications/
   
   # Создайте .desktop файл
   cat > ~/.local/share/applications/nextion-editor.desktop << 'DESKTOP'
   [Desktop Entry]
   Name=Nextion Editor
   Exec=/home/alex60/Applications/nextion-editor-*.AppImage
   Type=Application
   Categories=Development;
   DESKTOP
   ```

### Вариант 3: .deb пакет (для Ubuntu/Debian)

1. **Скачайте .deb файл**

2. **Установите:**
   ```bash
   sudo dpkg -i nextion-editor-*.deb
   # Если есть зависимости:
   sudo apt-get install -f
   ```

3. **Запустите:**
   ```bash
   nextion-editor
   # Или через меню приложений
   ```

---

## 🔧 Альтернатива: Использование через Wine (если нет Linux версии)

Если доступна только Windows версия, можно использовать Wine:

### Установка Wine:
```bash
sudo apt update
sudo apt install wine
```

### Установка Nextion Editor через Wine:
```bash
# Распакуйте zip
unzip nextion-setup-vLTS.zip

# Запустите установщик через Wine
wine setup.exe  # или как называется установщик
```

**⚠️ Внимание:** Wine может работать нестабильно. Лучше найти Linux версию.

---

## 📋 Проверка установки

После установки проверьте:

```bash
# Проверка, что программа запускается
which nextion-editor

# Или если в текущей директории:
./nextion-editor --version
```

---

## 🚀 Быстрый старт

1. **Запустите Nextion Editor**

2. **Создайте новый проект:**
   - File → New → New Project
   - Выберите модель: **NX4832T035_011**
   - Сохраните как `fitness_clock.HMI`

3. **Создайте интерфейс:**
   - См. `NEXTION_INTERFACE.md` для деталей

4. **Скомпилируйте:**
   - File → Build → Compile
   - Создастся файл `fitness_clock.tft`

5. **Скопируйте на microSD:**
   ```bash
   ./copy_tft_to_sd.sh /path/to/fitness_clock.tft
   ```

---

## ❓ Проблемы и решения

### Проблема: Нет Linux версии на сайте

**Решение:**
1. Проверьте, что вы на правильной странице загрузок
2. Попробуйте использовать .AppImage (обычно доступен)
3. Используйте Wine для Windows версии (не рекомендуется)

### Проблема: Программа не запускается

**Решение:**
```bash
# Проверьте права на выполнение
chmod +x nextion-editor

# Проверьте зависимости
ldd nextion-editor  # покажет недостающие библиотеки

# Установите недостающие библиотеки
sudo apt-get install libqt5core5a libqt5gui5 libqt5widgets5
```

### Проблема: Нет иконки в меню

**Решение:**
- Создайте .desktop файл (см. выше)
- Обновите меню: `update-desktop-database ~/.local/share/applications/`

---

## 📚 Дополнительные ресурсы

- **Официальный сайт:** https://nextion.tech/
- **Документация:** https://nextion.tech/instruction-set/
- **Форум:** https://nextion.tech/forums/

---

## ✅ Чек-лист установки

- [ ] Скачана правильная версия для Linux (.tar.gz или .AppImage)
- [ ] Файл распакован
- [ ] Права на выполнение установлены (`chmod +x`)
- [ ] Программа запускается
- [ ] Создан ярлык (опционально)

---

**Готово! Nextion Editor установлен!** 🎉

