/* ============================================================================
 *  md_m68k.c - 2026-11-20 MegaDrive for TTGO VGA32: the Mega Drive side of the Musashi 68000 core
 *   - m68k_run(): Gwenesis runs the 68000 up to a time in master clocks (68000 clock x 7)
 *   - interrupt level bits added for the H-INT (m68k_update_irq)
 *   - the wait-loop fast-forward (MD_WAITSKIP)
 *   - save / load of the CPU state (gwenesis_savestate)
 *  GPLv3 (part of this sketch, LICENSE). Musashi's own files (m68k.h, m68kconf.h, m68kcpu.c/.h, m68kops.c/.h) are under the MIT license.
 * ============================================================================ */
#pragma GCC optimize("O3")
#include "m68kcpu.h"
#include "gwenesis_savestate.h"

md_m68k_t m68k;
extern int m68ki_initial_cycles;

static int md_running;
static unsigned int md_base;

#if MD_WAITSKIP
static uint ws_slice;
#endif

void MD_M68K_FAST m68k_run(unsigned int cycles)
{
  if (m68k.cycles >= cycles)
    return;
  m68k.cycle_end = cycles;
#if MD_WAITSKIP
  ws_slice++;
#endif
  const int n = (int)((cycles - m68k.cycles + MD_M68K_MCLK - 1) / MD_M68K_MCLK);
  md_base = m68k.cycles;
  md_running = 1;
  const int used = m68k_execute(n);
  md_running = 0;
  m68k.cycles = md_base + (unsigned int)used * MD_M68K_MCLK;
}

int m68k_cycles_master(void)
{
  if (md_running)
    return (int)(md_base + (unsigned int)(m68ki_initial_cycles - m68ki_remaining_cycles) * MD_M68K_MCLK);
  return (int)m68k.cycles;
}

int md_m68k_cycles_left(void)
{
  return (int)m68k.cycle_end - m68k_cycles_master();
}

void m68k_update_irq(unsigned int mask)
{
  CPU_INT_LEVEL |= mask << 8;
}

#if MD_WAITSKIP
static uint ws_head = 0xFFFFFFFF, ws_br, ws_cyc, ws_iter;
static uint ws_vslice;
static int ws_ok;
static uint ws_areg[8], ws_dreg[8], ws_dmask, ws_chk_dmask;
static uint ws_bad[8];
static int ws_bad_i;

static int ws_code_ok(uint a)
{
  a &= 0xFFFFFF;
  return a < 0x7FFFF0 || (a >> 16) == 0xFF;
}

static int ws_mem(uint addr, int size)
{
  addr &= 0xFFFFFF;
  if (addr < 0x800000) return 0;
  if ((addr >> 16) == 0xFF) return 0;
  if ((addr >> 16) == 0xA0 && (addr & 0xFFFF) + size <= 0x2000) return 0;
  if (addr >= 0xA10000 && addr + size <= 0xA10020) return 0;
  if (addr >= 0xC00004 && addr + size <= 0xC00008 && size <= 2) {
    if (size == 2) return 0x0064;
    return (addr & 1) ? 0x64 : 0x00;
  }
  return -1;
}

typedef struct { int kind; uint val; } ws_ea_t;

static int ws_ea(uint mode, uint reg, int size, uint *pc, ws_ea_t *e)
{
  if (!ws_code_ok(*pc) || !ws_code_ok(*pc + 4)) return 0;
  switch (mode) {
  case 0: e->kind = 0; e->val = reg; return 1;
  case 2: e->kind = 1; e->val = REG_A[reg]; return 1;
  case 5: e->kind = 1; e->val = REG_A[reg] + (uint)(sint16)m68k_read_memory_16(*pc); *pc += 2; return 1;
  case 7:
    switch (reg) {
    case 0: e->kind = 1; e->val = (uint)(sint16)m68k_read_memory_16(*pc); *pc += 2; return 1;
    case 1: e->kind = 1; e->val = (m68k_read_memory_16(*pc) << 16) | m68k_read_memory_16(*pc + 2); *pc += 4; return 1;
    case 2: e->kind = 1; e->val = *pc + (uint)(sint16)m68k_read_memory_16(*pc); *pc += 2; return 1;
    case 4:
      e->kind = 2;
      if (size == 4) { e->val = (m68k_read_memory_16(*pc) << 16) | m68k_read_memory_16(*pc + 2); *pc += 4; }
      else { e->val = m68k_read_memory_16(*pc) & (size == 1 ? 0xFF : 0xFFFF); *pc += 2; }
      return 1;
    }
  }
  return 0;
}

