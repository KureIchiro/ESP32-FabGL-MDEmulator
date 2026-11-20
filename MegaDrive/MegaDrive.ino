/* ============================================================================
 *  SEGA MEGA DRIVE / GENESIS Emulator for ESP32 (Work in Progress)
 *  --- TTGO VGA32 / FabGL。画面は 320x240(VGA 640x480 と同じタイミングで縦横 2 倍)、64 色 ---
 * ============================================================================
 *
 * 【クレジットと謝辞】
 *  - エミュレーションのコア: bzhxx 氏の Gwenesis(ソースのファイルは GPLv3 以降。出元: https://github.com/bzhxx/gwenesis)。
 *    Retro-Gamer(GPLv3)に入っていたものを使い、下の「変更」のとおり手を入れている
 *  - 68000 CPU コア: Karl Stenerud 氏の Musashi(MIT ライセンス。出元: https://github.com/kstenerud/Musashi)。
 *    68000 用に設定・生成して手を入れている(md_m68k.c・md_m68k.h。下の「変更」)
 *  - Z80 CPU コア: z80core(このシリーズのために新しく書いたもの。MIT ライセンス。z80core.c・z80core.h)
 *  - FM 音源: PC-88 版の YM2203/OPNA の FM(ym2203.cpp)を YM2612 として使う
 *  - グラフィック/オーディオエンジン: Fabrizio Di Vittorio 氏による FabGL(GPLv3)
 *
 * 【ライセンス】GPLv3(LICENSE)。Gwenesis・FabGL と同じく、このスケッチ全体を GPLv3 で配る
 *  (Musashi のファイル(m68k.h・m68kconf.h・m68kcpu.c/.h・m68kops.c/.h)と z80core は MIT ライセンスの表示をそのまま残す)。
 *
 * 【変更(GPLv3 第 5 条 a)】2026-11-12 Gwenesis を TTGO VGA32 用のスケッチにした。
 *  ・画面: VDP の 1 ラインを FabGL の VGA の行バッファへ直接書く(gwenesis_vdp_gfx.c の gwenesis_vdp_render_line)。
 *    CRAM の色はそのたびに VGA の画素の値に直す(gwenesis_vdp_mem.c の md_pix_set)
 *  ・音: YM2612 と SN76489 は書き込みを時刻付きで積み、別のコアで 16384Hz で作る(md_sound.cpp。ym2612.c と
 *    gwenesis_sn76489.c の代わり)。YM2612 のタイマーはエミュレーション側でマスタークロックで数える
 *  ・68000 の読み書き: 作業 RAM と ROM を先に見て、それ以外だけ元の振り分けへ(gwenesis_bus.c)。ROM は読み込んだ
 *    バッファをそのまま使う(写しを作らない)
 *  ・VRAM・Z80 の RAM・68000 の RAM はこの順に内部 RAM に置く(入らないものは PSRAM)
 *  ・68000・Z80 のよく通る関数は IRAM に置く(68000・Z80 とも 2026-11-20 の変更を見る)
 *  ・EmuLauncher での機種の切り替え(スケッチのフォルダに emu_switch.h があるときだけ。ID は "MD")
 *  2026-11-13 速くした。
 *  ・描画をもう一方のコア(loop())で: cpuTask はラインが使う VDP の状態を写して(gwenesis_vdp_render_capture)
 *    描くのを頼み、次のラインへ進む(「描画をもう一方のコア」)。プレーン B は表を 1 回引くごとに 2 ドット
 *  ・各ファイルを O3 とジャンプテーブルありでコンパイルする(Arduino の既定は -Os・ジャンプテーブルなし)。
 *    Z80 のサイクル数の表は内部 RAM に置く(2026-11-20 に Z80 のコアを替えたので、今は無い)
 *  2026-11-16 直した。
 *  ・Z80 のリセット(A11200h)に 1 を書くたびに Z80 をリセットし直していた。リセット中から戻るときだけにした
 *    (Z80 を待つあいだ 1 を書き直すソフトが先へ進まなかった。z80inst.c)
 *  ・Z80 が 2000h 以降から命令を読むと別のメモリを読んでいた(2026-11-20 に Z80 のコアを替えた)
 *  ・音: DAC(2Ah)の書き込みで FM の合成を区切らず、サンプルごとに反映する(PCM を速く書き続けるソフトで、
 *    FM の合成が 1 サンプルずつになっていた。md_sound.cpp)
 *  ・コントローラ(Serial1)を 1P 専用にし、bit7 をボタン4(START)にした
 *  2026-11-17 速くした。
 *  ・Z80 が Z80 の RAM(0000h〜3FFFh)を読み書きするときは、関数を呼ばずにその場で(2026-11-20 から z80core の 8KB ページ)
 *  ・IRAM の割り振り: 描画(gwenesis_vdp_render_line)と音の合成(mdSoundFill・ym2203Generate)はもう一方のコアだけが
 *    使うのでフラッシュに置き、空いた分によく使う 68000 の命令(160 個。M68K_IRAM。2026-11-20 から m68kops.c)を置く
 *  ・68000 の待ちループの早送り(m68k_waitskip。MD_WAITSKIP。2026-11-20 から md_m68k.c): VDP の状態や RAM を読んで待つだけの
 *    短いループは、1 ラインの区切りまで結果が変わらないので、区切りの 1 周前まで飛ばす(飛ばさないときと同じ動き)
 *  2026-11-18 速くした。
 *  ・描画をもう一方のコアに頼むラインの状態を 32 ライン分ためられるようにした(gwenesis_vdp_gfx.c の md_ring)。
 *    cpuTask は 1 ラインずつ描き終わりを待たず、待つのはいっぱいのときと帰線期間に入る前だけ
 *  ・ビルドスイッチ MD_OUT_WORD(gwenesis_vdp_gfx.c。既定 1): ラインの色の番号を VGA の画素に直すとき、4 ドットを
 *    1 回で読む(0 = 1 ドットずつ。今まで)
 *  2026-11-18 直した。
 *  ・PSG(SN76489): 周期 0 は 0 のまま数え(セガの PSG。MAME と同じ)、周期 6 以下のチャンネルは聞こえない速さの反転の
 *    平均(音量の半分)を出す(md_sound.cpp)。周期 0 を 1024 として数えていたので、音量だけで PCM を鳴らすソフト
 *    (After Burner II など)で約 109Hz の音が鳴り続けていた
 *  2026-11-19 速くした(絵は同じ)。
 *  ・スプライト: 表(SAT_CACHE)をリンクの順に並べた一覧を、表が変わったときだけ作り直す。ラインごとには一覧の
 *    Y の範囲だけを比べる(gwenesis_vdp_gfx.c・gwenesis_vdp_mem.c の md_sat_gen)
 *  ・プレーン A・ウィンドウもプレーン B と同じく表を 1 回引くごとに 2 ドット。縦スクロールがライン全体で 1 つの
 *    ときは、ネームテーブルの行をラインの頭で 1 回だけ求める
 *  2026-11-19 変えた。
 *  ・ボタンの配置(A B C / C B A)を ROM の一覧のいちばん上の行で選べるようにし、/MD/PAD.CFG に保存する
 *  ・ROM の一覧: 上下を素早く動かしたときに 1 画面分飛ぶことがあったのを直した
 *  ・PSG の大きさを半分にした(実機と聞き比べて。md_sound.cpp の MD_PSG_SHIFT)
 *  ・計測のための表示(内訳)を外した。SPEED の行だけ残す
 *  2026-11-19 直した(esp-cpp/gwenesis の ESP32 版を参考に。どれもビルドスイッチで前に戻せる)。
 *  ・V 割り込みを、帰線期間の最初のラインに入って 788 マスタークロック後に出す(MD_VINT_DELAY)。最後の表示ラインの
 *    H 割り込みが先に受け付けられる(逆だと、H 割り込みでスプライトを書き換えるソフトが正しく進まなかった)
 *  ・Z80 の V 割り込みを、Z80 が受け付けるまで出したままにする(z80inst.h の MD_Z80_VINT_HOLD)。1 ラインだけだと、
 *    Z80 が割り込みを禁止しているあいだに来たものを取りこぼしていた
 *  ・遅れたフレームの描画を、続けて 2 フレームまで間引く(MD_MAX_SKIP。今までは 1 フレーム)
 *  2026-11-20 直した。
 *  ・ウィンドウを下側に出す設定(レジスタ 18 の bit7)で、境目のライン(VPOS×8)がウィンドウにならずプレーン A を
 *    出していた(VPOS 0 だと画面のいちばん上のライン。gwenesis_vdp_gfx.c)
 *  2026-11-20 Shift+F8 でスプライトの横の枚数制限を外せるようにした(MSX・マークIII 版と同じキー。既定は制限あり)
 *  2026-11-20 68000 のコアを Musashi(MIT ライセンス)の今の版に替えた。Gwenesis に入っていたもの(Musashi 3.32 を
 *    Genesis Plus GX 用に変えたもの)は非商用の条件付きで、GPLv3 のこのスケッチと一緒に配れないため。
 *  ・m68kmake で作った命令の処理(m68kops.c)のうち 68000 が使うものだけを残し、命令の表とサイクル数の表は PC で
 *    作って定数にした(フラッシュに置く。make_md_m68k.py)。68000 だけなので、CPU の種類・アドレスのマスク・
 *    サイクル数の表を定数にした(MD_M68K_CONST)。FPU・PMMU・バスエラーのコードは入れない
 *  ・ROM と 68000 の RAM の読み書き、命令の読み込みは関数を呼ばずにその場で(md_m68k.h)
 *  ・時間はマスタークロック(68000 の 7 倍)で数える(md_m68k.c の m68k_run・m68k_cycles_master)
 *  ・よく使う命令 160 個の IRAM 配置と、待ちループの早送り(MD_WAITSKIP)はそのまま移した
 *  2026-11-20 Z80 のコアを z80core(このシリーズのために Z80 の資料から新しく書いたもの。MIT ライセンス)に替えた。
 *    Marat Fayzullin 氏のコアは商用の配布を禁じていて、GPLv3 のこのスケッチと一緒に配れないため。
 *  ・命令を読む・Z80 の RAM の読み書きは 8KB ごとのページで(Z80 の RAM 0000h〜3FFFh)、ほかは z80inst.c の
 *    md_z80_read / md_z80_write。プログラムカウンタとサイクル数はローカル変数で持つ。実行の本体は IRAM、ED 命令はフラッシュ
 *  ・非公開の命令(IXH / IXL、DD CB の結果をレジスタにも入れる形、ED の別名)、フラグの 5・3 ビット、MEMPTR、
 *    R レジスタも資料どおり
 *
 * 【必要なファイル（SDカード）】
 *  /MD/ROM の .md / .gen / .bin / .smd : ROM(フォルダに分けてもよい)。起動すると一覧が出る
 *
 * 【操作】
 *  キーボード: ↑↓←→ = 方向、Z = A、X = B、C = C、Enter = START
 *  コントローラ(Serial1。1P 専用・4 ボタン): 方向、ボタン1 = A、ボタン2 = B、ボタン3 = C、ボタン4 = START
 *  (ROM の一覧のいちばん上の行で C B A にもできる。キーボードの Z / C も入れ替わる)
 *  F12: 一時停止 / 再開。Shift+F12: リセット(ESP32 ごと。ROM の一覧に戻る)
 *  Shift+F8: スプライトの横の枚数制限(1 ラインに 20 個)を外す / 戻す(ちらつきが減るゲームがある。重い場面は遅くなる)
 *  ROM の一覧で TAB / ボタン3: 次に起動する機種(EmuLauncher を使うときだけ)
 * ============================================================================
 */

