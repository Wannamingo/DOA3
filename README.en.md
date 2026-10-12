<p align="center">
  <img src="docs/images/logo.svg" alt="Dead or Alive 3" width="400">
</p>

<h1 align="center">DOA3</h1>

<p align="center"><b>Dead or Alive 3 · Android · ARM64</b></p>

<p align="center">
  <img src="docs/images/badges/android.svg" alt="Android 8.0+">
  <img src="docs/images/badges/abi.svg" alt="ARM64">
  <img src="docs/images/badges/version.svg" alt="APK 0.1.0 alpha4">
  <img src="docs/images/badges/resolution.svg" alt="1× / 2× / 3×">
</p>

<p align="center"><a href="README.md">Русский</a> · <b>English</b></p>

<p align="center">
  <a href="https://github.com/Wannamingo/DOA3/releases/download/v0.1.0-alpha4/DOA3.apk"><b>Download APK</b></a> ·
  <a href="https://github.com/Wannamingo/DOA3/releases">All releases</a> ·
  <a href="https://github.com/Wannamingo/DOA3/issues/new/choose">Report a bug</a>
</p>

Unofficial native Android port of **Dead or Alive 3**. The game's x86 code is statically recompiled for ARM64; graphics use OpenGL ES 3, while audio and input use SDL3. The launcher follows [DOAXBV](https://github.com/Wannamingo/DoAXBV), with gamepad support, touch controls and direct reading from your own USA ISO.

**Alpha build:** startup, menus and a fight have been tested on RG477M. Full completion, every character, arena and effect have not been tested; some renderer commands can still be declined. Performance varies by scene and device.

Your own original USA game copy is required. ISO images, XBE files and game-data archives are not distributed in this repository or its releases.

## Version

| Component | Value |
| --- | --- |
| APK | `0.1.0-alpha4` |
| Android package | `org.doa3.port` |
| Native core | DOA3 USA, ARM64 |
| Tested device | RG477M, Android 14 / GammaOS, Mali-G615 MC6 |

## Features

- All settings are available in-game: tap ⚙ beside FPS or use Select → Settings. Resolution, aspect ratio and controls change without restarting the fight.
- Russian and English launcher with DOA3 artwork and app icon.
- Read game files directly from an ISO without fully extracting or copying resources. ZIP loading is unsupported.
- Verify sizes and SHA-256 of all 24 files on first selection; retain access to the selected ISO.
- Transparent outline touch controls: D-pad, two sticks, colored face buttons and compact shoulder keys.
- Separate touch and gamepad button assignments, with persistent layouts and reset.
- Left stick duplicates all eight D-pad directions.
- In-game layout editor: drag controls while the game is paused, then save, cancel or reset.
- Automatic touch-control hiding when a gamepad is connected.
- Original 4:3 proportions or 16:9 display.
- **1×, 2× and 3×** internal resolution settings.
- A/B and X/Y swapping, and a menu for returning to the launcher.

| Mode | Internal resolution |
| --- | --- |
| 1× | 720 × 480 |
| 2× | 1440 × 960 |
| 3× | 2160 × 1440 |

The original 720 × 480 buffer is displayed at 4:3. Raising internal resolution preserves the selected display aspect ratio.

## Screenshots

**Test fight on RG477M**

![DOA3 fight on RG477M reading an ISO](docs/images/screenshots/gameplay.png)

| Launcher | Settings |
| --- | --- |
| ![DOA3 launcher on RG477M](docs/images/screenshots/launcher.png) | ![Display, resolution and control settings](docs/images/screenshots/settings.png) |

| Button assignments | Layout editor |
| --- | --- |
| ![Separate touch and gamepad layouts](docs/images/screenshots/mapping.png) | ![Drag touch controls during the game](docs/images/screenshots/layout-editor.png) |

**In-game settings**

![Change settings without leaving the fight](docs/images/screenshots/game-settings.png)

## Requirements

| Item | Requirement |
| --- | --- |
| OS | Android 8.0 or newer, API 26+ |
| Architecture | ARM64 / `arm64-v8a` |
| Graphics | OpenGL ES 3.0, as declared by the APK manifest |
| Game | Original USA version of Dead or Alive 3 |
| Storage | ISO on the device plus space for saves and game-managed cache |
| APK size | 56,260,191 bytes, approximately 53.7 MiB |

The 4,191,140,992 bytes of disc resources are read from the ISO. The app stores a small extent table and image reference. Saves and the game-managed cache remain separate.

## Installation

1. Download **DOA3.apk** from the [alpha4 release](https://github.com/Wannamingo/DOA3/releases/tag/v0.1.0-alpha4) and install it.
2. Copy your own **Dead or Alive 3 (USA) (En,Ja).iso** to the device. The tested full Redump ISO is **7,825,162,240 bytes**. Use internal storage or exFAT for a file of this size.
3. Open DOA3 and select **Select ISO**.
4. Select the ISO and wait for initial verification. It reads the image without copying resources.
5. Select **PLAY**. Keep the ISO available on the device; reselect it if it is moved or deleted.

No extraction is required. Only the verified USA files are supported. Direct reading needs a local, seekable image in internal storage or on an SD card. See [ISO setup](docs/iso.md).

APK updates preserve settings and saves. When upgrading from alpha1–alpha3, select an ISO once. The old extracted copy is not automatically deleted and is not used by the new mode. Uninstalling the app deletes local saves.

## Controls and settings

**Start** pauses the game. **Select** opens the app menu for returning to the launcher. **L1 → Black**, **R1 → White**, **L2/R2 → Xbox triggers**. Enable A/B and X/Y swapping if button labels differ.

**Settings → Button assignments → Configure** opens separate touch and gamepad layouts. To move controls, tap the small pencil button next to FPS during the game. Gameplay pauses while editing; tap Save to continue. See [controls and layout editing](docs/controls.md).

Settings include aspect ratio, internal resolution, touch controls and app language. Launcher changes apply on game launch. In-game changes apply when returning to the current fight. RU/EN changes the app interface, not the game's text.

## Validation and source

Direct ISO reads, 24 SHA-256 checks, process restart, menus, fight and pause were verified on RG477M. The ARM64 ISO test covers offsets beyond 4 GiB, random reads, EOF, independent cursors and read-only enforcement. Previous fight, audio, input and resolution checks remain in the [validation report](docs/validation.md).

The repository includes the hand-written runtime, Android backend, launcher, ISO verification and connection, tests and build scripts. Generated game code, XBE, disc resources and signing keys are excluded. A full build requires a locally prepared DOA3 PC port and external tools; **a clean clone is not a standalone build kit**. See [source status](docs/source-status.md) and [building instructions](docs/building.md).

## Issues and contributions

[Open an issue](https://github.com/Wannamingo/DOA3/issues/new/choose) with the APK version, device, Android version, resolution mode and reproduction steps. Public-source changes can be proposed via pull request; see [CONTRIBUTING.md](CONTRIBUTING.md).

## Credits and licenses

The native runtime is based on [NoRain211/doa3-re](https://github.com/NoRain211/doa3-re). The Android backend is adapted from [DOAXBV](https://github.com/Wannamingo/DoAXBV), based on [NoRain211/doaxvb-re](https://github.com/NoRain211/doaxvb-re). [GNU GPL v3](LICENSE), original notices and [third-party licenses](THIRD_PARTY_NOTICES.md) are retained.

This is an unofficial project and is not affiliated with the Dead or Alive rights holders. Original names, characters and artwork belong to their respective owners; the runtime license does not grant rights to those materials.
