// ============================================================================
//  メガドライブの音(説明は md_sound.h)
//  キューの 1 項目 = 時刻(マスタークロック、絶対の下位 32 ビット)+ (番号 << 8 | 値)
//    番号 000h〜1FFh = YM2612 のレジスタ(100h〜 = 裏の 30h〜B6h)、200h = SN76489 への書き込み、
//    3FFh = チップのリセット
//  このスケッチの一部として GPLv3(LICENSE)
// ============================================================================
/* 【最適化】Arduino の ESP32 コアは既定で -Os。このファイルだけ O3 に上げる
   (MegaDrive.ino の #pragma は .ino の翻訳単位にしか効かない)。 */
#pragma GCC optimize("O3")
#if defined(__GNUC__) && defined(ARDUINO)
#pragma GCC optimize("jump-tables")
#endif
#include "md_sound.h"
#include "ym2203.h"
#include <string.h>
#include <stdlib.h>
#ifdef ARDUINO
#include <Arduino.h>
#include <esp_heap_caps.h>
#define MD_SND_FAST IRAM_ATTR
#else
#define MD_SND_FAST
#endif

extern "C" {
int16_t gwenesis_ym2612_buffer[1];
int ym2612_index = 0, ym2612_clock = 0;
short gwenesis_sn76489_buffer[1];
int sn76489_index = 0, sn76489_clock = 0;

volatile uint32_t md_frame_base = 0;
volatile uint32_t md_emu_clock = 0;
volatile bool md_sound_mute = false;
}

static const uint16_t REG_PSG = 0x200, REG_RESET = 0x3FF;
static const int QUEUE = 4096;
static uint32_t *qTime = nullptr;
static uint32_t *qData = nullptr;
static volatile int qHead = 0, qTail = 0;

static void qPush(uint32_t t, uint16_t reg, uint8_t val) {
  int next = (qHead + 1) & (QUEUE - 1);
  if (next == qTail || !qTime) return;
  qTime[qHead] = t;
  qData[qHead] = ((uint32_t)reg << 8) | val;
  qHead = next;
}

static uint16_t ymAddr = 0;
static struct {
  uint16_t na; uint8_t nb, ctrl, status;
  int64_t cntA, cntB;
  uint32_t last;
} tm;
static inline int64_t periodA() { return (int64_t)(1024 - tm.na) * 72 * 14; }
static inline int64_t periodB() { return (int64_t)(256 - tm.nb) * 16 * 72 * 14; }

static void MD_SND_FAST timerAdvance(uint32_t now) {
  const int32_t d = (int32_t)(now - tm.last);
  if (d <= 0) return;
  tm.last = now;
  if (tm.ctrl & 1) {
    tm.cntA -= d;
    while (tm.cntA <= 0) { tm.cntA += periodA(); if (tm.ctrl & 4) tm.status |= 1; }
  }
  if (tm.ctrl & 2) {
    tm.cntB -= d;
    while (tm.cntB <= 0) { tm.cntB += periodB(); if (tm.ctrl & 8) tm.status |= 2; }
  }
}

static void timerWrite(uint8_t reg, uint8_t v) {
  switch (reg) {
    case 0x24: tm.na = (uint16_t)((tm.na & 3) | (v << 2)); break;
    case 0x25: tm.na = (uint16_t)((tm.na & 0x3FC) | (v & 3)); break;
    case 0x26: tm.nb = v; break;
    case 0x27: {
      const uint8_t old = tm.ctrl;
      if ((v & 1) && !(old & 1)) tm.cntA = periodA();
      if ((v & 2) && !(old & 2)) tm.cntB = periodB();
      if (v & 0x10) tm.status &= ~1;
      if (v & 0x20) tm.status &= ~2;
      tm.ctrl = v;
      break;
    }
  }
}