#pragma GCC optimize("O3")

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <SPI.h>
#include <SD.h>
#if __has_include("emu_switch.h")
#include "emu_switch.h"
#define EMU_SWITCH 1
#else
#define EMU_SWITCH 0
#endif
#include <fabgl.h>
#include <vector>
#include <algorithm>
#include "soc/rtc_wdt.h"
#include "esp_task_wdt.h"
#ifndef BOARD_HAS_PSRAM
#include "esp32/spiram.h"
#include "esp_heap_caps.h"
#define ps_malloc(n) heap_caps_malloc((n), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#endif
#if defined(BOARD_HAS_PSRAM) && !defined(EMU_PSRAM_NO_ERRATA_FIX)
#define psramFixMissing() false
#else
#define psramFixMissing() (ESP.getChipRevision() < 3)
#endif

extern "C" {
#include "gwenesis.h"

}
#include "md_sound.h"

//#define USE_SERIAL_DEBUG

#define MD_DIR "/MD"
#define MD_ROM_DIR MD_DIR "/ROM"
#define MD_ROM_MAX (4u * 1024 * 1024)

// TTGO VGA32
#define SD_SCLK 14
#define SD_MISO 2
#define SD_MOSI 12
#define SD_CS   13

// ORANGE ESPer
// #define SD_SCLK 14
// #define SD_MISO 35
// #define SD_MOSI 12
// #define SD_CS   13

