# R-Zzen

<img src="assets/Zzen.png" width="112" alt="Zzen icon">

**Zzen v0.1** is a native C++ Windows desktop app for visual automation in Roblox **Army Commander Tycoon**. One session combines anti-AFK, construction purchases and automatic prestige, then resumes building after verifying the promotion count.

[Download v0.1](https://github.com/kazah-png/R-Zzen/releases/tag/v0.1) · [English user guide](output/pdf/Zzen-v0.1-User-Guide.pdf) · [Validation](VALIDACION.txt)

## Getting started

1. Download and extract `Zzen-0.1-Windows-x64.zip` from Releases.
2. Open the tycoon and set the **game language to Spanish**. The guide explains how; Zzen's own interface is in English.
3. Close older Zzen copies and launch `Zzen.exe`.
4. Click **Start session**, switch to Roblox and keep its toolbar visible. The tested resolution is **1920 × 1080**.
5. Watch the subtle status strip at the top of the screen. Press **F8** to stop before taking manual control.

Starting a session includes automatic promotion, which resets your base and in-game cash. Zzen pauses input when Roblox loses focus and resumes when you return. Keep the computer awake and unlocked.

## Features

- Opens construction with B when a purchase notification appears and buys the game's suggested available upgrade.
- Checks promotion requirements, cost and count, handles the reset confirmation, and verifies the new count before continuing.
- Sends a keep-active jump approximately every five minutes when the game view is recognised.
- Rechecks the current More icon before clicking, including day/night toolbar movement.
- Stops on uncertain promotion outcomes and preserves a pending record across restarts.
- Provides a click-through status overlay, **Export log** and an optional local diagnostic.

The app uses visible screen capture, Windows OCR and normal keyboard/mouse input. It does not inject code into Roblox. It does not automate combat, premium purchases or reconnection.

## Feedback

Stop with **F8**, click **Export log**, and share the generated `feedback/Zzen-feedback-*.txt` with a description of the problem. Review the text before posting it publicly. Nothing is uploaded automatically. Diagnostics and screenshots remain local and are separate from the text export.

Do not delete `promocion-pendiente.txt` while a promotion outcome is uncertain: Zzen uses it to verify the previous attempt.

## Build from source

Requirements: Windows x64, Visual Studio Build Tools with **Desktop development with C++**, a Windows SDK, PowerShell and Windows OCR support for the game's text.

```powershell
.\build.ps1
.\test.ps1
.\package.ps1
```

The application is built as `native/Zzen.exe`. Packaging creates `release/Zzen-0.1-Windows-x64.zip`. The prebuilt English PDF is included in the repository; regenerating it requires Python, ReportLab and the Windows Segoe UI fonts:

```powershell
python .\make_english_guide.py
```

The public build passes **65 logic and synthetic-image checks**, with 18 private fixture files explicitly skipped. Private gameplay screenshots are not distributed, so their OCR regression checks are explicitly reported as skipped. A developer with the complete local fixture set can require those tests with:

```powershell
.\native\tests.exe --require-fixtures
```

## Validation and limits

The original complete local suite passed **88 checks**. Live testing on 26 October 2026 observed promotion **11 → 12**, automatic counter verification, subsequent construction, and oil platform 1 reaching **level 2/3**. Testing involved debugging and restarts; a full uninterrupted cycle on the final binary and promotion 12 → 13 have not been verified.

The verified-upgrades counter can undercount purchases when the game closes the panel immediately. A closed panel alone is deliberately not treated as proof of an upgrade. UI counters restart with each session.

Version 0.1 targets the observed game layout and Spanish UI, including some observed English fallback text. Other resolutions and future game changes may need adjustments. This is an independent project, not affiliated with Roblox or the experience's developers.

## Source layout

| Path | Purpose |
| --- | --- |
| `cpp/core.hpp` | Recognition rules and automation state machine |
| `cpp/vision.hpp` | Image handling and Windows OCR |
| `cpp/main.cpp` | Native runtime, input checks and pending-promotion recovery |
| `cpp/ui.hpp` | English desktop interface and status overlay |
| `cpp/feedback.hpp` | Text feedback export |
| `cpp/tests.cpp` | Logic, synthetic-image and optional private-fixture regressions |
| `assets/` | Application icon and version resources |
