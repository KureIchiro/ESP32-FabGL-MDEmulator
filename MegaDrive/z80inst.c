/*
Gwenesis : Genesis & megadrive Emulator.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.
This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.

__author__ = "bzhxx"
__contact__ = "https://github.com/bzhxx"
__license__ = "GPLv3"

*/
#pragma GCC optimize("O3")
#if defined(__GNUC__) && defined(ARDUINO)
#pragma GCC optimize("jump-tables")
#endif
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "z80core.h"   /* 2026-11-20 (GPLv3 section 5a) MegaDrive for TTGO VGA32: the Z80 core (MIT) */
#include "z80inst.h"
#include "m68k.h"
#include "gwenesis_bus.h"
#include "ym2612.h"
#include "gwenesis_sn76489.h"
#include "gwenesis_savestate.h"

#if GNW_TARGET_MARIO !=0 || GNW_TARGET_ZELDA!=0
  #pragma GCC optimize("Ofast")
#endif

/* 2026-11-12 (GPLv3 section 5a) MegaDrive for TTGO VGA32: hot functions run from internal RAM (IRAM) */
#ifdef ARDUINO
#include <esp_attr.h>
#define MD_FAST IRAM_ATTR
#else
#define MD_FAST
#endif
static int bus_ack = 0;
static int reset = 0;
static int reset_once = 0;
int zclk = 0;
static int initialized = 0;

unsigned char *Z80_RAM;

static z80_t cpu;
static uint8_t md_z80_read(z80_t *z, uint16_t Addr);
static void md_z80_write(z80_t *z, uint16_t Addr, uint8_t Value);

/* 2026-11-20 (GPLv3 section 5a) MegaDrive for TTGO VGA32: the Z80 RAM (0000h-1FFFh and its mirror 2000h-3FFFh) is
   read and written by the core directly; everything else goes through md_z80_read / md_z80_write */
static void md_z80_pages(void)
{
  for (int i = 0; i < 8; i++) cpu.rpage[i] = cpu.wpage[i] = 0;
  if (Z80_RAM) cpu.rpage[0] = cpu.rpage[1] = cpu.wpage[0] = cpu.wpage[1] = Z80_RAM;
}

#define Z80_INST_DISABLE_LOGGING 1

#if !Z80_INST_DISABLE_LOGGING
#include <stdarg.h>
void z80_log(const char *subs, const char *fmt, ...) {
  extern int frame_counter;
  extern int scan_line;

  va_list va;

  printf("%06d:%03d :[%s]", frame_counter, scan_line, subs);

  va_start(va, fmt);
  vfprintf(stdout, fmt, va);
  va_end(va);
  printf("\n");
}
#else
	#define z80_log(...)  do {} while(0)
#endif

// Bank register used by Z80 to access M68K Memory space 1 BANK=32KByte
int Z80_BANK;


void z80_start() {
    z80core_init(&cpu);
    cpu.read = md_z80_read;
    cpu.write = md_z80_write;
    md_z80_pages();
    cpu.irq_autoclear = MD_Z80_VINT_HOLD;
    reset=1;
    reset_once=0;
    bus_ack=0;
    zclk=0;
}

void z80_pulse_reset() {
  z80core_reset(&cpu);
}
static int current_timeslice = 0;

void MD_FAST z80_run(int target) {

  // we are in advance,nothind to do
current_timeslice = 0;
  if (zclk >= target) {
 // z80_log("z80_skip time","%1d%1d%1d||zclk=%d,tgt=%d",reset_once,bus_ack,reset, zclk, target);
    return;
  }

  current_timeslice = target - zclk;

  int rem = 0;
  if ((reset_once == 1) && (bus_ack == 0) && (reset == 0)) {

   // z80_log("z80_run", "%1d%1d%1d||zclk=%d,tgt=%d",reset_once, bus_ack, reset, zclk, target);
    rem = z80core_run(&cpu, current_timeslice / Z80_FREQ_DIVISOR);

  }

  zclk = target - rem * Z80_FREQ_DIVISOR;
}

void z80_sync(void) {
  /*
  get M68K cycles 
  Execute cycles on z80 to sync with m68K
  */

  z80_run(m68k_cycles_master());
}

void z80_set_memory(unsigned char *buffer)
{
    Z80_RAM = buffer;
    md_z80_pages();
    initialized = 1;
}

void z80_write_ctrl(unsigned int address, unsigned int value) {
  z80_sync();

  if (address == 0x1100) // BUSREQ
  {
    z80_log(__FUNCTION__,"BUSREQ = %d, current=%d", value,bus_ack);

    // Bus request. Z80 bus on hold.
    if (value) {
      bus_ack = 1;


    // Bus request cancel. Z80 runs.
    } else {
            bus_ack = 0;
    }

  } else if (address == 0x1200) // RESET
  {
    z80_log(__FUNCTION__,"RESET = %d, current=%d", value,reset);
  
    if (value == 0) {
      reset = 1;
    } else if (reset || !reset_once) {
      /* 2026-11-16 (GPLv3 section 5a) MegaDrive for TTGO VGA32: the Z80 is reset only when the reset is
         released (it was reset on every write of 1; some sound drivers write 1 again while polling the Z80) */

      z80_pulse_reset();
      reset = 0;
      reset_once = 1;
    }
  }
}

unsigned int z80_read_ctrl(unsigned int address) {

  z80_sync();

  if (address == 0x1100) {

    z80_log(__FUNCTION__,"RUNNING = %d ", bus_ack ? 0 : 1);
    return bus_ack == 1 ? 0 : 1;

  } else if (address == 0x1101) {
    return 0x00;

  } else if (address == 0x1200) {

    z80_log(__FUNCTION__,"RESET = %d ", reset );
    return reset;

  } else if (address == 0x1201) {
    return 0x00;
  }
  return 0xFF;
}

