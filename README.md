# InsideOut

Dynamischer Sidechain-EQ von **Boy made of Soy** · Philip Jin Plugins.

InsideOut sitzt auf einer Spur oder dem Mixbus und senkt genau die Frequenzen ab, in denen die Vocals (Sidechain) gerade spielen – nur so lange, wie sie spielen. Bässe und Höhen lassen sich mit Low/High Protect komplett schützen.

## Installieren (macOS, Logic)

1. Unter **Actions** den letzten Build öffnen und das Artefakt `InsideOut-macOS` laden (oder bei einem Release die Zips).
2. `InsideOut-AU.zip` entpacken und `InsideOut.component` nach `~/Library/Audio/Plug-Ins/Components/` kopieren.
3. Im Terminal einmal die Quarantäne entfernen:
   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/InsideOut.component
   ```
4. Logic starten. Falls das Plugin fehlt: *Logic Pro › Einstellungen › Plug-in-Manager › Zurücksetzen & neu scannen*.

## In Logic benutzen

1. InsideOut als Insert auf den Gitarrenbus, die Keys oder den Mixbus legen (Hersteller: *Philip Jin Plugins*).
2. Oben rechts im Plugin-Fenster unter **Side Chain** die Vocal-Spur auswählen.
3. Modus wählen (Mix Bus, Guitar, Piano, Synth, Universal) oder ein Preset laden, dann mit **Amount** dosieren.

Die grüne gestrichelte Linie zeigt die Vocals, die dunkle Fläche von oben zeigt, was gerade abgesenkt wird. Die Protect-Grenzen kannst du direkt im Spektrum ziehen (Doppelklick setzt sie zurück).

## Regler

| Regler | Funktion |
|---|---|
| Amount | Wie stark abgesenkt wird (die Tiefe hängt vom Modus ab: Mix Bus max. 6 dB, Piano 10, Guitar 15, Synth 20, Universal 24) |
| Attack / Release | Wie schnell die Absenkung kommt und wieder geht |
| SC Low Cut / SC High Cut | Teile der Stimme, die das Ducking nicht auslösen sollen |
| Low Protect / High Protect | Bereiche des Mixes, die nie angefasst werden („Aus“ am Anschlag) |
| Dry/Wet, Output | Parallelmischung und Ausgangspegel |
| Stereo / Mid / Side | Wo abgesenkt wird – Mid lässt die Stereobreite stehen |
| 16 / 32 / 64 | Frequenzauflösung |
| Listen | Nur hören, was entfernt wird |
| Bypass | Umgehen, Latenz bleibt gleich |

Wählst du im Plugin einen Modus, setzt er passende Werte für Low/High Protect und Stereo-Modus.

## Technik

- JUCE 8, CMake, Formate AU + VST3 (+ Standalone)
- STFT 2048 Samples, 75 % Overlap, sqrt-Hann; Latenz 2048 Samples (wird an Logic gemeldet)
- Verarbeitung im Mid/Side-Raum, Ducking pro Band mit Attack/Release und Glättung über Nachbarbänder

## Bauen

Automatisch per GitHub Actions (`.github/workflows/build.yml`) bei jedem Push auf `main`; ein Tag `v1.0.0` erzeugt zusätzlich ein Release.

Lokal (macOS):
```
cmake -S . -B build -G Xcode
cmake --build build --config Release --target InsideOut_AU
```

DSP-Tests (ohne Host):
```
cmake -S . -B build -DINSIDEOUT_BUILD_TESTS=ON
cmake --build build --target InsideOutDspTest
./build/InsideOutDspTest_artefacts/Release/InsideOutDspTest
```

## Lizenzen

Schriften: UnifrakturCook, DM Sans, DM Mono – SIL Open Font License (`Resources/Fonts/OFL.txt`).
