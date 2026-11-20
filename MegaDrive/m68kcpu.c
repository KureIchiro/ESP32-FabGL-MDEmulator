/* ======================================================================== */
/* ======================================================================== */



/* ======================================================================== */
/* ======================================================================== */



/* ======================================================================== */
/* ================================ INCLUDES ============================== */
/* ======================================================================== */

extern void m68040_fpu_op0(void);
extern void m68040_fpu_op1(void);
extern void m68881_mmu_ops(void);
extern const unsigned char m68ki_cycles[][0x10000];
extern void (*const m68ki_instruction_jump_table[0x10000])(void);
extern void m68ki_build_opcode_table(void);

#include "m68kops.h"
#include "m68kcpu.h"

#if MD_M68K_FPU
#include "m68kfpu.c"
#include "m68kmmu.h"
#endif

/* ======================================================================== */
/* ================================= DATA ================================= */
/* ======================================================================== */

int  m68ki_initial_cycles;
int  m68ki_remaining_cycles = 0;
uint m68ki_tracing = 0;
uint m68ki_address_space;

#ifdef M68K_LOG_ENABLE
const char *const m68ki_cpu_names[] =
{
	"Invalid CPU",
	"M68000",
	"M68010",
	"M68EC020",
	"M68020",
	"M68EC030",
	"M68030",
	"M68EC040",
	"M68LC040",
	"M68040",
	"SCC68070",
};
#endif

m68ki_cpu_core m68ki_cpu = {0};

#if M68K_EMULATE_ADDRESS_ERROR
#ifdef _BSD_SETJMP_H
sigjmp_buf m68ki_aerr_trap;
#else
jmp_buf m68ki_aerr_trap;
#endif
#endif

uint    m68ki_aerr_address;
uint    m68ki_aerr_write_mode;
uint    m68ki_aerr_fc;

jmp_buf m68ki_bus_error_jmp_buf;

const uint8 m68ki_shift_8_table[65] =
{
	0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff
};
const uint16 m68ki_shift_16_table[65] =
{
	0x0000, 0x8000, 0xc000, 0xe000, 0xf000, 0xf800, 0xfc00, 0xfe00, 0xff00,
	0xff80, 0xffc0, 0xffe0, 0xfff0, 0xfff8, 0xfffc, 0xfffe, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff
};
const uint m68ki_shift_32_table[65] =
{
	0x00000000, 0x80000000, 0xc0000000, 0xe0000000, 0xf0000000, 0xf8000000,
	0xfc000000, 0xfe000000, 0xff000000, 0xff800000, 0xffc00000, 0xffe00000,
	0xfff00000, 0xfff80000, 0xfffc0000, 0xfffe0000, 0xffff0000, 0xffff8000,
	0xffffc000, 0xffffe000, 0xfffff000, 0xfffff800, 0xfffffc00, 0xfffffe00,
	0xffffff00, 0xffffff80, 0xffffffc0, 0xffffffe0, 0xfffffff0, 0xfffffff8,
	0xfffffffc, 0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
};


const uint8 m68ki_exception_cycle_table[5][256] =
{
	{
		 40,
		  4,
		 50,
		 50,
		 34,
		 38,
		 40,
		 34,
		 34,
		 34,
		 34,
		 34,
		  4,
		  4,
		  4,
		 44,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		 44,
		 44,
		 44,
		 44,
		 44,
		 44,
		 44,
		 44,
		 34,
		 34,
		 34,
		 34,
		 34,
		 34,
		 34,
		 34,
		 34,
		 34,
		 34,
		 34,
		 34,
		 34,
		 34,
		 34,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
	},
	{
		 40,
		  4,
		126,
		126,
		 38,
		 44,
		 44,
		 34,
		 38,
		 38,
		  4,
		  4,
		  4,
		  4,
		  4,
		 44,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		 46,
		 46,
		 46,
		 46,
		 46,
		 46,
		 46,
		 46,
		 38,
		 38,
		 38,
		 38,
		 38,
		 38,
		 38,
		 38,
		 38,
		 38,
		 38,
		 38,
		 38,
		 38,
		 38,
		 38,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
	},
	{
		  4,
		  4,
		 50,
		 50,
		 20,
		 38,
		 40,
		 20,
		 34,
		 25,
		 20,
		 20,
		  4,
		  4,
		  4,
		 30,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		 30,
		 30,
		 30,
		 30,
		 30,
		 30,
		 30,
		 30,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
	},
	{
		  4,
		  4,
		 50,
		 50,
		 20,
		 38,
		 40,
		 20,
		 34,
		 25,
		 20,
		 20,
		  4,
		  4,
		  4,
		 30,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		 30,
		 30,
		 30,
		 30,
		 30,
		 30,
		 30,
		 30,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
	},
	{
		  4,
		  4,
		 50,
		 50,
		 20,
		 38,
		 40,
		 20,
		 34,
		 25,
		 20,
		 20,
		  4,
		  4,
		  4,
		 30,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		 30,
		 30,
		 30,
		 30,
		 30,
		 30,
		 30,
		 30,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		 20,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
	}
};