extern "C" {

void mdSoundResetChips(void) {
  memset(&tm, 0, sizeof(tm));
  tm.last = md_frame_base;
  ymAddr = 0;
  qPush(md_frame_base, REG_RESET, 0);
}

void YM2612Init(void) {}
void YM2612Config(unsigned char dac_bits) { (void)dac_bits; }
void YM2612ResetChip(void) { mdSoundResetChips(); }
void ym2612_run(int target) { (void)target; }
void gwenesis_ym2612_save_state(void) {}
void gwenesis_ym2612_load_state(void) {}

void MD_SND_FAST YM2612Write(unsigned int a, unsigned int v, int target) {
  v &= 0xFF;
  switch (a & 3) {
    case 0: ymAddr = (uint16_t)v; return;
    case 2: ymAddr = (uint16_t)(v | 0x100); return;
    default: break;
  }
  const uint32_t t = md_frame_base + (uint32_t)target;
  const uint16_t addr = ymAddr;
  const uint8_t reg = (uint8_t)addr;
  if (addr < 0x100) {
    if (reg >= 0x24 && reg <= 0x27) { timerAdvance(t); timerWrite(reg, (uint8_t)v); qPush(t, addr, (uint8_t)v); return; }
    if (reg == 0x22 || reg == 0x28 || reg == 0x2A || reg == 0x2B || (reg >= 0x30 && reg <= 0xB6)) qPush(t, addr, (uint8_t)v);
  } else if (reg >= 0x30 && reg <= 0xB6) {
    qPush(t, addr, (uint8_t)v);
  }
}

unsigned int MD_SND_FAST YM2612Read(int target) {
  timerAdvance(md_frame_base + (uint32_t)target);
  return tm.status;
}

void gwenesis_SN76489_Init(int PSGClockValue, int SamplingRate, int freq_divisor) { (void)PSGClockValue; (void)SamplingRate; (void)freq_divisor; }
void gwenesis_SN76489_Reset(void) {}
void gwenesis_SN76489_start(void) {}
void gwenesis_SN76489_run(int target) { (void)target; }
void gwenesis_sn76489_save_state(void) {}
void gwenesis_sn76489_load_state(void) {}
void MD_SND_FAST gwenesis_SN76489_Write(int data, int target) {
  qPush(md_frame_base + (uint32_t)target, REG_PSG, (uint8_t)data);
}

}

static Ym2203Fm *fm = nullptr;
static int sndRate = 16384;
static uint32_t clkPerSampleQ16 = 3278u << 16;
static uint32_t sndClock = 0, sndFrac = 0;
static bool sndStarted = false;

static const int BUF = 512;
static int16_t buf[BUF];
static volatile int head = 0, tail = 0;
static int16_t lastSample = 0;
static int32_t fmBuf[64];
static int32_t fmLp = 0, dcPrev = 0, dcOut = 0;
#define FM_TREBLE_FIX 320

static bool dacEn = false;
static int32_t dacOut = 0, dacSum = 0;
static int dacN = 0;

static struct {
  uint32_t tickAccum, tickStep;
  uint16_t tone[3]; uint8_t vol[4], noise, latchCh, latchType;
  int32_t cnt[3]; uint8_t out[3];
  int32_t noiseCnt; uint16_t lfsr; uint8_t noiseOut, lastT2;
} psg;
static const int16_t psgVol[16] = { 24, 23, 22, 20, 17, 15, 12, 11, 8, 7, 5, 3, 2, 1, 1, 0 };
#define PSG_CUTOFF 6
#ifndef MD_PSG_SHIFT
#define MD_PSG_SHIFT 1
#endif

static void psgReset() {
  memset(&psg, 0, sizeof(psg));
  for (int i = 0; i < 4; i++) psg.vol[i] = 15;
  psg.lfsr = 0x4000;
  psg.tickStep = (uint32_t)(((uint64_t)(3579545 / 16) << 16) / (uint32_t)sndRate);
}

