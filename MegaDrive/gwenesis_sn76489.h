/* gwenesis_sn76489.h - 2026-11-20 (GPLv3 section 5a) MegaDrive for TTGO VGA32: the SN76489 (PSG) functions
   Gwenesis calls. The sound is made by md_sound.cpp; this header only declares them. */
#ifndef _GWENESIS_SN76489_H_
#define _GWENESIS_SN76489_H_

extern short gwenesis_sn76489_buffer[];
extern int sn76489_index;
extern int sn76489_clock;

void gwenesis_SN76489_Init(int PSGClockValue, int SamplingRate, int freq_divisor);
void gwenesis_SN76489_Reset(void);
void gwenesis_SN76489_start(void);
void gwenesis_SN76489_Write(int data, int target);
void gwenesis_SN76489_run(int target);
void gwenesis_sn76489_save_state(void);
void gwenesis_sn76489_load_state(void);

#endif /* _GWENESIS_SN76489_H_ */