const uint8 m68ki_ea_idx_cycle_table[64] =
{
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,
	 5,
	 7,
	 7,
	 0,  5,  7,  7,  0,  5,  7,  7,  0,  5,  7,  7,
	 2,
	 7,
	 9,
	 9,
	 0,  7,  9,  9,  0,  7,  9,  9,  0,  7,  9,  9,
	 6,
	11,
	13,
	13,
	 0, 11, 13, 13,  0, 11, 13, 13,  0, 11, 13, 13
};



/* ======================================================================== */
/* =============================== CALLBACKS ============================== */
/* ======================================================================== */

/* Default callbacks used if the callback hasn't been set yet, or if the
 * callback is set to NULL
 */

/* Interrupt acknowledge */
static int default_int_ack_callback_data;
static int default_int_ack_callback(int int_level)
{
	default_int_ack_callback_data = int_level;
	CPU_INT_LEVEL = 0;
	return M68K_INT_ACK_AUTOVECTOR;
}

static unsigned int default_bkpt_ack_callback_data;
static void default_bkpt_ack_callback(unsigned int data)
{
	default_bkpt_ack_callback_data = data;
}

/* Called when a reset instruction is executed */
static void default_reset_instr_callback(void)
{
}

static void default_cmpild_instr_callback(unsigned int val, int reg)
{
	(void)val;
	(void)reg;
}

static void default_rte_instr_callback(void)
{
}

/* Called when a tas instruction is executed */
static int default_tas_instr_callback(void)
{
	return 1; // allow writeback
}

static int default_illg_instr_callback(int opcode)
{
	(void)opcode;
	return 0;
}

static int default_trap_instr_callback(int trap)
{
    (void)trap;
    return 0;
}

static unsigned int default_pc_changed_callback_data;
static void default_pc_changed_callback(unsigned int new_pc)
{
	default_pc_changed_callback_data = new_pc;
}

/* Called every time there's bus activity (read/write to/from memory */
static unsigned int default_set_fc_callback_data;
static void default_set_fc_callback(unsigned int new_fc)
{
	default_set_fc_callback_data = new_fc;
}

static void default_instr_hook_callback(unsigned int pc)
{
	(void)pc;
}


#if M68K_EMULATE_ADDRESS_ERROR
	#include <setjmp.h>
	#ifdef _BSD_SETJMP_H
	sigjmp_buf m68ki_aerr_trap;
	#else
	jmp_buf m68ki_aerr_trap;
	#endif
#endif

/* ======================================================================== */
/* ================================= API ================================== */
/* ======================================================================== */