static void psgWrite(uint8_t v) {
  if (v & 0x80) {
    psg.latchCh = (v >> 5) & 3;
    psg.latchType = (v >> 4) & 1;
    if (psg.latchType) psg.vol[psg.latchCh] = v & 15;
    else if (psg.latchCh < 3) psg.tone[psg.latchCh] = (uint16_t)((psg.tone[psg.latchCh] & 0x3F0) | (v & 15));
    else { psg.noise = v & 7; psg.lfsr = 0x4000; }
  } else {
    if (psg.latchType) psg.vol[psg.latchCh] = v & 15;
    else if (psg.latchCh < 3) psg.tone[psg.latchCh] = (uint16_t)((psg.tone[psg.latchCh] & 15) | ((v & 0x3F) << 4));
    else { psg.noise = v & 7; psg.lfsr = 0x4000; }
  }
}

static inline int psgSample2() {
  psg.tickAccum += psg.tickStep;
  uint32_t ticks = psg.tickAccum >> 16;
  psg.tickAccum &= 0xFFFF;
  while (ticks--) {
    for (int i = 0; i < 3; i++) {
      if (--psg.cnt[i] <= 0) { psg.cnt[i] = psg.tone[i] ? psg.tone[i] : 1; psg.out[i] ^= 1; }
    }
    bool shift = false;
    const int r = psg.noise & 3;
    if (r == 3) { if (psg.out[2] != psg.lastT2) { shift = true; psg.lastT2 = psg.out[2]; } }
    else if (--psg.noiseCnt <= 0) { psg.noiseCnt = 1 << r; shift = true; }
    if (shift) {
      int fb = (psg.noise & 4) ? ((psg.lfsr & 1) ^ ((psg.lfsr >> 1) & 1)) : (psg.lfsr & 1);
      psg.lfsr = (uint16_t)((psg.lfsr >> 1) | (fb << 14));
      psg.noiseOut = psg.lfsr & 1;
    }
  }
  int s = 0;
  for (int i = 0; i < 3; i++) {
    if (psg.tone[i] <= PSG_CUTOFF) s += psgVol[psg.vol[i]];
    else if (psg.out[i]) s += 2 * psgVol[psg.vol[i]];
  }
  if (psg.noiseOut) s += 2 * psgVol[psg.vol[3]];
  return s;
}

static void chipReset() {
  ym2203Reset(fm);
  ym2203WriteReg(fm, 0x29, 0x80);
  ym2203SetChMute(fm, 0);
  dacEn = false; dacOut = 0; dacSum = 0; dacN = 0;
  psgReset();
}

static void apply(uint32_t e) {
  const uint16_t reg = (uint16_t)(e >> 8);
  const uint8_t v = (uint8_t)e;
  if (reg == REG_PSG) { psgWrite(v); return; }
  if (reg == REG_RESET) { chipReset(); return; }
  if (reg == 0x2A) { dacOut = ((int32_t)v - 0x80) << 6; dacSum += dacOut; dacN++; return; }
  if (reg == 0x2B) { dacEn = (v & 0x80) != 0; ym2203SetChMute(fm, dacEn ? 0x20 : 0); return; }
  ym2203WriteReg(fm, reg, v);
}

