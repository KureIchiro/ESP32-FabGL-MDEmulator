/* ============================================================================
 *  md_m68k.h - 2026-11-20 MegaDrive for TTGO VGA32: the Mega Drive side of the Musashi 68000 core.
 *  Included at the end of m68k.h.
 *   - ROM and 68000 RAM are read / written without a call (the bus functions handle everything else)
 *   - Gwenesis counts time in master clocks (68000 clock x 7): m68k.cycles, m68k_run(), m68k_cycles_master()
 *   - the wait-loop fast-forward (MD_WAITSKIP, md_m68k.c)
 *  GPLv3 (part of this sketch, LICENSE). Musashi's own files (m68k.h, m68kconf.h, m68kcpu.c/.h, m68kops.c/.h) are under the MIT license.
 * ============================================================================ */
#ifndef MD_M68K_H
#define MD_M68K_H

#ifdef ARDUINO
#include <esp_attr.h>
#define MD_M68K_FAST IRAM_ATTR
#define M68K_IRAM IRAM_ATTR
#else
#define MD_M68K_FAST
#define M68K_IRAM
#endif

#define MD_M68K_MCLK 7

#define ROM_SWAP
extern unsigned char *ROM_DATA;
extern unsigned char *M68K_RAM;

#define FETCH8ROM(A)   (ROM_DATA[(A) ^ 1])
#define FETCH16ROM(A)  (*(unsigned short *)&ROM_DATA[(A)])
#define FETCH32ROM(A)  ((*(unsigned int *)&ROM_DATA[(A)] << 16) | (*(unsigned int *)&ROM_DATA[(A)] >> 16))
#define FETCH8RAM(A)   (M68K_RAM[((A) ^ 1) & 0xFFFF])
#define FETCH16RAM(A)  (*(unsigned short *)&M68K_RAM[(A) & 0xFFFF])
#define FETCH32RAM(A)  ((*(unsigned int *)&M68K_RAM[(A) & 0xFFFF] << 16) | (*(unsigned int *)&M68K_RAM[(A) & 0xFFFF] >> 16))
#define WRITE8RAM(A, V)  (M68K_RAM[((A) ^ 1) & 0xFFFF] = (V))
#define WRITE16RAM(A, V) (*(unsigned short *)&M68K_RAM[(A) & 0xFFFF] = (V))
#define WRITE32RAM(A, V) (*(unsigned int *)&M68K_RAM[(A) & 0xFFFF] = (((V) << 16) | ((V) >> 16)))

#define m68k_read_immediate_16(A)  (((A) & 0x800000) ? FETCH16RAM(A) : FETCH16ROM(A))
#define m68k_read_immediate_32(A)  (((A) & 0x800000) ? FETCH32RAM(A) : FETCH32ROM(A))
#define m68k_read_pcrelative_8(A)  (((A) & 0x800000) ? FETCH8RAM(A) : FETCH8ROM(A))
#define m68k_read_pcrelative_16(A) m68k_read_immediate_16(A)
#define m68k_read_pcrelative_32(A) m68k_read_immediate_32(A)

#define MD_READ_8(A)   ((A) < 0x800000 ? FETCH8ROM(A)  : (A) >= 0xFF0000 ? FETCH8RAM(A)  : m68k_read_memory_8(A))
#define MD_READ_16(A)  ((A) < 0x800000 ? FETCH16ROM(A) : (A) >= 0xFF0000 ? FETCH16RAM(A) : m68k_read_memory_16(A))
#define MD_READ_32(A)  ((A) < 0x800000 ? FETCH32ROM(A) : (A) >= 0xFF0000 ? FETCH32RAM(A) : m68k_read_memory_32(A))
#define MD_WRITE_8(A, V)  do { if ((A) >= 0xFF0000) WRITE8RAM(A, V);  else m68k_write_memory_8(A, V); } while (0)
#define MD_WRITE_16(A, V) do { if ((A) >= 0xFF0000) WRITE16RAM(A, V); else m68k_write_memory_16(A, V); } while (0)
#define MD_WRITE_32(A, V) do { if ((A) >= 0xFF0000) WRITE32RAM(A, V); else m68k_write_memory_32(A, V); } while (0)

typedef struct {
  unsigned int cycles;
  unsigned int cycle_end;
} md_m68k_t;
extern md_m68k_t m68k;

void m68k_run(unsigned int cycles);
int m68k_cycles_master(void);
int md_m68k_cycles_left(void);
void m68k_update_irq(unsigned int mask);
void gwenesis_m68k_save_state(void);
void gwenesis_m68k_load_state(void);

#ifndef MD_WAITSKIP
#define MD_WAITSKIP 1
#endif
#define MD_WAITSKIP_LEN 24
void m68k_waitskip(unsigned int head, unsigned int br);

#endif