/* Access the internals of the CPU */
unsigned int m68k_get_reg(void* context, m68k_register_t regnum)
{
	m68ki_cpu_core* cpu = context != NULL ?(m68ki_cpu_core*)context : &m68ki_cpu;

	switch(regnum)
	{
		case M68K_REG_D0:	return cpu->dar[0];
		case M68K_REG_D1:	return cpu->dar[1];
		case M68K_REG_D2:	return cpu->dar[2];
		case M68K_REG_D3:	return cpu->dar[3];
		case M68K_REG_D4:	return cpu->dar[4];
		case M68K_REG_D5:	return cpu->dar[5];
		case M68K_REG_D6:	return cpu->dar[6];
		case M68K_REG_D7:	return cpu->dar[7];
		case M68K_REG_A0:	return cpu->dar[8];
		case M68K_REG_A1:	return cpu->dar[9];
		case M68K_REG_A2:	return cpu->dar[10];
		case M68K_REG_A3:	return cpu->dar[11];
		case M68K_REG_A4:	return cpu->dar[12];
		case M68K_REG_A5:	return cpu->dar[13];
		case M68K_REG_A6:	return cpu->dar[14];
		case M68K_REG_A7:	return cpu->dar[15];
		case M68K_REG_PC:	return MASK_OUT_ABOVE_32(cpu->pc);
		case M68K_REG_SR:	return	cpu->t1_flag						|
									cpu->t0_flag						|
									(cpu->s_flag << 11)					|
									(cpu->m_flag << 11)					|
									cpu->int_mask						|
									((cpu->x_flag & XFLAG_SET) >> 4)	|
									((cpu->n_flag & NFLAG_SET) >> 4)	|
									((!cpu->not_z_flag) << 2)			|
									((cpu->v_flag & VFLAG_SET) >> 6)	|
									((cpu->c_flag & CFLAG_SET) >> 8);
		case M68K_REG_SP:	return cpu->dar[15];
		case M68K_REG_USP:	return cpu->s_flag ? cpu->sp[0] : cpu->dar[15];
		case M68K_REG_ISP:	return cpu->s_flag && !cpu->m_flag ? cpu->dar[15] : cpu->sp[4];
		case M68K_REG_MSP:	return cpu->s_flag && cpu->m_flag ? cpu->dar[15] : cpu->sp[6];
		case M68K_REG_SFC:	return cpu->sfc;
		case M68K_REG_DFC:	return cpu->dfc;
		case M68K_REG_VBR:	return cpu->vbr;
		case M68K_REG_CACR:	return cpu->cacr;
		case M68K_REG_CAAR:	return cpu->caar;
		case M68K_REG_PREF_ADDR:	return cpu->pref_addr;
		case M68K_REG_PREF_DATA:	return cpu->pref_data;
		case M68K_REG_PPC:	return MASK_OUT_ABOVE_32(cpu->ppc);
		case M68K_REG_IR:	return cpu->ir;
		case M68K_REG_CPU_TYPE:
			switch(cpu->cpu_type)
			{
				case CPU_TYPE_000:		return (unsigned int)M68K_CPU_TYPE_68000;
				case CPU_TYPE_010:		return (unsigned int)M68K_CPU_TYPE_68010;
				case CPU_TYPE_EC020:	return (unsigned int)M68K_CPU_TYPE_68EC020;
				case CPU_TYPE_020:		return (unsigned int)M68K_CPU_TYPE_68020;
				case CPU_TYPE_040:		return (unsigned int)M68K_CPU_TYPE_68040;
			}
			return M68K_CPU_TYPE_INVALID;
		default:			return 0;
	}
	return 0;
}

void m68k_set_reg(m68k_register_t regnum, unsigned int value)
{
	switch(regnum)
	{
		case M68K_REG_D0:	REG_D[0] = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_D1:	REG_D[1] = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_D2:	REG_D[2] = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_D3:	REG_D[3] = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_D4:	REG_D[4] = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_D5:	REG_D[5] = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_D6:	REG_D[6] = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_D7:	REG_D[7] = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_A0:	REG_A[0] = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_A1:	REG_A[1] = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_A2:	REG_A[2] = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_A3:	REG_A[3] = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_A4:	REG_A[4] = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_A5:	REG_A[5] = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_A6:	REG_A[6] = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_A7:	REG_A[7] = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_PC:	m68ki_jump(MASK_OUT_ABOVE_32(value)); return;
		case M68K_REG_SR:	m68ki_set_sr_noint_nosp(value); return;
		case M68K_REG_SP:	REG_SP = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_USP:	if(FLAG_S)
								REG_USP = MASK_OUT_ABOVE_32(value);
							else
								REG_SP = MASK_OUT_ABOVE_32(value);
							return;
		case M68K_REG_ISP:	if(FLAG_S && !FLAG_M)
								REG_SP = MASK_OUT_ABOVE_32(value);
							else
								REG_ISP = MASK_OUT_ABOVE_32(value);
							return;
		case M68K_REG_MSP:	if(FLAG_S && FLAG_M)
								REG_SP = MASK_OUT_ABOVE_32(value);
							else
								REG_MSP = MASK_OUT_ABOVE_32(value);
							return;
		case M68K_REG_VBR:	REG_VBR = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_SFC:	REG_SFC = value & 7; return;
		case M68K_REG_DFC:	REG_DFC = value & 7; return;
		case M68K_REG_CACR:	REG_CACR = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_CAAR:	REG_CAAR = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_PPC:	REG_PPC = MASK_OUT_ABOVE_32(value); return;
		case M68K_REG_IR:	REG_IR = MASK_OUT_ABOVE_16(value); return;
		case M68K_REG_CPU_TYPE: m68k_set_cpu_type(value); return;
		default:			return;
	}
}