// Switch Science FabGL
// #define SD_SCLK 14
// #define SD_MISO 35
// #define SD_MOSI 2
// #define SD_CS   13

#define MD_SCREEN_W 320
#define MD_SCREEN_H 240

extern "C" {
int system_clock = 0;
int scan_line = 0;
extern int zclk;
extern unsigned char *M68K_RAM;
extern unsigned char *ZRAM;
extern unsigned char *VRAM;
extern unsigned char gwenesis_vdp_regs[0x20];
extern unsigned short gwenesis_vdp_status;
extern int screen_width, screen_height;
extern int hint_pending;
extern unsigned char button_state[3];
extern unsigned int md_rom_size;
extern uint8_t **md_rows;
extern uint8_t md_sync;
extern int md_yofs;
}

fabgl::VGAController DisplayController;
fabgl::Canvas *canvas;
fabgl::SoundGenerator soundGenerator;
fabgl::PS2Controller PS2Controller;

class MdWave : public fabgl::WaveformGenerator {
public:
  int getSample() override { return mdSoundGetSample(); }
  void setFrequency(int value) override { (void)value; }
};
static MdWave mdWave;

static volatile bool isPaused = false;
static volatile bool cpuStopped = false;
static volatile bool frameReady = false;
static bool renderOnCore1 = false;
static volatile uint32_t renderQueued = 0, renderDone = 0;
static void renderService();
static uint8_t *vpRows[MD_SCREEN_H];

static volatile uint8_t joyBits = 0xFF;
static void updateJoystick(bool hold) {
  uint8_t j = hold ? joyBits : 0xFF;
  while (Serial1.available() > 0) j = (uint8_t)Serial1.read();
  joyBits = j;
}

static int joyMenuPoll() {
  static uint8_t state = 0xFF, last = 0xFF, cand = 0xFF;
  static uint32_t lastRx = 0, repeatAt = 0;
  uint32_t now = millis();
  while (Serial1.available() > 0) {
    const uint8_t b = (uint8_t)Serial1.read();
    if (b == cand) state = b;
    cand = b;
    lastRx = now;
  }
  if (now - lastRx > 500) state = cand = 0xFF;
  uint8_t pressed = (uint8_t)~state, prev = (uint8_t)~last;
  uint8_t edge = pressed & ~prev;
  last = state;
  if (edge & 0x90) return 'A';
  if (edge & 0x20) return 'B';
  if (edge & 0x40) return 'C';
  for (int i = 0; i < 2; i++) {
    if (!(pressed & (1 << i))) continue;
    if (edge & (1 << i)) { repeatAt = now + 400; return i ? 'D' : 'U'; }
    if ((int32_t)(now - repeatAt) >= 0) { repeatAt = now + 120; return i ? 'D' : 'U'; }
  }
  static uint8_t lrHeld = 0;
  static uint32_t lrAt = 0;
  const uint8_t lr = (pressed & 0x03) ? 0 : (uint8_t)(pressed & 0x0C);
  if (lr != lrHeld) { lrHeld = lr; lrAt = now + 150; }
  if ((lr == 0x04 || lr == 0x08) && (int32_t)(now - lrAt) >= 0) { lrAt = now + 400; return lr == 0x04 ? 'L' : 'R'; }
  return 0;
}
static int joyMenuScancode(bool *e0) {
  *e0 = true;
  switch (joyMenuPoll()) {
    case 'U': return 0x75;
    case 'D': return 0x72;
    case 'L': return 0x7D;
    case 'R': return 0x7A;
    case 'A': *e0 = false; return 0x5A;
    case 'B': *e0 = false; return 0x76;
    case 'C': *e0 = false; return 0x0D;
    default: return 0;
  }
}

enum { PAD_BIT_UP = 0, PAD_BIT_DOWN, PAD_BIT_LEFT, PAD_BIT_RIGHT, PAD_BIT_B, PAD_BIT_C, PAD_BIT_A, PAD_BIT_START };

#define MD_PAD_FILE MD_DIR "/PAD.CFG"
static bool padSwap = false;
static void loadPadLayout() {
  File f = SD.open(MD_PAD_FILE, FILE_READ);
  if (!f) return;
  padSwap = f.read() == 'C';
  f.close();
}
static void savePadLayout() {
  SD.remove(MD_PAD_FILE);
  File f = SD.open(MD_PAD_FILE, FILE_WRITE);
  if (!f) return;
  f.write((uint8_t)(padSwap ? 'C' : 'A'));
  f.close();
}
static const char *padLayoutLabel() { return padSwap ? "BUTTON 1 2 3 = C B A  (ENTER: CHANGE)" : "BUTTON 1 2 3 = A B C  (ENTER: CHANGE)"; }
static volatile uint8_t kbdPad = 0;
static bool shiftDown = false;

static void setKbdBit(int bit, bool down) {
  if (down) kbdPad |= (uint8_t)(1 << bit);
  else kbdPad &= (uint8_t)~(1 << bit);
}

