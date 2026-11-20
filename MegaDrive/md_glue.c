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

SaveState *saveGwenesisStateOpenForRead(const char *f) { (void)f; return (SaveState *)1; }
SaveState *saveGwenesisStateOpenForWrite(const char *f) { (void)f; return (SaveState *)1; }
int saveGwenesisStateGet(SaveState *s, const char *tag) { (void)s; (void)tag; return 0; }
void saveGwenesisStateSet(SaveState *s, const char *tag, int val) { (void)s; (void)tag; (void)val; }
void saveGwenesisStateGetBuffer(SaveState *s, const char *tag, void *buf, int len) { (void)s; (void)tag; (void)buf; (void)len; }
void saveGwenesisStateSetBuffer(SaveState *s, const char *tag, void *buf, int len) { (void)s; (void)tag; (void)buf; (void)len; }

void gwenesis_io_get_buttons(void) {}