/* Set the callbacks */
void m68k_set_int_ack_callback(int  (*callback)(int int_level))
{
	CALLBACK_INT_ACK = callback ? callback : default_int_ack_callback;
}

void m68k_set_bkpt_ack_callback(void  (*callback)(unsigned int data))
{
	CALLBACK_BKPT_ACK = callback ? callback : default_bkpt_ack_callback;
}

void m68k_set_reset_instr_callback(void  (*callback)(void))
{
	CALLBACK_RESET_INSTR = callback ? callback : default_reset_instr_callback;
}

void m68k_set_cmpild_instr_callback(void  (*callback)(unsigned int, int))
{
	CALLBACK_CMPILD_INSTR = callback ? callback : default_cmpild_instr_callback;
}

void m68k_set_rte_instr_callback(void  (*callback)(void))
{
	CALLBACK_RTE_INSTR = callback ? callback : default_rte_instr_callback;
}

void m68k_set_tas_instr_callback(int  (*callback)(void))
{
	CALLBACK_TAS_INSTR = callback ? callback : default_tas_instr_callback;
}

void m68k_set_illg_instr_callback(int  (*callback)(int))
{
	CALLBACK_ILLG_INSTR = callback ? callback : default_illg_instr_callback;
}

void m68k_set_trap_instr_callback(int  (*callback)(int))
{
    CALLBACK_TRAP_INSTR = callback ? callback : default_trap_instr_callback;
}

void m68k_set_pc_changed_callback(void  (*callback)(unsigned int new_pc))
{
	CALLBACK_PC_CHANGED = callback ? callback : default_pc_changed_callback;
}

void m68k_set_fc_callback(void  (*callback)(unsigned int new_fc))
{
	CALLBACK_SET_FC = callback ? callback : default_set_fc_callback;
}

void m68k_set_instr_hook_callback(void  (*callback)(unsigned int pc))
{
	CALLBACK_INSTR_HOOK = callback ? callback : default_instr_hook_callback;
}