void z80_irq_line(unsigned int value)
{
    if (reset_once == 0) return;

    if (value)
        cpu.irq = 1;
    else
        cpu.irq = 0;

    z80_log(__FUNCTION__,"Interrupt = %d ", value);

}


/********************************************
 * Z80 Bank
 ********************************************/

unsigned int zbankreg_mem_r8(unsigned int address)
{
      z80_log(__FUNCTION__,"Z80 bank read pointer : %06x", Z80_BANK);

    return Z80_BANK;
}

static inline void zbankreg_mem_w8(unsigned int value) {
  Z80_BANK >>= 1;
  Z80_BANK |= (value & 1) << 8;
  z80_log(__FUNCTION__,"Z80 bank points to: %06x", Z80_BANK << 15);
  return;
}

static inline unsigned int zbank_mem_r8(unsigned int address)
{
    address &= 0x7FFF;
    address |= (Z80_BANK << 15);

    z80_log(__FUNCTION__,"Z80 bank read: %06x", address);
    return m68k_read_memory_8(address);
}

static inline void zbank_mem_w8(unsigned int address, unsigned int value) {
  address &= 0x7FFF;
  address |= (Z80_BANK << 15);

  z80_log(__FUNCTION__,"Z80 bank write %06x: %02x", address, value);
  m68k_write_memory_8(address, value);

}

// TODO ??
/*
unsigned int zvdp_mem_r8(unsigned int address)
{
    if (address >= 0x7F00 && address < 0x7F20)
        return vdp_mem_r8(address);
    return 0xFF;
}

void zvdp_mem_w8(unsigned int address, unsigned int value)
{
    if (address >= 0x7F00 && address < 0x7F20)
        vdp_mem_w8(address, value);
}

*/

static uint8_t MD_FAST md_z80_read(z80_t *z, uint16_t Addr) {
  (void)z;

  if (Addr < 0x4000)
    return Z80_RAM[Addr & 0x1FFF];

  if (Addr < 0x6000)
    return YM2612Read(zclk + current_timeslice - (cpu.icount * Z80_FREQ_DIVISOR));

  z80_log(__FUNCTION__, "addr= %x", Addr);

  if (Addr >= 0x8000)
    return zbank_mem_r8(Addr);

  z80_log(__FUNCTION__, "addr= %x", Addr);

  return 0xFF;
}

extern int system_clock;

static void MD_FAST md_z80_write(z80_t *z, uint16_t Addr, uint8_t Value) {
  (void)z;

  // ZRAM & mirror
  if (Addr < 0x4000) {
    Z80_RAM[Addr&0x1FFF] = Value;
    return;
  }

  // @4000-4003
  if (Addr < 0x6000) {
    z80_log("Z80","ZZYM(%x,%x) zk=%d,tgt=%d",Addr&0x3,Value, zclk, zclk + current_timeslice -(cpu.icount * Z80_FREQ_DIVISOR) );
    YM2612Write(Addr&0x3, Value, zclk + current_timeslice -(cpu.icount * Z80_FREQ_DIVISOR) );
    return;
  }

  // @6000
  if (Addr == 0x6000) {
    zbankreg_mem_w8(Value);
    return;
  }

  // @7F11
  if (Addr ==  0x7F11) {
    z80_log("Z80","ZZSN zk=%d,tgt=%d", zclk, zclk + current_timeslice -(cpu.icount * Z80_FREQ_DIVISOR) );
    gwenesis_SN76489_Write(Value,zclk + current_timeslice -(cpu.icount * Z80_FREQ_DIVISOR) );
    return;
  }
 
  z80_log("Z80","WrZ80  %x %x", Addr, Value);

  if (Addr >= 0x8000) {
    zbank_mem_w8(Addr, Value);
    return;
  }
  z80_log("Z80","WrZ80  %x %x", Addr, Value);

}



void gwenesis_z80inst_save_state() {
    SaveState* state;
    state = saveGwenesisStateOpenForWrite("z80inst");
    saveGwenesisStateSetBuffer(state, "cpu", &cpu, z80_state_size);
    saveGwenesisStateSet(state, "bus_ack", bus_ack);
    saveGwenesisStateSet(state, "reset", reset);
    saveGwenesisStateSet(state, "reset_once", reset_once);
    saveGwenesisStateSet(state, "zclk", zclk);
    saveGwenesisStateSet(state, "initialized", initialized);
    saveGwenesisStateSet(state, "Z80_BANK", Z80_BANK);
    saveGwenesisStateSet(state, "current_timeslice",current_timeslice);
}

void gwenesis_z80inst_load_state() {
    SaveState* state = saveGwenesisStateOpenForRead("z80inst");
    saveGwenesisStateGetBuffer(state, "cpu", &cpu, z80_state_size);
    cpu.irq_autoclear = MD_Z80_VINT_HOLD;
    bus_ack = saveGwenesisStateGet(state, "bus_ack");
    reset = saveGwenesisStateGet(state, "reset");
    reset_once = saveGwenesisStateGet(state, "reset_once");
    zclk = saveGwenesisStateGet(state, "zclk");
    initialized = saveGwenesisStateGet(state, "initialized");
    Z80_BANK = saveGwenesisStateGet(state, "Z80_BANK");
    current_timeslice = saveGwenesisStateGet(state, "current_timeslice");

}
