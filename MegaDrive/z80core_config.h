/* z80core_config.h - Mega Drive: the Z80 of the sound part uses the default memory map and I/O of z80core
   (z->rpage / z->wpage pages, z->read / z->write; z80inst.c).
   2026-11-25: the PSRAM build with the errata fix (BOARD_HAS_PSRAM without EMU_PSRAM_NO_ERRATA_FIX) is short of IRAM:
   ADD / ADC / SUB / SBC / CP are called there instead of inlined (about 3 KB less; the same results) */
#ifndef Z80CORE_CONFIG_H
#define Z80CORE_CONFIG_H

#if defined(BOARD_HAS_PSRAM) && !defined(EMU_PSRAM_NO_ERRATA_FIX)
#define Z80CORE_ALU_CALL 1
#endif

#endif