extern "C" {

void mdSoundBegin(int rate, uint32_t frameUs) {
  sndRate = rate > 0 ? rate : 16384;
  qTime = (uint32_t *)heap_caps_malloc(sizeof(uint32_t) * QUEUE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  qData = (uint32_t *)heap_caps_malloc(sizeof(uint32_t) * QUEUE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  fm = ym2203Create(MD_MCLK_NTSC / 14, (uint32_t)sndRate);
  clkPerSampleQ16 = (uint32_t)((((uint64_t)MD_MCLK_PER_FRAME * 1000000u) << 16) / ((uint64_t)frameUs * (uint32_t)sndRate));
  chipReset();
}

void mdSoundFill(void) {
  if (!fm || !qTime) return;
  int space = (tail - head - 1 + BUF) % BUF;
  if (md_sound_mute) {
    while (qTail != qHead) { apply(qData[qTail]); qTail = (qTail + 1) & (QUEUE - 1); }
    while (space-- > 0) { buf[head] = 0; head = (head + 1) % BUF; }
    sndStarted = false;
    return;
  }
  const uint32_t frameClk = MD_MCLK_PER_FRAME;
  const uint32_t emu = md_emu_clock;
  if (!sndStarted || (int32_t)(emu - sndClock) > (int32_t)(frameClk + frameClk / 4)) {
    while (qTail != qHead) { apply(qData[qTail]); qTail = (qTail + 1) & (QUEUE - 1); }
    sndClock = emu - frameClk; sndFrac = 0;
    sndStarted = true;
  }
  while (space > 0) {
    int n = space > 64 ? 64 : space;
    while (qTail != qHead && (int32_t)(qTime[qTail] - sndClock) <= 0) {
      apply(qData[qTail]);
      qTail = (qTail + 1) & (QUEUE - 1);
    }
    const uint32_t span = (uint32_t)(((uint64_t)sndFrac + (uint64_t)n * clkPerSampleQ16) >> 16);
    for (int k = qTail; k != qHead; k = (k + 1) & (QUEUE - 1)) {
      const int32_t d = (int32_t)(qTime[k] - sndClock);
      if (d >= (int32_t)span + 1) break;
      if ((qData[k] >> 8) == 0x2A) continue;
      if ((int32_t)(qTime[k] - emu) <= 0) {
        int m = (int)((((uint64_t)d << 16) + clkPerSampleQ16 - 1) / clkPerSampleQ16);
        if (m < n) n = m;
      }
      break;
    }
    const uint32_t c0 = sndClock, f0 = sndFrac;
    uint64_t f = sndFrac + (uint64_t)n * clkPerSampleQ16;
    sndClock += (uint32_t)(f >> 16);
    sndFrac = (uint32_t)(f & 0xFFFF);
    if ((int32_t)(sndClock - emu) > 0) { sndClock = emu; sndFrac = 0; }
    ym2203Generate(fm, fmBuf, n);
    for (int i = 0; i < n; i++) {
      if (i > 0) {
        const uint32_t ti = c0 + (uint32_t)(((uint64_t)f0 + (uint64_t)i * clkPerSampleQ16) >> 16);
        while (qTail != qHead && (qData[qTail] >> 8) == 0x2A && (int32_t)(qTime[qTail] - ti) <= 0) {
          apply(qData[qTail]);
          qTail = (qTail + 1) & (QUEUE - 1);
        }
      }
      int32_t x = fmBuf[i];
      if (dacEn) {
        int32_t dv = dacOut;
        if (dacN > 1) dv = dacSum / dacN;
        x += dv;
      }
      dacSum = 0; dacN = 0;
#if FM_TREBLE_FIX
      fmLp += ((x - fmLp) * FM_TREBLE_FIX) >> 8;
#else
      fmLp = x;
#endif
      int32_t y = ((fmLp >> 8) * 2 + (psgSample2() >> MD_PSG_SHIFT)) * 128;
      dcOut = y - dcPrev + dcOut - (dcOut >> 8);
      dcPrev = y;
      int32_t s = dcOut >> 8;
      if (s > 96) s = 96 + (s - 96) / 3; else if (s < -96) s = -96 + (s + 96) / 3;
      if (s > 127) s = 127; else if (s < -128) s = -128;
      buf[head] = (int16_t)s;
      head = (head + 1) % BUF;
    }
    space -= n;
  }
}

int MD_SND_FAST mdSoundGetSample(void) {
  if (head == tail) return lastSample;
  lastSample = buf[tail];
  tail = (tail + 1) % BUF;
  return lastSample;
}

}