void m68k_set_cpu_type(unsigned int cpu_type)
{
	switch(cpu_type)
	{
		case M68K_CPU_TYPE_68000:
			m68ki_cpu.cpu_type         = CPU_TYPE_000;
			m68ki_cpu.address_mask = 0x00ffffff;
			m68ki_cpu.sr_mask      = 0xa71f;
			m68ki_cpu.cyc_instruction  = m68ki_cycles[0];
			CYC_EXCEPTION    = m68ki_exception_cycle_table[0];
			m68ki_cpu.cyc_bcc_notake_b = -2;
			m68ki_cpu.cyc_bcc_notake_w = 2;
			m68ki_cpu.cyc_dbcc_f_noexp = -2;
			m68ki_cpu.cyc_dbcc_f_exp   = 2;
			m68ki_cpu.cyc_scc_r_true   = 2;
			m68ki_cpu.cyc_movem_w      = 2;
			m68ki_cpu.cyc_movem_l      = 3;
			m68ki_cpu.cyc_shift        = 1;
			m68ki_cpu.cyc_reset        = 132;
			HAS_PMMU	 = 0;
			return;
		case M68K_CPU_TYPE_SCC68070:
			m68k_set_cpu_type(M68K_CPU_TYPE_68010);
			m68ki_cpu.address_mask = 0xffffffff;
			m68ki_cpu.cpu_type         = CPU_TYPE_SCC070;
			return;
		case M68K_CPU_TYPE_68010:
			m68ki_cpu.cpu_type         = CPU_TYPE_010;
			m68ki_cpu.address_mask = 0x00ffffff;
			m68ki_cpu.sr_mask      = 0xa71f;
			m68ki_cpu.cyc_instruction  = m68ki_cycles[1];
			CYC_EXCEPTION    = m68ki_exception_cycle_table[1];
			m68ki_cpu.cyc_bcc_notake_b = -4;
			m68ki_cpu.cyc_bcc_notake_w = 0;
			m68ki_cpu.cyc_dbcc_f_noexp = 0;
			m68ki_cpu.cyc_dbcc_f_exp   = 6;
			m68ki_cpu.cyc_scc_r_true   = 0;
			m68ki_cpu.cyc_movem_w      = 2;
			m68ki_cpu.cyc_movem_l      = 3;
			m68ki_cpu.cyc_shift        = 1;
			m68ki_cpu.cyc_reset        = 130;
			HAS_PMMU	 = 0;
			return;
		case M68K_CPU_TYPE_68EC020:
			m68ki_cpu.cpu_type         = CPU_TYPE_EC020;
			m68ki_cpu.address_mask = 0x00ffffff;
			m68ki_cpu.sr_mask      = 0xf71f;
			m68ki_cpu.cyc_instruction  = m68ki_cycles[2];
			CYC_EXCEPTION    = m68ki_exception_cycle_table[2];
			m68ki_cpu.cyc_bcc_notake_b = -2;
			m68ki_cpu.cyc_bcc_notake_w = 0;
			m68ki_cpu.cyc_dbcc_f_noexp = 0;
			m68ki_cpu.cyc_dbcc_f_exp   = 4;
			m68ki_cpu.cyc_scc_r_true   = 0;
			m68ki_cpu.cyc_movem_w      = 2;
			m68ki_cpu.cyc_movem_l      = 2;
			m68ki_cpu.cyc_shift        = 0;
			m68ki_cpu.cyc_reset        = 518;
			HAS_PMMU	 = 0;
			return;
		case M68K_CPU_TYPE_68020:
			m68ki_cpu.cpu_type         = CPU_TYPE_020;
			m68ki_cpu.address_mask = 0xffffffff;
			m68ki_cpu.sr_mask      = 0xf71f;
			m68ki_cpu.cyc_instruction  = m68ki_cycles[2];
			CYC_EXCEPTION    = m68ki_exception_cycle_table[2];
			m68ki_cpu.cyc_bcc_notake_b = -2;
			m68ki_cpu.cyc_bcc_notake_w = 0;
			m68ki_cpu.cyc_dbcc_f_noexp = 0;
			m68ki_cpu.cyc_dbcc_f_exp   = 4;
			m68ki_cpu.cyc_scc_r_true   = 0;
			m68ki_cpu.cyc_movem_w      = 2;
			m68ki_cpu.cyc_movem_l      = 2;
			m68ki_cpu.cyc_shift        = 0;
			m68ki_cpu.cyc_reset        = 518;
			HAS_PMMU	 = 0;
			return;
		case M68K_CPU_TYPE_68030:
			m68ki_cpu.cpu_type         = CPU_TYPE_030;
			m68ki_cpu.address_mask = 0xffffffff;
			m68ki_cpu.sr_mask      = 0xf71f;
			m68ki_cpu.cyc_instruction  = m68ki_cycles[3];
			CYC_EXCEPTION    = m68ki_exception_cycle_table[3];
			m68ki_cpu.cyc_bcc_notake_b = -2;
			m68ki_cpu.cyc_bcc_notake_w = 0;
			m68ki_cpu.cyc_dbcc_f_noexp = 0;
			m68ki_cpu.cyc_dbcc_f_exp   = 4;
			m68ki_cpu.cyc_scc_r_true   = 0;
			m68ki_cpu.cyc_movem_w      = 2;
			m68ki_cpu.cyc_movem_l      = 2;
			m68ki_cpu.cyc_shift        = 0;
			m68ki_cpu.cyc_reset        = 518;
			HAS_PMMU	       = 1;
			return;
		case M68K_CPU_TYPE_68EC030:
			m68ki_cpu.cpu_type         = CPU_TYPE_EC030;
			m68ki_cpu.address_mask = 0xffffffff;
			m68ki_cpu.sr_mask          = 0xf71f;
			m68ki_cpu.cyc_instruction  = m68ki_cycles[3];
			CYC_EXCEPTION    = m68ki_exception_cycle_table[3];
			m68ki_cpu.cyc_bcc_notake_b = -2;
			m68ki_cpu.cyc_bcc_notake_w = 0;
			m68ki_cpu.cyc_dbcc_f_noexp = 0;
			m68ki_cpu.cyc_dbcc_f_exp   = 4;
			m68ki_cpu.cyc_scc_r_true   = 0;
			m68ki_cpu.cyc_movem_w      = 2;
			m68ki_cpu.cyc_movem_l      = 2;
			m68ki_cpu.cyc_shift        = 0;
			m68ki_cpu.cyc_reset        = 518;
			HAS_PMMU	       = 0;
			return;
		case M68K_CPU_TYPE_68040:
			m68ki_cpu.cpu_type         = CPU_TYPE_040;
			m68ki_cpu.address_mask = 0xffffffff;
			m68ki_cpu.sr_mask      = 0xf71f;
			m68ki_cpu.cyc_instruction  = m68ki_cycles[4];
			CYC_EXCEPTION    = m68ki_exception_cycle_table[4];
			m68ki_cpu.cyc_bcc_notake_b = -2;
			m68ki_cpu.cyc_bcc_notake_w = 0;
			m68ki_cpu.cyc_dbcc_f_noexp = 0;
			m68ki_cpu.cyc_dbcc_f_exp   = 4;
			m68ki_cpu.cyc_scc_r_true   = 0;
			m68ki_cpu.cyc_movem_w      = 2;
			m68ki_cpu.cyc_movem_l      = 2;
			m68ki_cpu.cyc_shift        = 0;
			m68ki_cpu.cyc_reset        = 518;
			HAS_PMMU	 = 1;
			return;
		case M68K_CPU_TYPE_68EC040:
			m68ki_cpu.cpu_type         = CPU_TYPE_EC040;
			m68ki_cpu.address_mask = 0xffffffff;
			m68ki_cpu.sr_mask      = 0xf71f;
			m68ki_cpu.cyc_instruction  = m68ki_cycles[4];
			CYC_EXCEPTION    = m68ki_exception_cycle_table[4];
			m68ki_cpu.cyc_bcc_notake_b = -2;
			m68ki_cpu.cyc_bcc_notake_w = 0;
			m68ki_cpu.cyc_dbcc_f_noexp = 0;
			m68ki_cpu.cyc_dbcc_f_exp   = 4;
			m68ki_cpu.cyc_scc_r_true   = 0;
			m68ki_cpu.cyc_movem_w      = 2;
			m68ki_cpu.cyc_movem_l      = 2;
			m68ki_cpu.cyc_shift        = 0;
			m68ki_cpu.cyc_reset        = 518;
			HAS_PMMU	 = 0;
			return;
		case M68K_CPU_TYPE_68LC040:
			m68ki_cpu.cpu_type         = CPU_TYPE_LC040;
			m68ki_cpu.sr_mask          = 0xf71f;
			m68ki_cpu.cyc_instruction  = m68ki_cycles[4];
			m68ki_cpu.cyc_exception    = m68ki_exception_cycle_table[4];
			m68ki_cpu.cyc_bcc_notake_b = -2;
			m68ki_cpu.cyc_bcc_notake_w = 0;
			m68ki_cpu.cyc_dbcc_f_noexp = 0;
			m68ki_cpu.cyc_dbcc_f_exp   = 4;
			m68ki_cpu.cyc_scc_r_true   = 0;
			m68ki_cpu.cyc_movem_w      = 2;
			m68ki_cpu.cyc_movem_l      = 2;
			m68ki_cpu.cyc_shift        = 0;
			m68ki_cpu.cyc_reset        = 518;
			HAS_PMMU	       = 1;
			return;
	}
}