static void showStatus(const char *msg);
extern "C" volatile uint8_t md_sprite_nolimit;
static uint32_t statusUntil = 0;
static void flashStatus(const char *msg, uint32_t ms) {
  showStatus(msg);
  statusUntil = millis() + ms;
  if (!statusUntil) statusUntil = 1;
}
static void statusTick();

static void updateKeyboard() {
  static bool isBreak = false, isE0 = false;
  auto kb = PS2Controller.keyboard();
  if (!kb) return;
  while (kb->scancodeAvailable() > 0) {
    int sc = kb->getNextScancode(0);
    if (sc == 0xF0) { isBreak = true; continue; }
    if (sc == 0xE0) { isE0 = true; continue; }
    const bool down = !isBreak, e0 = isE0;
    isBreak = false; isE0 = false;
    if (!e0 && (sc == 0x12 || sc == 0x59)) { shiftDown = down; continue; }
    if (e0) {
      switch (sc) {
        case 0x75: setKbdBit(PAD_BIT_UP, down); break;
        case 0x72: setKbdBit(PAD_BIT_DOWN, down); break;
        case 0x6B: setKbdBit(PAD_BIT_LEFT, down); break;
        case 0x74: setKbdBit(PAD_BIT_RIGHT, down); break;
        case 0x5A: setKbdBit(PAD_BIT_START, down); break;
      }
      continue;
    }
    switch (sc) {
      case 0x1A: setKbdBit(padSwap ? PAD_BIT_C : PAD_BIT_A, down); break;
      case 0x22: setKbdBit(PAD_BIT_B, down); break;
      case 0x21: setKbdBit(padSwap ? PAD_BIT_A : PAD_BIT_C, down); break;
      case 0x5A: setKbdBit(PAD_BIT_START, down); break;
      case 0x0A:
        if (!down || !shiftDown) break;
        md_sprite_nolimit = !md_sprite_nolimit;
        flashStatus(md_sprite_nolimit ? "NO LIMIT SPRITES" : "LIMIT SPRITES", 1000);
        break;
      case 0x07:
        if (!down) break;
        if (shiftDown) ESP.restart();
        if (!isPaused) {
          isPaused = true;
          md_sound_mute = true;
          while (!cpuStopped) {
            renderService();
            if (!renderOnCore1) vTaskDelay(1);
          }
          showStatus("PAUSED  F12:RESUME  SHIFT+F12:RESET");
        } else {
          isPaused = false;
          md_sound_mute = false;
        }
        break;
    }
  }
}

static void updatePad() {
  const uint8_t j = (uint8_t)~joyBits;
  uint8_t p = kbdPad;
  if (j & 0x01) p |= 1 << PAD_BIT_UP;
  if (j & 0x02) p |= 1 << PAD_BIT_DOWN;
  if (j & 0x04) p |= 1 << PAD_BIT_LEFT;
  if (j & 0x08) p |= 1 << PAD_BIT_RIGHT;
  if (j & 0x10) p |= 1 << (padSwap ? PAD_BIT_C : PAD_BIT_A);
  if (j & 0x20) p |= 1 << PAD_BIT_B;
  if (j & 0x40) p |= 1 << (padSwap ? PAD_BIT_A : PAD_BIT_C);
  if (j & 0x80) p |= 1 << PAD_BIT_START;
  button_state[0] = (uint8_t)~p;
}

static void statusTick() {
  if (!statusUntil || isPaused || (int32_t)(millis() - statusUntil) < 0) return;
  statusUntil = 0;
  const uint8_t bg = DisplayController.createRawPixel(fabgl::RGB222(0, 0, 0));
  for (int r = 0; r < 10; r++) memset(vpRows[MD_SCREEN_H - 10 + r], bg, MD_SCREEN_W);
}

static void showStatus(const char *msg) {
  const uint8_t bg = DisplayController.createRawPixel(fabgl::RGB222(0, 0, 0));
  const uint8_t fg = DisplayController.createRawPixel(fabgl::RGB222(3, 3, 3));
  const fabgl::FontInfo &fnt = fabgl::FONT_8x8;
  for (int r = 0; r < 10; r++) memset(vpRows[MD_SCREEN_H - 10 + r], bg, MD_SCREEN_W);
  for (int i = 0; msg[i] && i < MD_SCREEN_W / 8; i++) {
    const uint8_t *g = fnt.data + (uint8_t)msg[i] * 8;
    for (int r = 0; r < 8; r++)
      for (int k = 0; k < 8; k++)
        vpRows[MD_SCREEN_H - 9 + r][(i * 8 + k) ^ 2] = (g[r] & (0x80 >> k)) ? fg : bg;
  }
}

struct RomItem { String name; int kind; };
static std::vector<RomItem> romItems;

static bool isRomName(const String &name) {
  String u = name; u.toUpperCase();
  return u.endsWith(".MD") || u.endsWith(".GEN") || u.endsWith(".BIN") || u.endsWith(".SMD");
}

static void listRoms(const String &path, bool canGoUp) {
  romItems.clear();
  File dir = SD.open(path.c_str());
  if (!dir) return;
  std::vector<RomItem> dirs, files;
  File e;
  while ((e = dir.openNextFile())) {
    String name = e.name();
    int s = name.lastIndexOf('/');
    if (s >= 0) name = name.substring(s + 1);
    if (name.length() && name[0] != '.' && name != "System Volume Information") {
      if (e.isDirectory()) dirs.push_back({ name, 1 });
      else if (isRomName(name)) files.push_back({ name, 2 });
    }
    e.close();
  }
  dir.close();
  auto byName = [](const RomItem &a, const RomItem &b) { return strcasecmp(a.name.c_str(), b.name.c_str()) < 0; };
  std::sort(dirs.begin(), dirs.end(), byName);
  std::sort(files.begin(), files.end(), byName);
  if (canGoUp) romItems.push_back({ "..", 0 });
  romItems.insert(romItems.end(), dirs.begin(), dirs.end());
  romItems.insert(romItems.end(), files.begin(), files.end());
}

#if EMU_SWITCH
static int emuNext = -1;
#endif

