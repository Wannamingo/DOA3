# Состав исходников / Source status

Публичный снимок содержит:

- `recomp-runtime/` — ручной runtime DOA3, адаптеры Xbox API и тесты.
- `android/native/` — ARM64 fiber, GLES, SDL3, JNI, ввод и звуковой вывод.
- `android/java/` — лаунчер, экранное управление, ISO/ZIP импорт с проверкой файлов.
- `android/assets/launcher/`, `android/res/` — интерфейс, фон и иконка.
- `xbe/` — парсер XBE и SHA.
- `tools/` — импорт приватных входных данных, аутентифицированная сборка и упаковка.

В `private/` локально находятся сгенерированный код, идентичности ресурсов,
библиотеки и ключ подписи. Этот каталог исключён из Git и исходного архива релиза.
ISO, XBE, дисковые ресурсы и архив игровых данных не публикуются.

Это не автономная сборка из чистого клона: необходимы собственная USA-копия игры,
подготовленный снимок PC-порта и внешние инструменты. Подробности — [building.md](building.md).
Рецепт фиксирует SHA-256 XBE, файлов сгенерированного кода и манифеста.
Отсутствующие игровые функции завершают выполнение с диагностикой; их наличие
ограничивает непроверенные пути выполнения alpha-сборки.

Скриншоты лаунчера, настроек, назначений кнопок, редактора и боя сняты на RG477M
при проверках alpha2 и alpha3. Показания FPS на отдельных кадрах не являются бенчмарком.

The public source tree excludes generated game code, disc contents and signing keys.
It is not a self-contained clean-clone build. The APK's release checksum identifies
the tested binary; it does not imply a reproducible byte-identical APK from public
files alone. Launcher, settings, button assignments, layout editor and gameplay
captures are from alpha2/alpha3 validation on RG477M. Individual FPS readings are not benchmarks.
