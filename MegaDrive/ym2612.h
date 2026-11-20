/* ym2612.h - 2026-11-20 (GPLv3 section 5a) MegaDrive for TTGO VGA32: the YM2612 functions Gwenesis calls.
   The sound is made by md_sound.cpp (with the FM of ym2203.cpp); this header only declares them. */
#ifndef _H_YM2612_
#define _H_YM2612_

#include <stdint.h>

extern int16_t gwenesis_ym2612_buffer[];
extern int ym2612_index;
extern int ym2612_clock;

void YM2612Init(void);
void YM2612Config(unsigned char dac_bits);
void YM2612ResetChip(void);
void YM2612Write(unsigned int a, unsigned int v, int target);
void ym2612_run(int target);
unsigned int YM2612Read(int target);
void gwenesis_ym2612_save_state(void);
void gwenesis_ym2612_load_state(void);

#endif