static String selectRom() {
  String sub = "", want = "";
  canvas->selectFont(&fabgl::FONT_8x8);
  auto kb = PS2Controller.keyboard();
  const int visible = 24;
  loadPadLayout();
#if EMU_SWITCH
  emuNext = emuNextInit("MD");
#endif

  for (;;) {
    String path = String(MD_ROM_DIR) + (sub == "" ? "" : "/" + sub);
    listRoms(path, sub != "");
    if (romItems.empty() && sub != "") { sub = ""; want = ""; continue; }
    const bool noRom = romItems.empty();
    if (sub == "") romItems.insert(romItems.begin(), RomItem{ "", 3 });
    int cursor = (sub == "" && romItems.size() > 1) ? 1 : 0;
    for (int i = 0; i < (int)romItems.size(); i++)
      if (romItems[i].kind != 3 && romItems[i].name == want) { cursor = i; break; }
    bool isBreak = false, isE0 = false, changeDir = false;
    int shownTop = -1, shownCursor = -1, listTop = 0;

    auto drawItem = [&](int idx, int top) {
      int y = 32 + (idx - top) * 8;
      canvas->setBrushColor(fabgl::Color::Blue);
      canvas->fillRectangle(0, y, MD_SCREEN_W - 1, y + 7);
      if (idx < 0 || idx >= (int)romItems.size()) return;
      const RomItem &it = romItems[idx];
      String label = it.kind == 3 ? String(padLayoutLabel()) : it.kind == 2 ? it.name : it.name + "/";
      if (label.length() > 38) label = label.substring(0, 37) + "~";
      canvas->setPenColor(idx == cursor ? fabgl::Color::BrightYellow
                          : it.kind == 3 ? fabgl::Color::BrightGreen
                          : it.kind == 2 ? fabgl::Color::BrightWhite : fabgl::Color::BrightCyan);
      canvas->drawText(8, y, label.c_str());
    };
#if EMU_SWITCH
    auto drawNext = [&]() {
      char m[48], line[64];
      emuNextLabel(emuNext, "MD", m, sizeof(m));
      snprintf(line, sizeof(line), "TAB/C: NEXT = %s", m);
      canvas->setBrushColor(fabgl::Color::Blue);
      canvas->fillRectangle(0, 228, MD_SCREEN_W - 1, 235);
      canvas->setPenColor(fabgl::Color::BrightCyan);
      canvas->drawText(8, 228, line);
    };
#endif
    auto draw = [&]() {
      if (cursor < listTop) listTop = cursor;
      if (cursor >= listTop + visible) listTop = cursor - visible + 1;
      const int top = listTop;
      if (shownTop < 0) {
        canvas->setBrushColor(fabgl::Color::Blue);
        canvas->clear();
        canvas->setPenColor(fabgl::Color::White);
        canvas->drawText(8, 4, "MEGA DRIVE  SELECT A ROM");
        canvas->drawText(8, 12, "ENTER:OK  BS:UP  (JOY: 1=OK 2=UP)");
        String shown = path + "/";
        if (shown.length() > 38) shown = "..." + shown.substring(shown.length() - 35);
        canvas->setPenColor(fabgl::Color::Cyan);
        canvas->drawText(8, 20, shown.c_str());
        if (noRom) {
          canvas->setPenColor(fabgl::Color::BrightRed);
          canvas->drawText(8, 40, "NO ROM IN " MD_ROM_DIR);
        }
#if EMU_SWITCH
        drawNext();
#endif
      }
      if (shownTop < 0 || top != shownTop) {
        for (int i = 0; i < visible; i++) drawItem(top + i, top);
      } else if (cursor != shownCursor) {
        drawItem(shownCursor, top);
        drawItem(cursor, top);
      }
      shownTop = top;
      shownCursor = cursor;
    };
    draw();

    while (!changeDir) {
      int jsc = 0;
      if (!kb->scancodeAvailable() && (jsc = joyMenuScancode(&isE0)) != 0) isBreak = false;
      if (jsc || kb->scancodeAvailable()) {
        int sc = jsc ? jsc : kb->getNextScancode(0);
        if (sc == 0xF0) { isBreak = true; continue; }
        if (sc == 0xE0) { isE0 = true; continue; }
        if (!isBreak) {
          const int n = (int)romItems.size();
          if (sc == 0x75 && isE0) { if (cursor > 0) cursor--; draw(); }
          else if (sc == 0x72 && isE0) { if (cursor < n - 1) cursor++; draw(); }
          else if (sc == 0x7D && isE0) { cursor = cursor - visible < 0 ? 0 : cursor - visible; draw(); }
          else if (sc == 0x7A && isE0) { cursor = cursor + visible >= n ? n - 1 : cursor + visible; if (cursor < 0) cursor = 0; draw(); }
#if EMU_SWITCH
          else if (sc == 0x0D) { emuNext = emuNextCycle(emuNext, "MD"); drawNext(); }
#endif
          else if (n > 0 && ((sc == 0x66 || sc == 0x76) && sub != "")) {
            int p = sub.lastIndexOf('/');
            want = p >= 0 ? sub.substring(p + 1) : sub;
            sub = p >= 0 ? sub.substring(0, p) : "";
            changeDir = true;
          } else if (n > 0 && sc == 0x5A && romItems[cursor].kind == 3) {
            padSwap = !padSwap;
            savePadLayout();
            drawItem(cursor, listTop);
          } else if (n > 0 && sc == 0x5A && romItems[cursor].kind == 0) {
            int p = sub.lastIndexOf('/');
            want = p >= 0 ? sub.substring(p + 1) : sub;
            sub = p >= 0 ? sub.substring(0, p) : "";
            changeDir = true;
          } else if (n > 0 && sc == 0x5A && romItems[cursor].kind == 1) {
            sub = sub == "" ? romItems[cursor].name : sub + "/" + romItems[cursor].name;
            want = "";
            changeDir = true;
          } else if (n > 0 && sc == 0x5A) {
            return path + "/" + romItems[cursor].name;
          }
        }
        isBreak = false; isE0 = false;
      }
      vTaskDelay(1);
    }
  }
}

