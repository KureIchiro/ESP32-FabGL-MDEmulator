/* ============================================================================
 *  Gwenesis が呼ぶ関数で、メガドライブ版では何もしないもの(Retro-Gamer の genesis_bridge.c にあったもの)
 *  (.ino に書くと Arduino が足す関数の宣言が extern "C" の中に入ってしまうので、C のファイルに分けた)
 *  このスケッチの一部として GPLv3(LICENSE)
 * ============================================================================ */
/* 【最適化】Arduino の ESP32 コアは既定で -Os。このファイルだけ O3 に上げる
   (MegaDrive.ino の #pragma は .ino の翻訳単位にしか効かない)。 */
#pragma GCC optimize("O3")
#include <stdint.h>
#include "gwenesis_savestate.h"

#include <string.h>
#include <stdlib.h>
#ifdef ARDUINO
#include <esp_heap_caps.h>
#endif
#define SS_SEC 16
#define SS_TAG 32
#define SS_HDR 8
#define SS_CAP (256 * 1024)
static const uint8_t ss_magic[SS_HDR] = { 'M', 'D', 'S', 'T', 1, 0, 0, 0 };
static uint8_t *ss_buf;
static size_t ss_len;
static char ss_sec[SS_SEC];
static int ss_err;

static int ss_alloc(void) {
  if (ss_buf) return 1;
#ifdef ARDUINO
  ss_buf = (uint8_t *)heap_caps_malloc(SS_CAP, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
  ss_buf = (uint8_t *)malloc(SS_CAP);
#endif
  return ss_buf != NULL;
}
static void ss_section(const char *f) { memset(ss_sec, 0, SS_SEC); strncpy(ss_sec, f, SS_SEC - 1); }
SaveState *saveGwenesisStateOpenForRead(const char *f) { ss_section(f); return (SaveState *)1; }
SaveState *saveGwenesisStateOpenForWrite(const char *f) { ss_section(f); return (SaveState *)1; }

static void ss_put(const char *tag, const void *data, int len) {
  if (len < 0 || ss_len + SS_SEC + SS_TAG + 4 + (size_t)len > SS_CAP) { ss_err = 1; return; }
  memcpy(ss_buf + ss_len, ss_sec, SS_SEC); ss_len += SS_SEC;
  memset(ss_buf + ss_len, 0, SS_TAG); strncpy((char *)ss_buf + ss_len, tag, SS_TAG - 1); ss_len += SS_TAG;
  memcpy(ss_buf + ss_len, &len, 4); ss_len += 4;
  memcpy(ss_buf + ss_len, data, (size_t)len); ss_len += (size_t)len;
}
static const uint8_t *ss_find(const char *tag, int *len) {
  char t[SS_TAG];
  memset(t, 0, SS_TAG); strncpy(t, tag, SS_TAG - 1);
  size_t p = SS_HDR;
  while (p + SS_SEC + SS_TAG + 4 <= ss_len) {
    int l;
    memcpy(&l, ss_buf + p + SS_SEC + SS_TAG, 4);
    if (l < 0 || p + SS_SEC + SS_TAG + 4 + (size_t)l > ss_len) break;
    if (!memcmp(ss_buf + p, ss_sec, SS_SEC) && !memcmp(ss_buf + p + SS_SEC, t, SS_TAG)) { *len = l; return ss_buf + p + SS_SEC + SS_TAG + 4; }
    p += SS_SEC + SS_TAG + 4 + (size_t)l;
  }
  ss_err = 1;
  return NULL;
}
void saveGwenesisStateSet(SaveState *s, const char *tag, int val) { (void)s; ss_put(tag, &val, 4); }
void saveGwenesisStateSetBuffer(SaveState *s, const char *tag, void *buf, int len) { (void)s; ss_put(tag, buf, len); }
int saveGwenesisStateGet(SaveState *s, const char *tag) {
  (void)s;
  int l, v = 0;
  const uint8_t *p = ss_find(tag, &l);
  if (p && l == 4) memcpy(&v, p, 4);
  return v;
}
void saveGwenesisStateGetBuffer(SaveState *s, const char *tag, void *buf, int len) {
  (void)s;
  int l;
  const uint8_t *p = ss_find(tag, &l);
  if (p) memcpy(buf, p, (size_t)(l < len ? l : len));
}

const uint8_t *md_state_save(size_t *len) {
  if (!ss_alloc()) return NULL;
  memcpy(ss_buf, ss_magic, SS_HDR);
  ss_len = SS_HDR;
  ss_err = 0;
  gwenesis_save_state();
  if (ss_err) return NULL;
  *len = ss_len;
  return ss_buf;
}
uint8_t *md_state_buffer(size_t *cap) {
  if (!ss_alloc()) return NULL;
  *cap = SS_CAP;
  return ss_buf;
}
int md_state_load(size_t len) {
  if (!ss_buf || len < SS_HDR || len > SS_CAP || memcmp(ss_buf, ss_magic, SS_HDR) != 0) return -1;
  ss_len = len;
  ss_err = 0;
  gwenesis_load_state();
  return 0;
}

void gwenesis_io_get_buttons(void) {}
