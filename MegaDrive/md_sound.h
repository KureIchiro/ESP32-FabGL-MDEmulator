#pragma once
#include <stdint.h>

#define MD_MCLK_NTSC 53693175u
#define MD_MCLK_PER_FRAME 896040u

#ifdef __cplusplus
extern "C" {
#endif
extern volatile uint32_t md_frame_base;
extern volatile uint32_t md_emu_clock;
void mdSoundResetChips(void);

void mdSoundBegin(int rate, uint32_t frameUs);
void mdSoundFill(void);
int  mdSoundGetSample(void);
extern volatile bool md_sound_mute;
#ifdef __cplusplus
}
#endif