int MD_M68K_FAST m68k_execute(int num_cycles)
{
	if (RESET_CYCLES) {
	    int rc = RESET_CYCLES;
	    RESET_CYCLES = 0;
	    num_cycles -= rc;
	    if (num_cycles <= 0)
		return rc;
	}

	SET_CYCLES(num_cycles);
	m68ki_initial_cycles = num_cycles;

	m68ki_check_interrupts();

	/* Make sure we're not stopped */
	if(!CPU_STOPPED)
	{
		m68ki_set_address_error_trap(); /* auto-disable (see m68kcpu.h) */

#if MD_M68K_BUS_ERROR
		m68ki_check_bus_error_trap();
#endif

		do
		{
			int i;
			m68ki_trace_t1(); /* auto-disable (see m68kcpu.h) */

			/* Set the address space for reads */
			m68ki_use_data_space(); /* auto-disable (see m68kcpu.h) */

			m68ki_instr_hook(REG_PC); /* auto-disable (see m68kcpu.h) */

			REG_PPC = REG_PC;

#if MD_M68K_BUS_ERROR
			for (i = 15; i >= 0; i--){
				REG_DA_SAVE[i] = REG_DA[i];
			}
#else
			(void)i;
#endif

			REG_IR = m68ki_read_imm_16();
			m68ki_instruction_jump_table[REG_IR]();
			USE_CYCLES(CYC_INSTRUCTION[REG_IR]);

			/* Trace m68k_exception, if necessary */
			m68ki_exception_if_trace(); /* auto-disable (see m68kcpu.h) */
		} while(GET_CYCLES() > 0);

		REG_PPC = REG_PC;
	}
	else
		SET_CYCLES(0);

	return m68ki_initial_cycles - GET_CYCLES();
}


