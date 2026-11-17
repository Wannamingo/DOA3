# DOA3 Recompiled – Android alpha5

The launcher label is now **DOA3 Recompiled**. The Android package ID and
version upgrade path are unchanged.

**Stretch to screen** is available in the launcher and in-game aspect settings.
It fills the display with the complete image, so the proportions change when
the display and game aspects differ. The existing original, 4:3 and 16:9 modes
still fit the image with bars as needed.

**GPU driver** is available under launcher settings. The system driver is the
default. To try ANGLE, choose a ZIP containing a matched standalone Android
ARM64 pair (`libEGL_angle.so` and `libGLESv2_angle.so`) built for your device's
Android version and page size. Put both libraries at the ZIP root, under
`arm64-v8a/` or under `lib/arm64-v8a/`. This is an OpenGL ES renderer.
Turnip-only Vulkan ZIPs cannot be imported directly. No driver is included.
Driver selection takes effect when the game next starts. If loading the
libraries or creating their context fails, DOA3 retries with the system
driver. If a driver crashes on launch, reopen the launcher and choose System.

**Frame pacing.** Android no longer requests the extra blocking EGL VSync
on top of the existing 60 Hz Xbox guest pacing. On Android 11 and newer the
game surface also requests 60 FPS. This is a display scheduling hint; it
cannot override device policy or make the game render faster than the GPU and
CPU allow. To diagnose a lock that persists, compare `GLES swap interval` and
`GLES performance` (including `swap_ms`) in `files/logs/game.log` from
a fight at 1×. Report the device, Android version, selected driver and the
measured FPS. OnePlus Pad 2 and Snapdragon 870 require device validation.