static uint8_t *loadRom(const String &path, uint32_t *size) {
  File f = SD.open(path.c_str(), FILE_READ);
  if (!f) return nullptr;
  uint32_t sz = f.size();
  if (sz == 0 || sz > MD_ROM_MAX) { f.close(); return nullptr; }
  const uint32_t alloc = (sz + 0xFFFF) & ~0xFFFFu;
  uint8_t *buf = (uint8_t *)ps_malloc(alloc);
  if (!buf) { f.close(); return nullptr; }
  memset(buf + sz, 0xFF, alloc - sz);
  uint32_t done = 0;
  while (done < sz) {
    size_t n = f.read(buf + done, std::min((uint32_t)16384, sz - done));
    if (n == 0) break;
    done += n;
  }
  f.close();
  if (done != sz) { free(buf); return nullptr; }
  *size = sz;
  return buf;
}

#include "soc/i2s_struct.h"
static lldesc_t volatile *vgaDesc = nullptr;
static int vgaDescCount = 0;
static uint8_t *vgaDescRow = nullptr;

static inline int vgaRowNow() {
  uint32_t a = I2S1.out_link_dscr;
  uint32_t i = (a - (uint32_t)vgaDesc) / sizeof(lldesc_t);
  return i < (uint32_t)vgaDescCount ? vgaDescRow[i] : MD_SCREEN_H;
}