static uint ws_szmask(int size) { return size == 1 ? 0xFF : size == 2 ? 0xFFFF : 0xFFFFFFFF; }

static int __attribute__((noinline)) ws_check(uint head, uint br)
{
  uint modified = 0;
  for (int pass = 0; pass < 2; pass++) {
    const int chk = pass == 1;
    uint written = 0;
    uint taint[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    uint dep = 0;
    uint pc = head;
    for (;;) {
      if (!ws_code_ok(pc)) return 0;
      const uint at = pc;
      const uint op = m68k_read_memory_16(pc); pc += 2;
      ws_ea_t s, im;
      int sz;
      uint t, r;
      if (at == br) {
        if ((op & 0xF000) != 0x6000 || ((op >> 8) & 0xF) == 1 || (op & 0xFF) == 0 || (op & 0xFF) == 0xFF) return 0;
        if (pc + (uint)(sint8)(op & 0xFF) != head) return 0;
        if (((op >> 8) & 0xF) != 0 && dep) return 0;
        break;
      }
      if (at > br) return 0;
#define WS_READ_D(n) do { if (chk && ((modified >> (n)) & 1) && !((written >> (n)) & 1)) return 0; } while (0)
#define WS_SRC(e, size, tv) do { \
        if ((e).kind == 0) { WS_READ_D((e).val); tv = taint[(e).val]; } \
        else if ((e).kind == 1) { int m_ = ws_mem((e).val, size); if (m_ < 0) return 0; tv = (uint)m_; } \
        else tv = 0; } while (0)
      if (op == 0x4E71) continue;
      if ((op & 0xC000) == 0 && (op & 0x3000) && ((op >> 6) & 7) == 0) {
        sz = ((op >> 12) & 3) == 1 ? 1 : ((op >> 12) & 3) == 3 ? 2 : 4;
        if (!ws_ea((op >> 3) & 7, op & 7, sz, &pc, &s)) return 0;
        WS_SRC(s, sz, t);
        r = (op >> 9) & 7;
        modified |= 1u << r; written |= 1u << r;
        taint[r] = (taint[r] & ~ws_szmask(sz)) | (t & ws_szmask(sz));
        dep = t & ws_szmask(sz);
        continue;
      }
      if ((op & 0xFF00) == 0x4A00 && ((op >> 6) & 3) != 3) {
        sz = 1 << ((op >> 6) & 3);
        if (!ws_ea((op >> 3) & 7, op & 7, sz, &pc, &s) || s.kind == 2) return 0;
        WS_SRC(s, sz, t);
        dep = t & ws_szmask(sz);
        continue;
      }
      if ((op & 0xFFC0) == 0x0800 || (op & 0xF1C0) == 0x0100) {
        uint n;
        if ((op & 0xFFC0) == 0x0800) {
          if (!ws_code_ok(pc)) return 0;
          n = m68k_read_memory_16(pc); pc += 2;
        } else {
          r = (op >> 9) & 7;
          if (chk && ((modified >> r) & 1)) return 0;
          n = REG_D[r];
          ws_chk_dmask |= 1u << r;
        }
        if (!ws_ea((op >> 3) & 7, op & 7, 1, &pc, &s) || s.kind == 2) return 0;
        if (s.kind == 0) { WS_READ_D(s.val); dep = taint[s.val] & (1u << (n & 31)); }
        else { int m_ = ws_mem(s.val, 1); if (m_ < 0) return 0; dep = (uint)m_ & (1u << (n & 7)); }
        continue;
      }
      if ((op & 0xFF00) == 0x0C00 && ((op >> 6) & 3) != 3) {
        sz = 1 << ((op >> 6) & 3);
        if (!ws_ea(7, 4, sz, &pc, &im)) return 0;
        if (!ws_ea((op >> 3) & 7, op & 7, sz, &pc, &s) || s.kind == 2) return 0;
        WS_SRC(s, sz, t);
        dep = t & ws_szmask(sz);
        continue;
      }
      if ((op & 0xF100) == 0xB000 && ((op >> 6) & 3) != 3) {
        sz = 1 << ((op >> 6) & 3);
        if (!ws_ea((op >> 3) & 7, op & 7, sz, &pc, &s)) return 0;
        if (s.kind == 1 && ((op >> 3) & 7) == 1) return 0;
        WS_SRC(s, sz, t);
        r = (op >> 9) & 7;
        WS_READ_D(r);
        dep = (t | taint[r]) & ws_szmask(sz);
        continue;
      }
      if ((op & 0xFF38) == 0x0200 && ((op >> 6) & 3) != 3) {
        sz = 1 << ((op >> 6) & 3);
        if (!ws_ea(7, 4, sz, &pc, &im)) return 0;
        r = op & 7;
        if (chk && !((written >> r) & 1)) return 0;
        modified |= 1u << r;
        taint[r] &= im.val | ~ws_szmask(sz);
        dep = taint[r] & ws_szmask(sz);
        continue;
      }
#undef WS_SRC
#undef WS_READ_D
      return 0;
    }
  }
  return 1;
}

