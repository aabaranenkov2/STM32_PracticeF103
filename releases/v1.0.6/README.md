# LotSensor 1.0.6

Проверенная прошивка для `lot_sensor_f103c8_clone`:

- `LotSensor-f103c8-v1.0.6.bin`
- `LotSensor-f103c8-v1.0.6.bin.sha256`
- `LotSensorSetup-v1.0.6.exe.sha256`
- `settings.json` — снимок настроек контроллера и геометрии на момент выпуска.

Прошивка через ST-Link из PowerShell в каталоге проекта:

```powershell
pio run -e lot_sensor_f103c8_clone -t upload
```

Копирование выпуска на флешку в Linux:

```bash
FLASH="/media/alex60/ИМЯ_ФЛЕШКИ"
cp -v dist/LotSensorSetup-v1.0.6.exe "$FLASH/"
cp -v releases/v1.0.6/LotSensor-f103c8-v1.0.6.bin "$FLASH/"
cp -v releases/v1.0.6/LotSensor-f103c8-v1.0.6.bin.sha256 "$FLASH/"
cp -v releases/v1.0.6/LotSensorSetup-v1.0.6.exe.sha256 "$FLASH/"
cp -v releases/v1.0.6/settings.json "$FLASH/"
sync
```

Контроль расхождения Enc/Mot_FG в сохранённых настройках выключен
(`mismatch_mm = 0`). Включать его следует только при установленной нити
на основном Enc.