static void vgaSyncSetup() {
  lldesc_t volatile *start = (lldesc_t volatile *)I2S1.out_link_dscr;
  if (!start) { Serial.println("VGA sync: off (no DMA descriptor)"); return; }
  lldesc_t volatile *best = nullptr;
  int bestLen = 0, n = 0;
  lldesc_t volatile *d = start;
  do {
    if ((uint32_t)d < 0x3FF80000u || (uint32_t)d >= 0x40000000u) { Serial.println("VGA sync: off (bad descriptor)"); return; }
    lldesc_t volatile *next = (lldesc_t volatile *)d->qe.stqe_next;
    lldesc_t volatile *prevCont = d - 1;
    bool head = true;
    if ((uint32_t)prevCont >= 0x3FF80000u && (lldesc_t volatile *)prevCont->qe.stqe_next == d) head = false;
    if (head) {
      int len = 1;
      lldesc_t volatile *e = d;
      while ((lldesc_t volatile *)e->qe.stqe_next == e + 1 && len < 4096) { e++; len++; }
      if (len > bestLen) { bestLen = len; best = d; }
    }
    d = next;
    n++;
  } while (d && d != start && n < 4096);
  if (d != start || !best) { Serial.println("VGA sync: off (not a ring)"); return; }
  vgaDescRow = (uint8_t *)heap_caps_malloc(bestLen, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
  if (!vgaDescRow) { Serial.println("VGA sync: off (no memory)"); return; }
  int lastVis = -1, row = 0, views = 0;
  for (int i = 0; i < bestLen; i++) {
    const volatile uint8_t *b = (const volatile uint8_t *)best[i].buf;
    for (int y = 0; y < MD_SCREEN_H; y++) if (b == vpRows[y]) { row = y; lastVis = i; views++; break; }
    vgaDescRow[i] = (uint8_t)row;
  }
  for (int i = lastVis + 2; i < bestLen; i++) vgaDescRow[i] = MD_SCREEN_H;
  if (views < MD_SCREEN_H) {
    Serial.printf("VGA sync: off (%d of %d descriptors are rows)\n", views, bestLen);
    heap_caps_free(vgaDescRow); vgaDescRow = nullptr;
    return;
  }
  vgaDesc = best;
  vgaDescCount = bestLen;
  int seenVis = 0, seenBlank = 0;
  for (int64_t t0 = esp_timer_get_time(); esp_timer_get_time() - t0 < 20000; ) {
    if (vgaRowNow() < MD_SCREEN_H) seenVis++; else seenBlank++;
    ets_delay_us(50);
  }
  if (!seenVis || !seenBlank) {
    Serial.printf("VGA sync: off (position test failed: visible %d, blank %d)\n", seenVis, seenBlank);
    vgaDesc = nullptr;
    return;
  }
  Serial.printf("VGA sync: on (%d desc, %d rows, test visible %d / blank %d)\n", bestLen, views, seenVis, seenBlank);
}

static void vgaWaitVblank(int64_t frameStart) {
  for (;;) {
    int64_t el = esp_timer_get_time() - frameStart;
    if (el >= 17500) return;
    if (el < 8000) { vTaskDelay(pdMS_TO_TICKS((8000 - el) / 1000 + 1)); continue; }
    int r = vgaRowNow();
    if (r >= MD_SCREEN_H) return;
    int us = (MD_SCREEN_H - r) * 64;
    if (us > 2500) vTaskDelay(pdMS_TO_TICKS(us / 1000 - 1));
  }
}

#define MD_FRAME_US 16683

#ifndef MD_VINT_DELAY
#define MD_VINT_DELAY 788
#endif
#ifndef MD_MAX_SKIP
#define MD_MAX_SKIP 2
#endif

static inline uint32_t ccountNow() {
  uint32_t c;
  __asm__ __volatile__("rsr %0, ccount" : "=a"(c));
  return c;
}

static inline void renderWait() {
  while (renderDone != renderQueued) {}
}

static void renderService() {
  const uint32_t d = renderDone;
  if (d == renderQueued) return;
  gwenesis_vdp_render_slot(d % MD_RENDER_RING);
  renderDone = d + 1;
}

void cpuTask(void *pvParameters) {
  const int64_t frameTimeUs = MD_FRAME_US;
  int64_t nextFrameTime = esp_timer_get_time();
  int skipRun = 0;
  bool gaveUp = false;
  bool noWait = false, prevNoWait = false;

#ifdef USE_SERIAL_DEBUG
  int64_t dbgMin = INT64_MAX, dbgMax = 0, dbgSum = 0, dbgWinStart = 0, dbgIdle = 0;
  int dbgOverBudget = 0, dbgFrameCnt = 0, dbgSkipped = 0;
#endif

  while (1) {
    renderWait();
    if (isPaused) {
      cpuStopped = true;
      vTaskDelay(pdMS_TO_TICKS(10));
      nextFrameTime = esp_timer_get_time();
      continue;
    }
    cpuStopped = false;
#ifdef USE_SERIAL_DEBUG
    int64_t frameStart = esp_timer_get_time();
    if (dbgFrameCnt == 0) dbgWinStart = frameStart;
#endif
    updateJoystick(prevNoWait);
    prevNoWait = noWait;
    updatePad();

    const bool late = skipRun < MD_MAX_SKIP && (gaveUp || esp_timer_get_time() > nextFrameTime + 1000);
    gaveUp = false;
    const bool draw = !late;
    skipRun = late ? skipRun + 1 : 0;
#ifdef USE_SERIAL_DEBUG
    if (late) dbgSkipped++;
#endif
    nextFrameTime += frameTimeUs;

    const int lines_per_frame = REG1_PAL ? LINES_PER_FRAME_PAL : LINES_PER_FRAME_NTSC;
    int hint_counter = gwenesis_vdp_regs[10];
    screen_width = REG12_MODE_H40 ? 320 : 256;
    screen_height = REG1_PAL ? 240 : 224;
    md_yofs = (MD_SCREEN_H - screen_height) / 2;
    gwenesis_vdp_render_config();
    system_clock = 0;
    zclk = 0;
    scan_line = 0;
    while (scan_line < lines_per_frame) {
      if (draw && renderOnCore1 && scan_line == screen_height) {
        renderWait();
      }
#if MD_VINT_DELAY
      if (scan_line == screen_height) {
        m68k_run(system_clock + MD_VINT_DELAY);
        if (REG1_VBLANK_INTERRUPT != 0) {
          gwenesis_vdp_status |= STATUS_VIRQPENDING;
          m68k_set_irq(6);
        }
      }
#endif
      m68k_run(system_clock + VDP_CYCLES_PER_LINE);
      z80_run(system_clock + VDP_CYCLES_PER_LINE);
      if (draw && scan_line < screen_height) {
        if (renderOnCore1) {
          const uint32_t q = renderQueued;
          while (q - renderDone >= MD_RENDER_RING) {}
          gwenesis_vdp_render_capture_slot(scan_line, (int)(q % MD_RENDER_RING));
          renderQueued = q + 1;
        } else {
          gwenesis_vdp_render_capture(scan_line);
          gwenesis_vdp_render_line(scan_line);
        }
      }
      if ((scan_line == 0) || (scan_line > screen_height)) hint_counter = REG10_LINE_COUNTER;
      if (--hint_counter < 0) {
        if ((REG0_LINE_INTERRUPT != 0) && (scan_line <= screen_height)) {
          hint_pending = 1;
          if ((gwenesis_vdp_status & STATUS_VIRQPENDING) == 0) m68k_update_irq(4);
        }
        hint_counter = REG10_LINE_COUNTER;
      }
      scan_line++;
      if (scan_line == screen_height) {
#if !MD_VINT_DELAY
        if (REG1_VBLANK_INTERRUPT != 0) {
          gwenesis_vdp_status |= STATUS_VIRQPENDING;
          m68k_set_irq(6);
        }
#endif
        z80_irq_line(1);
        frameReady = true;
      }
#if !MD_Z80_VINT_HOLD
      if (scan_line == screen_height + 1) z80_irq_line(0);
#endif
      system_clock += VDP_CYCLES_PER_LINE;
      md_emu_clock = md_frame_base + (uint32_t)system_clock;
    }
    m68k.cycles -= system_clock;
    md_frame_base += (uint32_t)system_clock;

#ifdef USE_SERIAL_DEBUG
    int64_t elapsed = esp_timer_get_time() - frameStart;
    if (elapsed < dbgMin) dbgMin = elapsed;
    if (elapsed > dbgMax) dbgMax = elapsed;
    dbgSum += elapsed;
    if (elapsed > frameTimeUs) dbgOverBudget++;
    if (++dbgFrameCnt >= 300) {
      const int64_t wall = esp_timer_get_time() - dbgWinStart;
      Serial.printf("SPEED: %d%% of real | frame us min=%lld max=%lld avg=%lld over=%d/%d (budget %lld) | idle avg=%lldus | skipped %d\n",
                    (int)((int64_t)dbgFrameCnt * frameTimeUs * 100 / (wall > 0 ? wall : 1)),
                    (long long)dbgMin, (long long)dbgMax, (long long)(dbgSum / dbgFrameCnt), dbgOverBudget, dbgFrameCnt,
                    (long long)frameTimeUs, (long long)(dbgIdle / dbgFrameCnt), dbgSkipped);
      dbgMin = INT64_MAX; dbgMax = dbgSum = dbgIdle = 0;
      dbgOverBudget = dbgFrameCnt = dbgSkipped = 0;
    }
#endif

    int64_t now = esp_timer_get_time();
    int64_t waitTime = nextFrameTime - now;
    noWait = waitTime <= 0;
    if (vgaDesc) {
      if (waitTime > 0) {
        vgaWaitVblank(nextFrameTime - frameTimeUs);
        nextFrameTime = esp_timer_get_time();
      } else if (waitTime < -2 * frameTimeUs) {
        nextFrameTime = now;
        gaveUp = true;
      }
    } else if (waitTime > 0) {
      if (waitTime >= 1000) vTaskDelay(pdMS_TO_TICKS(waitTime / 1000));
      while (esp_timer_get_time() < nextFrameTime) asm volatile("nop");
    } else if (waitTime < -2 * frameTimeUs) {
      nextFrameTime = now;
      gaveUp = true;
    }
#ifdef USE_SERIAL_DEBUG
    if (waitTime > 0) dbgIdle += esp_timer_get_time() - now;
#endif
  }
}

#ifndef BOARD_HAS_PSRAM
bool psramEnable() {
  if (esp_spiram_init() != ESP_OK) return false;
  esp_spiram_init_cache();
  if (!esp_spiram_test()) return false;
  if (esp_spiram_add_to_heapalloc() != ESP_OK) return false;
  heap_caps_malloc_extmem_enable(CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL);
  return true;
}
#endif

static uint8_t *allocFast(size_t n, size_t keep, const char *what) {
  uint8_t *p = nullptr;
  if (heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL) >= n + 64 &&
      heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL) >= n + keep)
    p = (uint8_t *)heap_caps_malloc(n, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
  const bool internal = p != nullptr;
  if (!p) p = (uint8_t *)ps_malloc(n);
  Serial.printf("%s: %u bytes in %s\n", what, (unsigned)n, internal ? "internal RAM" : "PSRAM");
  return p;
}