int m68k_cycles_run(void)
{
	return m68ki_initial_cycles - GET_CYCLES();
}

int m68k_cycles_remaining(void)
{
	return GET_CYCLES();
}

void m68k_modify_timeslice(int cycles)
{
	m68ki_initial_cycles += cycles;
	ADD_CYCLES(cycles);
}


void m68k_end_timeslice(void)
{
	m68ki_initial_cycles -= GET_CYCLES();
	SET_CYCLES(0);
}


/* ASG: rewrote so that the int_level is a mask of the IPL0/IPL1/IPL2 bits */
/* KS: Modified so that IPL* bits match with mask positions in the SR
 *     and cleaned out remenants of the interrupt controller.
 */
void m68k_set_irq(unsigned int int_level)
{
	uint old_level = CPU_INT_LEVEL;
	CPU_INT_LEVEL = int_level << 8;

	if(old_level != 0x0700 && CPU_INT_LEVEL == 0x0700)
		m68ki_cpu.nmi_pending = TRUE;
}

void m68k_set_virq(unsigned int level, unsigned int active)
{
	uint state = m68ki_cpu.virq_state;
	uint blevel;

	if(active)
		state |= 1 << level;
	else
		state &= ~(1 << level);
	m68ki_cpu.virq_state = state;

	for(blevel = 7; blevel > 0; blevel--)
		if(state & (1 << blevel))
			break;
	m68k_set_irq(blevel);
}

unsigned int m68k_get_virq(unsigned int level)
{
	return (m68ki_cpu.virq_state & (1 << level)) ? 1 : 0;
}

void m68k_init(void)
{
	static uint emulation_initialized = 0;

	/* The first call to this function initializes the opcode handler jump table */
	if(!emulation_initialized)
		{
		emulation_initialized = 1;
	}

	m68k_set_int_ack_callback(NULL);
	m68k_set_bkpt_ack_callback(NULL);
	m68k_set_reset_instr_callback(NULL);
	m68k_set_cmpild_instr_callback(NULL);
	m68k_set_rte_instr_callback(NULL);
	m68k_set_tas_instr_callback(NULL);
	m68k_set_illg_instr_callback(NULL);
    m68k_set_trap_instr_callback(NULL);
	m68k_set_pc_changed_callback(NULL);
	m68k_set_fc_callback(NULL);
	m68k_set_instr_hook_callback(NULL);
}

