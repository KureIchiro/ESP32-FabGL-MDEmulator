/* ======================================================================== */
/* ========================= LICENSING & COPYRIGHT ======================== */
/* ======================================================================== */


#ifndef M68K__HEADER
#define M68K__HEADER

#ifdef __cplusplus
extern "C" {
#endif

#ifndef M68K_ARRAY_LENGTH
#define M68K_ARRAY_LENGTH(x)         (sizeof(x) / sizeof(x[0]))
#endif

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

/* ======================================================================== */
/* ======================================================================== */

#ifdef MUSASHI_CNF
#include MUSASHI_CNF
#else
#include "m68kconf.h"
#endif

/* ======================================================================== */
/* ============================ GENERAL DEFINES =========================== */

/* ======================================================================== */

/* There are 7 levels of interrupt to the 68K.
 * A transition from < 7 to 7 will cause a non-maskable interrupt (NMI).
 */
#define M68K_IRQ_NONE 0
#define M68K_IRQ_1    1
#define M68K_IRQ_2    2
#define M68K_IRQ_3    3
#define M68K_IRQ_4    4
#define M68K_IRQ_5    5
#define M68K_IRQ_6    6
#define M68K_IRQ_7    7


/* Special interrupt acknowledge values.
 * Use these as special returns from the interrupt acknowledge callback
 * (specified later in this header).
 */

/* Causes an interrupt autovector (0x18 + interrupt level) to be taken.
 * This happens in a real 68K if VPA or AVEC is asserted during an interrupt
 * acknowledge cycle instead of DTACK.
 */
#define M68K_INT_ACK_AUTOVECTOR    0xffffffff

/* Causes the spurious interrupt vector (0x18) to be taken
 * This happens in a real 68K if BERR is asserted during the interrupt
 * acknowledge cycle (i.e. no devices responded to the acknowledge).
 */
#define M68K_INT_ACK_SPURIOUS      0xfffffffe


enum
{
	M68K_CPU_TYPE_INVALID,
	M68K_CPU_TYPE_68000,
	M68K_CPU_TYPE_68010,
	M68K_CPU_TYPE_68EC020,
	M68K_CPU_TYPE_68020,
	M68K_CPU_TYPE_68EC030,
	M68K_CPU_TYPE_68030,
	M68K_CPU_TYPE_68EC040,
	M68K_CPU_TYPE_68LC040,
	M68K_CPU_TYPE_68040,
	M68K_CPU_TYPE_SCC68070
};

/* Registers used by m68k_get_reg() and m68k_set_reg() */
typedef enum
{
	/* Real registers */
	M68K_REG_D0,		/* Data registers */
	M68K_REG_D1,
	M68K_REG_D2,
	M68K_REG_D3,
	M68K_REG_D4,
	M68K_REG_D5,
	M68K_REG_D6,
	M68K_REG_D7,
	M68K_REG_A0,		/* Address registers */
	M68K_REG_A1,
	M68K_REG_A2,
	M68K_REG_A3,
	M68K_REG_A4,
	M68K_REG_A5,
	M68K_REG_A6,
	M68K_REG_A7,
	M68K_REG_PC,		/* Program Counter */
	M68K_REG_SR,		/* Status Register */
	M68K_REG_SP,		/* The current Stack Pointer (located in A7) */
	M68K_REG_USP,		/* User Stack Pointer */
	M68K_REG_ISP,		/* Interrupt Stack Pointer */
	M68K_REG_MSP,
	M68K_REG_SFC,
	M68K_REG_DFC,
	M68K_REG_VBR,
	M68K_REG_CACR,
	M68K_REG_CAAR,

	/* Assumed registers */
	/* These are cheat registers which emulate the 1-longword prefetch
	 * present in the 68000 and 68010.
	 */
	M68K_REG_PREF_ADDR,	/* Last prefetch address */
	M68K_REG_PREF_DATA,	/* Last prefetch data */

	/* Convenience registers */
	M68K_REG_PPC,
	M68K_REG_IR,		/* Instruction register */
	M68K_REG_CPU_TYPE
} m68k_register_t;

/* ======================================================================== */
/* ======================================================================== */



/* Read from anywhere */
unsigned int  m68k_read_memory_8(unsigned int address);
unsigned int  m68k_read_memory_16(unsigned int address);
unsigned int  m68k_read_memory_32(unsigned int address);

/* Read data immediately following the PC */
unsigned int  m68k_read_immediate_16(unsigned int address);
unsigned int  m68k_read_immediate_32(unsigned int address);

/* Read data relative to the PC */
unsigned int  m68k_read_pcrelative_8(unsigned int address);
unsigned int  m68k_read_pcrelative_16(unsigned int address);
unsigned int  m68k_read_pcrelative_32(unsigned int address);

/* Memory access for the disassembler */
unsigned int m68k_read_disassembler_8  (unsigned int address);
unsigned int m68k_read_disassembler_16 (unsigned int address);
unsigned int m68k_read_disassembler_32 (unsigned int address);

/* Write to anywhere */
void m68k_write_memory_8(unsigned int address, unsigned int value);
void m68k_write_memory_16(unsigned int address, unsigned int value);
void m68k_write_memory_32(unsigned int address, unsigned int value);

void m68k_write_memory_32_pd(unsigned int address, unsigned int value);



/* ======================================================================== */
/* ============================== CALLBACKS =============================== */
/* ======================================================================== */

/* These functions allow you to set callbacks to the host when specific events
 * occur.  Note that you must enable the corresponding value in m68kconf.h
 * in order for these to do anything useful.
 * Note: I have defined default callbacks which are used if you have enabled
 * the corresponding #define in m68kconf.h but either haven't assigned a
 * callback or have assigned a callback of NULL.
 */

/* Set the callback for an interrupt acknowledge.
 * You must enable M68K_EMULATE_INT_ACK in m68kconf.h.
 * The CPU will call the callback with the interrupt level being acknowledged.
 * The host program must return either a vector from 0x02-0xff, or one of the
 * special interrupt acknowledge values specified earlier in this header.
 * If this is not implemented, the CPU will always assume an autovectored
 * interrupt, and will automatically clear the interrupt request when it
 * services the interrupt.
 * Default behavior: return M68K_INT_ACK_AUTOVECTOR.
 */
void m68k_set_int_ack_callback(int  (*callback)(int int_level));


void m68k_set_bkpt_ack_callback(void (*callback)(unsigned int data));


/* Set the callback for the RESET instruction.
 * You must enable M68K_EMULATE_RESET in m68kconf.h.
 * The CPU calls this callback every time it encounters a RESET instruction.
 * Default behavior: do nothing.
 */
void m68k_set_reset_instr_callback(void  (*callback)(void));


void m68k_set_pc_changed_callback(void  (*callback)(unsigned int new_pc));

/* Set the callback for the TAS instruction.
 * You must enable M68K_TAS_HAS_CALLBACK in m68kconf.h.
 * The CPU calls this callback every time it encounters a TAS instruction.
 * Default behavior: return 1, allow writeback.
 */
void m68k_set_tas_instr_callback(int  (*callback)(void));

void m68k_set_illg_instr_callback(int  (*callback)(int));

void m68k_set_trap_instr_callback(int  (*callback)(int));

/* Set the callback for CPU function code changes.
 * You must enable M68K_EMULATE_FC in m68kconf.h.
 * The CPU calls this callback with the function code before every memory
 * access to set the CPU's function code according to what kind of memory
 * access it is (supervisor/user, program/data and such).
 * Default behavior: do nothing.
 */
void m68k_set_fc_callback(void  (*callback)(unsigned int new_fc));


void m68k_set_instr_hook_callback(void  (*callback)(unsigned int pc));



/* ======================================================================== */
/* ====================== FUNCTIONS TO ACCESS THE CPU ===================== */
/* ======================================================================== */

void m68k_set_cpu_type(unsigned int cpu_type);

/* Do whatever initialisations the core requires.  Should be called
 * at least once at init time.
 */
void m68k_init(void);

void m68k_pulse_reset(void);

int m68k_execute(int num_cycles);

int m68k_cycles_run(void);
int m68k_cycles_remaining(void);
void m68k_modify_timeslice(int cycles);
void m68k_end_timeslice(void);

/* Set the IPL0-IPL2 pins on the CPU (IRQ).
 * A transition from < 7 to 7 will cause a non-maskable interrupt (NMI).
 * Setting IRQ to 0 will clear an interrupt request.
 */
void m68k_set_irq(unsigned int int_level);

void m68k_set_virq(unsigned int level, unsigned int active);
unsigned int m68k_get_virq(unsigned int level);

/* Halt the CPU as if you pulsed the HALT pin. */
void m68k_pulse_halt(void);


void m68k_pulse_bus_error(void);



unsigned int m68k_context_size(void);

unsigned int m68k_get_context(void* dst);

void m68k_set_context(void* dst);

void m68k_state_register(const char *type, int index);


/* Peek at the internals of a CPU context.  This can either be a context
 * retrieved using m68k_get_context() or the currently running context.
 * If context is NULL, the currently running CPU context will be used.
 */
unsigned int m68k_get_reg(void* context, m68k_register_t reg);

/* Poke values into the internals of the currently running CPU context */
void m68k_set_reg(m68k_register_t reg, unsigned int value);

unsigned int m68k_is_valid_instruction(unsigned int instruction, unsigned int cpu_type);

unsigned int m68k_disassemble(char* str_buff, unsigned int pc, unsigned int cpu_type);

unsigned int m68k_disassemble_raw(char* str_buff, unsigned int pc, const unsigned char* opdata, const unsigned char* argdata, unsigned int cpu_type);


/* ======================================================================== */
/* ======================================================================== */

#if M68K_COMPILE_FOR_MAME == M68K_OPT_ON
#include "m68kmame.h"
#endif


/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#ifdef __cplusplus
}
#endif


#include "md_m68k.h"

#endif /* M68K__HEADER */