static void haltMessage(const char *l1, const char *l2) {
  Serial.printf("%s %s\n", l1, l2 ? l2 : "");
  canvas->selectFont(&fabgl::FONT_8x8);
  canvas->setPenColor(fabgl::Color::BrightRed);
  canvas->drawText(8, 16, l1);
  if (l2) canvas->drawText(8, 32, l2);
  while (1) delay(1000);
}

void setup() {
  Serial.begin(115200);
#ifndef BOARD_HAS_PSRAM
  Serial.printf("PSRAM: %s\n", psramEnable() ? "enabled by the sketch" : "init FAILED");
#endif
  setCpuFrequencyMhz(240);
  disableCore0WDT();
  disableCore1WDT();
  rtc_wdt_protect_off();
  rtc_wdt_disable();
  esp_task_wdt_delete(NULL);
  delay(100);

  PS2Controller.begin(PS2Preset::KeyboardPort0_MousePort1, fabgl::KbdMode::NoVirtualKeys);
  PS2Controller.keyboard()->begin(GPIO_NUM_33, GPIO_NUM_32, false, false);
  PS2Controller.keyboard()->setLEDs(false, false, false);
  Serial1.begin(115200, SERIAL_8N1, 39, -1);

  DisplayController.begin();
  DisplayController.setResolution(QVGA_320x240_60Hz);
  canvas = new fabgl::Canvas(&DisplayController);
  canvas->setBrushColor(fabgl::Color::Black);
  canvas->clear();

  if (psramFixMissing()) {
    char msg[48];
    snprintf(msg, sizeof(msg), "ESP32 REV.%d NEEDS THE PSRAM BUG FIX", ESP.getChipRevision());
    haltMessage(msg, "TOOLS > PSRAM > \"Enabled\", BUILD AGAIN");
  }

  VRAM = allocFast(0x10000, 16 * 1024, "VRAM");
  ZRAM = allocFast(0x2000, 16 * 1024, "Z80 RAM");
  M68K_RAM = allocFast(0x10000, 40 * 1024, "68000 RAM");
  uint8_t *ring = allocFast(gwenesis_vdp_render_ring_bytes(), 16 * 1024, "render ring");
  if (!VRAM || !ZRAM || !M68K_RAM || !ring) haltMessage("MEMORY ALLOCATION FAILED", nullptr);
  gwenesis_vdp_render_set_ring(ring);
  memset(VRAM, 0, 0x10000);
  memset(ZRAM, 0, 0x2000);
  memset(M68K_RAM, 0, 0x10000);
  Serial.printf("internal RAM free: %u (largest %u), PSRAM free: %u\n",
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

  mdSoundBegin(16384, MD_FRAME_US);
  soundGenerator.attach(&mdWave);
  mdWave.setSampleRate(16384);
  mdWave.enable(true);
  soundGenerator.setVolume(100);
  soundGenerator.play(true);

  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI)) haltMessage("SD CARD NOT FOUND", nullptr);
#if EMU_SWITCH
  emuSwitchCheck("MD");
#endif
  uint32_t romSize = 0;
  uint8_t *rom = nullptr;
  while (!rom) {
    String path = selectRom();
    canvas->setBrushColor(fabgl::Color::Blue);
    canvas->clear();
    canvas->setPenColor(fabgl::Color::White);
    canvas->drawText(8, 16, "LOADING...");
    rom = loadRom(path, &romSize);
    if (!rom) {
      canvas->setPenColor(fabgl::Color::BrightRed);
      canvas->drawText(8, 32, "ROM LOAD ERROR (MAX 28MBIT)");
      delay(1500);
    } else Serial.printf("ROM %s: %u bytes\n", path.c_str(), (unsigned)romSize);
  }

  canvas->setBrushColor(fabgl::Color::Black);
  canvas->clear();
  DisplayController.processPrimitives();
  canvas->waitCompletion();
  for (int y = 0; y < MD_SCREEN_H; y++) {
    vpRows[y] = DisplayController.getScanline(y);
    if (!vpRows[y] || ((uintptr_t)vpRows[y] & 3)) {
      Serial.printf("VIEWPORT row %d is NULL or unaligned (%p)\n", y, vpRows[y]);
      while (1) delay(1000);
    }
  }
  md_sync = DisplayController.createRawPixel(fabgl::RGB222(0, 0, 0)) & 0xC0;
  md_rows = vpRows;
  vgaSyncSetup();

  load_cartridge(rom, romSize);
  power_on();
  reset_emulation();
  Serial.printf("ROM in use: %u bytes\n", md_rom_size);

  renderOnCore1 = xPortGetCoreID() != 0;
  Serial.printf("render on core %d\n", renderOnCore1 ? xPortGetCoreID() : 0);
  Serial.println("MEGA DRIVE Booting...");
#ifndef USE_SERIAL_DEBUG
  Serial.end();
#endif
  xTaskCreatePinnedToCore(cpuTask, "cpuTask", 8192, NULL, 24, NULL, 0);
}

void loop() {
  if (renderOnCore1) {
    static uint32_t lastFill = 0;
    renderService();
    const uint32_t now = ccountNow();
    if ((uint32_t)(now - lastFill) < 240u * 500u) return;
    lastFill = now;
    mdSoundFill();
    if (frameReady || isPaused) {
      frameReady = false;
      updateKeyboard();
      statusTick();
    }
    return;
  }
  mdSoundFill();
  if (frameReady || isPaused) {
    frameReady = false;
    updateKeyboard();
    statusTick();
  }
  vTaskDelay(1);
}
