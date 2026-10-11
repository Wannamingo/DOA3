# Сборка / Building

## Локальные входные данные

Публичный репозиторий содержит ручной runtime и Android-часть, но не сгенерированный
игровой код. `tools/import_pc_payload.py` принимает **подготовленный PC-порт DOA3**,
с `private/program-*/generated/` и `private/imported-disc/disc/`.
Скрипт не генерирует C-код из произвольного ISO. Для генерации нужен собственный
образ и инструменты исходного проекта [NoRain211/doa3-re](https://github.com/NoRain211/doa3-re).
Поддерживается только снимок, совпадающий с `tools/game-recipe/usa-android.json`.

The public tree does not contain generated game code. The import script expects
a **prepared DOA3 PC port**, not an ISO. It authenticates the generated snapshot
and derives disc identities from the local game copy.

## Инструменты

Проверенная сборка выполнена на Windows с Python 3, CMake, Ninja, Android NDK r30,
JDK 21, Android API 35 / Build Tools 35, SDL3 Android 3.4.16, glslang 16.6.0
и SPIRV-Cross `aa217aeb6c9f0ace7a0ab233b28807edf45eb165`.
Имена каталогов используются сценариями сборки:

```text
toolchains/
  cmake/bin/cmake.exe
  ninja/ninja.exe
  android-ndk-r30/
  jdk-21.0.12.1+1/bin/
  sdk/android-35/android.jar
  sdk/android-15/             # build tools: aapt2, zipalign, lib/d8.jar, lib/apksigner.jar
  SDL3-android/aar/           # extracted official AAR: classes.jar and prefab/
  glslang-16.6.0/
  SPIRV-Cross-aa217aeb6c9f0ace7a0ab233b28807edf45eb165/
```

`sdk/android-15` — историческое имя каталога Android Build Tools, не уровень API.
Инструменты не входят в репозиторий и не загружаются сценариями автоматически.

## Команды

Из корня репозитория, подставив свои пути:

```powershell
python tools/import_pc_payload.py "C:/path/to/prepared-doa3-pc-port"
python tools/build_android.py --toolchains "C:/path/to/toolchains" --jobs 2
python tools/package_android.py --toolchains "C:/path/to/toolchains" --output "C:/path/to/output"
```

Упаковка проверяет квитанцию нативной сборки, JNI-экспорты, подпись и выравнивание
16 КиБ. Для локальной сборки создаётся собственный отладочный ключ под `private/`.
Он отличается от ключа опубликованного APK; побайтовая идентичность APK не обещается.
Сохраняйте свой ключ для дальнейших обновлений собственной сборки.

## Проверки

```powershell
python tools/verify_iso.py "C:/path/to/Dead or Alive 3 (USA) (En,Ja).iso" --toolchains "C:/path/to/toolchains"
python tools/build_android.py --toolchains "C:/path/to/toolchains" doa3_portable_simd_test doa3_fiber_context_test doa3_save_test doa3_sha_test doa3_shader_test doa3_gles_test
```

Исполняемые ARM64-тесты появляются в `private/build-arm64/` и запускаются на Android.
Проверка ISO использует тот же `XboxIso.java`, что APK, и ожидаемые SHA-256 из локального импорта.
Прохождение тестов библиотек не подтверждает прохождение всех режимов игры.