void MD_M68K_FAST m68k_waitskip(uint head, uint br)
{
  const uint now = (uint)(m68ki_initial_cycles - m68ki_remaining_cycles);
  if (head != ws_head || br != ws_br) {
    for (int i = 0; i < 8; i++) if (ws_bad[i] == head) return;
    ws_head = head; ws_br = br; ws_cyc = now; ws_iter = 0; ws_ok = 0; ws_vslice = ws_slice;
    return;
  }
  if (ws_vslice != ws_slice) {
    ws_vslice = ws_slice; ws_cyc = now;
    return;
  }
  const uint iter = now - ws_cyc;
  ws_cyc = now;
  if (iter != ws_iter) { ws_iter = iter; return; }
  if (ws_ok) {
    for (int i = 0; i < 8; i++) if (REG_A[i] != ws_areg[i]) ws_ok = 0;
    for (int i = 0; i < 8; i++) if (((ws_dmask >> i) & 1) && REG_D[i] != ws_dreg[i]) ws_ok = 0;
  }
  if (!ws_ok) {
    ws_chk_dmask = 0;
    if (!ws_check(head, br)) { ws_bad[ws_bad_i++ & 7] = head; ws_head = 0xFFFFFFFF; return; }
    ws_ok = 1; ws_dmask = ws_chk_dmask;
    for (int i = 0; i < 8; i++) { ws_areg[i] = REG_A[i]; ws_dreg[i] = REG_D[i]; }
  }
  const uint end = (uint)m68ki_initial_cycles;
  if (iter == 0 || end <= now) return;
  uint k = (end - now) / iter;
  if (k < 2) return;
  k -= 1;
  USE_CYCLES(k * iter);
  ws_cyc += k * iter;
}
#endif

void gwenesis_m68k_save_state(void)
{
  SaveState *state = saveGwenesisStateOpenForWrite("m68k");
  saveGwenesisStateSetBuffer(state, "REG_D", REG_DA, sizeof(m68ki_cpu.dar));
  saveGwenesisStateSet(state, "SR", m68ki_get_sr());
  saveGwenesisStateSet(state, "REG_PC", REG_PC);
  saveGwenesisStateSet(state, "REG_SP", REG_SP);
  saveGwenesisStateSet(state, "REG_USP", REG_USP);
  saveGwenesisStateSet(state, "REG_ISP", REG_ISP);
  saveGwenesisStateSet(state, "REG_IR", REG_IR);
  saveGwenesisStateSet(state, "m68k_cycle_end", m68k.cycle_end);
  saveGwenesisStateSet(state, "m68k_cycles", m68k.cycles);
  saveGwenesisStateSet(state, "m68k_int_level", CPU_INT_LEVEL);
  saveGwenesisStateSet(state, "m68k_stopped", CPU_STOPPED);
}

void gwenesis_m68k_load_state(void)
{
  SaveState *state = saveGwenesisStateOpenForRead("m68k");
  saveGwenesisStateGetBuffer(state, "REG_D", REG_DA, sizeof(m68ki_cpu.dar));
  m68ki_set_sr(saveGwenesisStateGet(state, "SR"));
  REG_PC = saveGwenesisStateGet(state, "REG_PC");
  REG_SP = saveGwenesisStateGet(state, "REG_SP");
  REG_USP = saveGwenesisStateGet(state, "REG_USP");
  REG_ISP = saveGwenesisStateGet(state, "REG_ISP");
  REG_IR = saveGwenesisStateGet(state, "REG_IR");
  m68k.cycle_end = saveGwenesisStateGet(state, "m68k_cycle_end");
  m68k.cycles = saveGwenesisStateGet(state, "m68k_cycles");
  CPU_INT_LEVEL = saveGwenesisStateGet(state, "m68k_int_level");
  CPU_STOPPED = saveGwenesisStateGet(state, "m68k_stopped");
}