void m68k_pulse_bus_error(void)
{
	m68ki_exception_bus_error();
}

/* Pulse the RESET line on the CPU */
void m68k_pulse_reset(void)
{
	m68ki_cpu.pmmu_enabled = 0;

	CPU_STOPPED = 0;
	SET_CYCLES(0);

	CPU_RUN_MODE = RUN_MODE_BERR_AERR_RESET;
	CPU_INSTR_MODE = INSTRUCTION_YES;

	/* Turn off tracing */
	FLAG_T1 = FLAG_T0 = 0;
	m68ki_clear_trace();
	/* Interrupt mask to level 7 */
	FLAG_INT_MASK = 0x0700;
	CPU_INT_LEVEL = 0;
	m68ki_cpu.virq_state = 0;
	REG_VBR = 0;
	/* Go to supervisor mode */
	m68ki_set_sm_flag(SFLAG_SET | MFLAG_CLEAR);

	/* Invalidate the prefetch queue */
#if M68K_EMULATE_PREFETCH
	/* Set to arbitrary number since our first fetch is from 0 */
	CPU_PREF_ADDR = 0x1000;
#endif /* M68K_EMULATE_PREFETCH */

	/* Read the initial stack pointer and program counter */
	m68ki_jump(0);
	REG_SP = m68ki_read_imm_32();
	REG_PC = m68ki_read_imm_32();
	m68ki_jump(REG_PC);

	CPU_RUN_MODE = RUN_MODE_NORMAL;

	RESET_CYCLES = CYC_EXCEPTION[EXCEPTION_RESET];
}

/* Pulse the HALT line on the CPU */
void m68k_pulse_halt(void)
{
	CPU_STOPPED |= STOP_LEVEL_HALT;
}

unsigned int m68k_context_size(void)
{
	return sizeof(m68ki_cpu_core);
}

unsigned int m68k_get_context(void* dst)
{
	if(dst) *(m68ki_cpu_core*)dst = m68ki_cpu;
	return sizeof(m68ki_cpu_core);
}

void m68k_set_context(void* src)
{
	if(src) m68ki_cpu = *(m68ki_cpu_core*)src;
}

/* ======================================================================== */
/* ======================================================================== */

#if M68K_COMPILE_FOR_MAME == M68K_OPT_ON

static struct {
	UINT16 sr;
	UINT8 stopped;
	UINT8 halted;
} m68k_substate;

static void m68k_prepare_substate(void)
{
	m68k_substate.sr = m68ki_get_sr();
	m68k_substate.stopped = (CPU_STOPPED & STOP_LEVEL_STOP) != 0;
	m68k_substate.halted  = (CPU_STOPPED & STOP_LEVEL_HALT) != 0;
}

static void m68k_post_load(void)
{
	m68ki_set_sr_noint_nosp(m68k_substate.sr);
	CPU_STOPPED = m68k_substate.stopped ? STOP_LEVEL_STOP : 0
		        | m68k_substate.halted  ? STOP_LEVEL_HALT : 0;
	m68ki_jump(REG_PC);
}

void m68k_state_register(const char *type, int index)
{
	state_save_register_item_array(type, index, REG_D);
	state_save_register_item(type, index, REG_PPC);
	state_save_register_item(type, index, REG_PC);
	state_save_register_item(type, index, REG_USP);
	state_save_register_item(type, index, REG_ISP);
	state_save_register_item(type, index, REG_MSP);
	state_save_register_item(type, index, REG_VBR);
	state_save_register_item(type, index, REG_SFC);
	state_save_register_item(type, index, REG_DFC);
	state_save_register_item(type, index, REG_CACR);
	state_save_register_item(type, index, REG_CAAR);
	state_save_register_item(type, index, m68k_substate.sr);
	state_save_register_item(type, index, CPU_INT_LEVEL);
	state_save_register_item(type, index, m68k_substate.stopped);
	state_save_register_item(type, index, m68k_substate.halted);
	state_save_register_item(type, index, CPU_PREF_ADDR);
	state_save_register_item(type, index, CPU_PREF_DATA);
	state_save_register_func_presave(m68k_prepare_substate);
	state_save_register_func_postload(m68k_post_load);
}

#endif

/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */
