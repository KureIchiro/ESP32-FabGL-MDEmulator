# SEGA MEGA DRIVE / GENESIS Emulator for ESP32 (TTGO VGA32)

A Mega Drive / Genesis emulator for the LILYGO TTGO VGA32 (ESP32 + FabGL), built on the Gwenesis core by bzhxx.
It uses the same setup as the other emulators in this collection: ROM list at start-up, PS/2 keyboard, the joystick adapter on Serial1, F12 pause.

> ⚠️ **Notice Regarding Development & Licenses**
>
> Most of the main source code is built and modified based on AI-generated code. Therefore, the developer does not completely grasp all internal logic and cannot answer detailed questions regarding source code specifications. Thank you for your understanding.
> Please refer to the header comments at the beginning of the source code for license details and agree to them before use.

---

## Credits & License

- **Emulation core**: Gwenesis by bzhxx (https://github.com/bzhxx/gwenesis; its source files are GPLv3 or later), as included in Retro-Gamer (GPLv3). Modified as listed in the header of `MegaDrive.ino` (changes dated 2026-11-12 to 2026-11-26).
- **68000 CPU core**: Musashi by Karl Stenerud (MIT License, https://github.com/kstenerud/Musashi), configured and generated for the 68000 and modified as listed in the header of `MegaDrive.ino` (2026-11-20 to 2026-11-26: tables made on a PC, two-level dispatch, handlers that return their clocks).
- **Z80 CPU core**: z80core, written for this collection from the Z80 documentation (MIT License, Copyright Kure Ichiro; `z80core.c` / `z80core.h`).
- **FM sound**: the YM2203/OPNA FM code of the PC-8801 emulator in this collection, used as the YM2612.
- **Graphics/audio engine**: FabGL by Fabrizio Di Vittorio (GPLv3).

Like Gwenesis and FabGL, this sketch as a whole is distributed under the GPLv3 (`LICENSE`), with its full source. The Musashi files and z80core keep their MIT license notice.

---

## 🛠️ Environment & Build Settings

- **Board Manager**: ESP32 by Espressif Systems (**v2.0.5 Recommended**)
- **Library**: FabGL v1.0.9, **with one file replaced** (required): copy `fabgl_patch/displaycontroller.cpp` over `FabGL/src/displaycontroller.cpp` in your Arduino libraries folder. The stock FabGL keeps its drawing routines in the ESP32's internal instruction RAM (IRAM), and with them this sketch does not fit (the link fails with "iram0_0_seg overflowed"). The patched file only keeps those routines in flash instead (about 8 KB of IRAM freed); nothing else changes, and other FabGL sketches keep working. It is the same file as in the MZ-2500 sketch and stays under FabGL's GPL v3 license (see `fabgl_patch/LICENSE`).
- **Board**: `ESP32 Dev Module` (TTGO VGA32)
- **PSRAM** (ON = `"Enabled"`, OFF = `"Disabled"`): With PSRAM ON, a software workaround for the chip errata (Espressif errata CPU-3.9) is built in, and the speed drops by about 20%. With PSRAM OFF, the sketch enables PSRAM by itself and the software workaround is not built in. However, this causes failures unless the ESP32 is chip revision 3 or later, so on any other revision a PSRAM OFF build is made not to start. This emulator is heavy, so an ESP32 of revision 3 or later with PSRAM OFF is recommended; with PSRAM ON it runs, but slower.
- **Partition Scheme**: the default is fine (the program is about 1 MB).
- **SD card pins** (`SD_SCLK` / `SD_MISO` / `SD_MOSI` / `SD_CS` near the top of the `.ino`): they differ by board. The default is TTGO VGA32 (14 / 2 / 12 / 13). ORANGE ESPer: 14 / 35 / 12 / 13. Switch Science FabGL development board: 14 / 35 / 2 / 13. The FabGL development board wires the SD card to MISO = 16 and MOSI = 17, but on an ESP32-WROVER these two pins are not connected (they are used inside the module for the PSRAM), so jumper 16 → 35 and 17 → 2 on the FabGL board. Enable the block for your board and comment out the others.
- **Switching machines with EmuLauncher** (optional): copy `emu_switch.h` and `partitions.csv` from the `EmuLauncher` folder into this sketch folder. The ROM list then shows the machine to start next at the bottom (TAB or controller button 3 changes it), and the flash is laid out as in `partitions.csv` (the Partition Scheme setting is not used). Without the two files the sketch works on its own as before. See `EmuLauncher/README.md`.

---

## 📂 SD Card

```text
/MD/ROM/   .md / .gen / .bin / .smd  (sub-folders are fine)
```

- **ROM size**: the ROM is loaded into PSRAM (4 MB), which is shared with the other buffers, so the limit is the free PSRAM, up to 28 Mbit (3.5 MB). 32 Mbit (4 MB) ROMs do not fit ("ROM LOAD ERROR").

---

## 🎮 Controls

| | Keyboard | Controller (Serial1) |
|---|---|---|
| D-pad | Arrow keys | D-pad |
| A / B / C | Z / X / C | Button 1 / 2 / 3 |
| START | Enter | Button 4 |

- **Controller**: player 1 only, 4 buttons. The adapter on Serial1 sends one byte at a time as on the other machines (0 = pressed; bit 0–3 = up/down/left/right, bit 4–6 = buttons 1–3), but bit 7 is button 4 here (the other machines use bit 7 to tell player 1 from player 2), so the adapter must be in its 1-controller 4-button mode.
- **F12**: pause / resume. **Shift + F12**: reset (restarts the ESP32 and goes back to the ROM list).
- **F9 / F10 / F11**: load state slot 2 / 1 / 0. **Shift + F9 / F10 / F11**: save the state to that slot (`/MD/SAVE/<ROM name>_<slot>.sta` on the SD card). The sound restarts from the next note after a load.
- Battery backup RAM (when the ROM header declares it, past the end of the ROM): saved automatically to `/MD/SAVE/<ROM name>.srm` 2 seconds after the game writes it, and loaded when the game starts. Not supported: the switched backup RAM of ROMs over 2 MB, and EEPROM saves.
- **Shift + F8**: removes / restores the horizontal sprite limit (20 per line). Removing it reduces flicker in some games; heavy scenes may run slower.
- **ROM list**: Up/Down = select, PageUp/PageDown = page, Enter = start / open the folder, BackSpace or ESC = folder up. With the controller: up/down = select, left/right = page (hold left/right briefly while up/down is not pressed), button 1 or 4 = Enter, button 2 = folder up. TAB (controller button 3) = machine to start next (only with EmuLauncher).
- **Button layout**: each Enter on the top row of the ROM list (BUTTON 1 2 3 = …) switches buttons 1–3 (keyboard Z / X / C) between A B C and C B A. It is saved to `/MD/PAD.CFG` on the SD card. START does not change.

---

## How it runs

- **Picture**: each VDP line is written straight into FabGL's VGA line buffer (320×240, double-scanned to 640×480 at 59.94 Hz); there is no frame buffer and no copy. Each frame starts at the VGA vertical blank, so the picture does not tear. When a frame starts late, its drawing is skipped (at most two frames in a row). The lines are drawn on the other CPU core (the one that runs `loop()`, together with the sound): the emulation copies the VDP state a line needs (up to 32 lines are queued) and goes on without waiting for each line to be drawn. A VRAM or sprite-table change can show up to 32 lines early.
- **Colours**: the VGA output of the TTGO VGA32 has 2 bits per colour (64 colours); the Mega Drive's 3 bits per colour are reduced to the top 2.
- **Sound**: the YM2612 (FM 6 channels + DAC) and the SN76489 are generated at 16384 Hz on the other CPU core. The emulation only queues the register writes with their time; the YM2612 timers run on the emulation side.
- **Memory**: VRAM, Z80 RAM, the 68000 dispatch table and 68000 RAM are put in the ESP32's internal RAM in this order (whatever does not fit goes to PSRAM, or stays in flash for the table). The ROM is in PSRAM.
- **68000**: Musashi set up for the 68000 only, with the same results as Musashi. Each instruction finds its handler in a two-level table (about 21 KB, copied to internal RAM at start) instead of Musashi's 64K-entry table in flash, and the handler returns its clock count (no cycle table is read). The handlers run most often, chosen from a profile of many games, run from the ESP32's internal instruction RAM (IRAM): about 93% of the executed instructions. ROM and 68000 RAM are read and written without a function call. Short wait loops that only poll the VDP status or memory are fast-forwarded to the end of the current line slice, with the same result as running them.
- **Z80**: z80core, written for this collection (the undocumented instructions and flags included). Its main loop runs from IRAM, the interrupt check runs only when something may have changed it, and the Z80 RAM is read and written without a function call.
- **Interrupts**: the vertical interrupt is raised 788 master clocks into the first blanking line, so a line interrupt at the end of the last visible line is taken first (as on the real machine). A line interrupt that is pending while the game turns line interrupts off is taken when they are turned on again. The Z80's vertical interrupt stays asserted until the Z80 takes it.
- **Build switches**: the speed-ups and the interrupt timings above can be switched back at the top of `md_m68k.h` (MD_M68K_*, MD_WAITSKIP) and `MegaDrive.ino` / `gwenesis_vdp.h` (MD_HINT_*, MD_VINT_DELAY). The defaults are recommended.

## Known limitations

- 3-button pad (no 6-button pad), player 1 only.
- No cartridges with mappers or extra chips (SVP etc.), no switched backup RAM of ROMs over 2 MB, no EEPROM saves.
- Interlace mode is not shown; shadow / highlight colours are shown as normal colours (as in Gwenesis on embedded targets).
- PAL (50 Hz) games run at the NTSC frame rate.
- Speed: heavy scenes run slower than the real machine. Some games do not run correctly.

---
---

# SEGA メガドライブ / GENESIS エミュレータ for ESP32(TTGO VGA32)

LILYGO TTGO VGA32(ESP32 + FabGL)用のメガドライブ / GENESIS のエミュレータです。bzhxx さんの Gwenesis を元にしています。
このシリーズのほかのエミュレータと同じ作りです(起動すると ROM の一覧、PS/2 キーボード、Serial1 のジョイスティック、F12 の一時停止)。

> ⚠️ **開発・ライセンスに関するご注意**
>
> メインソースコードの大部分は AI 生成コードをベースに構築・改変しているため、開発者もすべてのロジックを完全に把握しているわけではありません。そのため、ソースコードの詳細な仕様や内部ロジックに関する質問にはお答えできかねます。あらかじめご了承ください。
> ライセンスの詳細な条件はソースコード冒頭のヘッダコメントを参照し、同意の上でご利用ください。

---

## クレジットとライセンス

- **エミュレーションのコア**: bzhxx さんの Gwenesis(https://github.com/bzhxx/gwenesis。ソースのファイルは GPLv3 以降)。Retro-Gamer(GPLv3)に入っていたものを使い、`MegaDrive.ino` の先頭に書いたとおりに変えています(2026-11-12〜2026-11-26 の変更)。
- **68000 CPU コア**: Karl Stenerud さんの Musashi(MIT ライセンス、https://github.com/kstenerud/Musashi)。68000 用に設定・生成し、`MegaDrive.ino` の先頭に書いたとおりに変えています(2026-11-20〜2026-11-26: PC で作った表、2 段の振り分け、クロック数を返す命令の処理)。
- **Z80 CPU コア**: z80core。このシリーズのために Z80 の資料から新しく書いたものです(MIT ライセンス、Copyright Kure Ichiro。`z80core.c`・`z80core.h`)。
- **FM 音源**: このシリーズの PC-8801 版の YM2203/OPNA の FM を YM2612 として使っています。
- **グラフィック/オーディオエンジン**: Fabrizio Di Vittorio さんの FabGL(GPLv3)。

Gwenesis・FabGL と同じく、このスケッチ全体を GPLv3 で、ソースごと配ります(`LICENSE`)。Musashi のファイルと z80core には MIT ライセンスの表示をそのまま残しています。

---

## 🛠️ 開発環境とビルド設定

- **ボードマネージャ**: ESP32 by Espressif Systems(**v2.0.5 推奨**)
- **ライブラリ**: FabGL v1.0.9。**ファイルを 1 つ差し替えてください**(必須): Arduino のライブラリのフォルダの `FabGL/src/displaycontroller.cpp` を、`fabgl_patch/displaycontroller.cpp` で上書きします。元の FabGL は描画の処理を ESP32 の内部の命令 RAM(IRAM)に置くため、そのままではこのスケッチが入りません(リンクで「iram0_0_seg overflowed」になります)。差し替えたファイルはその処理をフラッシュに置くだけで(IRAM が約 8KB 空きます)、ほかは何も変わらず、ほかの FabGL のスケッチもそのまま動きます。MZ-2500 版と同じファイルで、ライセンスは FabGL と同じ GPL v3 です(`fabgl_patch/LICENSE`)。
- **ボード**: `ESP32 Dev Module`(TTGO VGA32)
- **PSRAM**（ON = `"Enabled"`、OFF = `"Disabled"`）: PSRAMをONにするとソフトでのエラッタ対策が入り、20%程度速度が低下します。PSRAMをOFFにするとソフトでPSRAMを有効にし、ソフトでのエラッタ対策は入りません。但し、ESP32のリビジョンが3以降でないと不具合が発生するため、それ以外のリビジョンでPSRAMをOFFにしても起動しなくしてあります。このエミュレータは処理が重いので、リビジョン3以降のESP32でPSRAMをOFFにすることをお勧めします(ONでも動きますが、遅くなります)。
- **Partition Scheme**: 既定のままでかまいません(プログラムは約 1MB)。
- **SD カードのピン**（`.ino` の先頭近くの `SD_SCLK` / `SD_MISO` / `SD_MOSI` / `SD_CS`）: ボードによって違います。既定は TTGO VGA32（14 / 2 / 12 / 13）です。ORANGE ESPer は 14 / 35 / 12 / 13、スイッチサイエンスの FabGL 開発ボードは 14 / 35 / 2 / 13 です。FabGL 開発ボードは SD カードを MISO = 16、MOSI = 17 で配線していますが、ESP32-WROVER ではこの 2 本はモジュールの中で PSRAM に使われていて外にはつながっていない（NC）ため、FabGL ボード上で 16 → 35、17 → 2 にジャンパしてください。使うボードのブロックを有効にし、ほかはコメントにしてください。
- **EmuLauncher での機種の切り替え**（使うときだけ）: `EmuLauncher` のフォルダの `emu_switch.h` と `partitions.csv` を、このスケッチのフォルダにコピーします。ROM の一覧のいちばん下に次に起動する機種が出て（TAB かコントローラの ボタン 3で変える）、フラッシュの分け方が `partitions.csv` のとおりになります（Partition Scheme の設定は使われません）。2 つのファイルが無ければ、今までどおり単体で動きます。詳しくは `EmuLauncher/README.md` を見てください。

---

## 📂 SD カード

```text
/MD/ROM/   .md / .gen / .bin / .smd(フォルダに分けてもよい)
```

- **ROM の大きさ**: ROM は PSRAM(4MB)に読み込み、PSRAM はほかのバッファと共用なので、上限は PSRAM の空きで決まり、28M ビット(3.5MB)までです。32M ビット(4MB)の ROM は入りません(「ROM LOAD ERROR」になります)。

---

## 🎮 操作

| | キーボード | コントローラ(Serial1) |
|---|---|---|
| 方向 | 矢印キー | 方向 |
| A / B / C | Z / X / C | ボタン 1 / 2 / 3 |
| START | Enter | ボタン 4 |

- **コントローラ**: 1P 専用・4 ボタンです。Serial1 のアダプタからはほかの機種と同じく 1 バイトずつ届きます(0 = 押下。bit0〜3 = 上下左右、bit4〜6 = ボタン 1〜3)が、この機種では bit7 がボタン 4 です(ほかの機種は bit7 で 1P と 2P を分けています)。アダプタは 1 コン 4 ボタンのモードにしてください。
- **F12**: 一時停止 / 再開。**Shift + F12**: リセット(ESP32 ごと再起動して ROM の一覧に戻る)。
- **F9 / F10 / F11**: ステートのスロット 2 / 1 / 0 を読み込む。**Shift + F9 / F10 / F11**: そのスロットへ保存(SD カードの `/MD/SAVE/<ROM の名前>_<スロット>.sta`)。読み込んだあとの音は、次の音から鳴ります。
- バッテリーバックアップ RAM(ROM のヘッダに書いてあり、ROM の後ろにあるもの): ゲームが書き換えてから 2 秒後に SD カードの `/MD/SAVE/<ROM の名前>.srm` へ自動で保存し、ゲームを始めるときに読みます。2MB を超える ROM の切り替え式のものと、EEPROM のセーブには対応していません。
- **Shift + F8**: スプライトの横の枚数制限(1 ラインに 20 個)を外す / 戻す。ちらつきが減るゲームがあります。重い場面は遅くなることがあります。
- **ROM の一覧**: ↑↓ = 選ぶ、PageUp / PageDown = ページ、Enter = 起動 / フォルダに入る、BackSpace か ESC = 上のフォルダ。コントローラは上下 = 選ぶ、左右 = ページ(上下を押していないときに左右を少し押し続ける)、ボタン1 か 4 = Enter、ボタン2 = 上のフォルダ。TAB(コントローラはボタン 3)= 次に起動する機種(EmuLauncher を使うときだけ)。
- **ボタンの配置**: ROM の一覧のいちばん上の行(BUTTON 1 2 3 = …)で Enter を押すたびに、ボタン 1〜3(キーボードの Z / X / C)が A B C と C B A で入れ替わります。SD の `/MD/PAD.CFG` に保存されます。START は変わりません。

---

## 動き方

- **画面**: VDP の 1 ラインを FabGL の VGA の行バッファへ直接書きます(320×240 を縦横 2 倍にして 640×480・59.94Hz で出す)。画面全体のバッファも写しもありません。フレームは VGA の垂直帰線で始めるので、絵が裂けません。予定より遅れて始まったフレームは描画を間引きます(続けて飛ばすのは 2 フレームまで)。ラインはもう一方の CPU コア(`loop()` のコア。音と同じ)で描きます。エミュレーションはそのラインが使う VDP の状態を写して(32 ライン分までためる)、1 ラインずつ描き終わりを待たずに先へ進みます。VRAM・スプライトの表の変化が最大 32 ライン早く出ることがあります。
- **色**: TTGO VGA32 の VGA は 1 色 2 ビット(64 色)なので、メガドライブの 1 色 3 ビットの上の 2 ビットを使います。
- **音**: YM2612(FM 6 チャンネル + DAC)と SN76489 は、もう一方の CPU コアで 16384Hz で作ります。エミュレーション側はレジスタの書き込みを時刻付きで積むだけで、YM2612 のタイマーはエミュレーション側で数えます。
- **メモリ**: VRAM・Z80 の RAM・68000 の命令の振り分け表・68000 の RAM を、この順に ESP32 の内部 RAM に置きます(入らないものは PSRAM。振り分け表はフラッシュのまま)。ROM は PSRAM です。
- **68000**: Musashi を 68000 だけの設定にしています(結果は Musashi と同じ)。命令の処理は、フラッシュにある Musashi の 6 万 5 千項目の表ではなく、2 段の表(約 21KB。起動時に内部 RAM へ写す)で探します。命令の処理は自分のクロック数を返すので、クロック数の表も読みません。多くのゲームで数えて実行回数の多かった命令の処理は、ESP32 の内部の命令用 RAM(IRAM)で動かします(実行される命令の約 93%)。ROM と 68000 の RAM の読み書きは関数を呼ばずにその場でします。VDP の状態やメモリを読んで待つだけの短いループは、そのラインの区切りまで早送りします(結果は早送りしないときと同じ)。
- **Z80**: このシリーズのために書いた z80core です(非公開の命令やフラグも扱います)。実行の本体は IRAM で動かし、割り込みの確認は変わりうるときだけにし、Z80 の RAM の読み書きは関数を呼ばずにその場でします。
- **割り込み**: V 割り込みは帰線期間の最初のラインに入って 788 マスタークロック後に出すので、最後の表示ラインの H 割り込みが先に受け付けられます(実機と同じ順)。H 割り込みが受け付けられる前にゲームが H 割り込みを無効にした場合は、有効に戻したときに受け付けます。Z80 の V 割り込みは、Z80 が受け付けるまで出したままにします。
- **ビルドスイッチ**: 上の高速化と割り込みのタイミングは、`md_m68k.h`(MD_M68K_…、MD_WAITSKIP)と `MegaDrive.ino` / `gwenesis_vdp.h`(MD_HINT_…、MD_VINT_DELAY)の先頭で前の作りに戻せます。既定のままをお勧めします。

## できないこと

- 3 ボタンのパッド(6 ボタンのパッドには対応していません)、1P だけです。
- マッパーや追加のチップ(SVP など)のあるカートリッジ、2MB を超える ROM の切り替え式のバックアップ RAM、EEPROM のセーブには対応していません。
- インターレースのモードは表示しません。シャドウ / ハイライトの色は普通の色で表示します(Gwenesis の組み込み用と同じ)。
- PAL(50Hz)のソフトも NTSC のフレームの速さで動きます。
- 速さ: 重い場面は実機より遅くなります。正しく動かないソフトもあります。
