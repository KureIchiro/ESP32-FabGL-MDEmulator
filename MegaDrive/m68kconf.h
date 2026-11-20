/* ======================================================================== */
/* ======================================================================== */




#ifndef M68KCONF__HEADER
#define M68KCONF__HEADER


#define M68K_OPT_OFF             0
#define M68K_OPT_ON              1
#define M68K_OPT_SPECIFY_HANDLER 2

#include "md_m68kconf.h"


/* ======================================================================== */
/* ======================================================================== */

#ifndef M68K_COMPILE_FOR_MAME
#define M68K_COMPILE_FOR_MAME      M68K_OPT_OFF
#endif


#if M68K_COMPILE_FOR_MAME == M68K_OPT_OFF


/* ======================================================================== */
/* ======================================================================== */

#ifndef M68K_EMULATE_010
#define M68K_EMULATE_010            M68K_OPT_ON
#endif

#ifndef M68K_EMULATE_EC020
#define M68K_EMULATE_EC020          M68K_OPT_ON
#endif

#ifndef M68K_EMULATE_020
#define M68K_EMULATE_020            M68K_OPT_ON
#endif

#ifndef M68K_EMULATE_030
#define M68K_EMULATE_030            M68K_OPT_ON
#endif

#ifndef M68K_EMULATE_040
#define M68K_EMULATE_040            M68K_OPT_ON
#endif


#ifndef M68K_SEPARATE_READS
#define M68K_SEPARATE_READS         M68K_OPT_OFF
#endif

/* If ON, the CPU will call m68k_write_32_pd() when it executes move.l with a
 * predecrement destination EA mode instead of m68k_write_32().
 * To simulate real 68k behavior, m68k_write_32_pd() must first write the high
 * word to [address+2], and then write the low word to [address].
 */
#ifndef M68K_SIMULATE_PD_WRITES
#define M68K_SIMULATE_PD_WRITES     M68K_OPT_OFF
#endif

/* If ON, CPU will call the interrupt acknowledge callback when it services an
 * interrupt.
 * If off, all interrupts will be autovectored and all interrupt requests will
 * auto-clear when the interrupt is serviced.
 */
#ifndef M68K_EMULATE_INT_ACK
#define M68K_EMULATE_INT_ACK        M68K_OPT_OFF
#define M68K_INT_ACK_CALLBACK(A)    your_int_ack_handler_function(A)
#endif


#ifndef M68K_EMULATE_BKPT_ACK
#define M68K_EMULATE_BKPT_ACK       M68K_OPT_OFF
#define M68K_BKPT_ACK_CALLBACK()    your_bkpt_ack_handler_function()
#endif


/* If ON, the CPU will monitor the trace flags and take trace exceptions
 */
#ifndef M68K_EMULATE_TRACE
#define M68K_EMULATE_TRACE          M68K_OPT_OFF
#endif


/* If ON, CPU will call the output reset callback when it encounters a reset
 * instruction.
 */
#ifndef M68K_EMULATE_RESET
#define M68K_EMULATE_RESET          M68K_OPT_OFF
#define M68K_RESET_CALLBACK()       your_reset_handler_function()
#endif

#ifndef M68K_CMPILD_HAS_CALLBACK
#define M68K_CMPILD_HAS_CALLBACK     M68K_OPT_OFF
#define M68K_CMPILD_CALLBACK(v,r)    your_cmpild_handler_function(v,r)
#endif


#ifndef M68K_RTE_HAS_CALLBACK
#define M68K_RTE_HAS_CALLBACK       M68K_OPT_OFF
#define M68K_RTE_CALLBACK()         your_rte_handler_function()
#endif

/* If ON, CPU will call the callback when it encounters a tas
 * instruction.
 */
#ifndef M68K_TAS_HAS_CALLBACK
#define M68K_TAS_HAS_CALLBACK       M68K_OPT_OFF
#define M68K_TAS_CALLBACK()         your_tas_handler_function()
#endif

#ifndef M68K_ILLG_HAS_CALLBACK
#define M68K_ILLG_HAS_CALLBACK      M68K_OPT_OFF
#define M68K_ILLG_CALLBACK(opcode)  your_op_illg_handler_function(opcode)
#endif

#ifndef M68K_TRAP_HAS_CALLBACK
#define M68K_TRAP_HAS_CALLBACK  	M68K_OPT_OFF
#define M68K_TRAP_CALLBACK(trap)	your_op_trap_handler_function(trap)
#endif

/* If ON, CPU will call the set fc callback on every memory access to
 * differentiate between user/supervisor, program/data access like a real
 * 68000 would.  This should be enabled and the callback should be set if you
 * want to properly emulate the m68010 or higher. (moves uses function codes
 * to read/write data from different address spaces)
 */
#ifndef M68K_EMULATE_FC
#define M68K_EMULATE_FC             M68K_OPT_OFF
#define M68K_SET_FC_CALLBACK(A)     your_set_fc_handler_function(A)
#endif

#ifndef M68K_MONITOR_PC
#define M68K_MONITOR_PC             M68K_OPT_OFF
#define M68K_SET_PC_CALLBACK(A)     your_pc_changed_handler_function(A)
#endif


#ifndef M68K_INSTRUCTION_HOOK
#define M68K_INSTRUCTION_HOOK       M68K_OPT_OFF
#define M68K_INSTRUCTION_CALLBACK(pc) your_instruction_hook_function(pc)
#endif


/* If ON, the CPU will emulate the 4-byte prefetch queue of a real 68000 */
#ifndef M68K_EMULATE_PREFETCH
#define M68K_EMULATE_PREFETCH       M68K_OPT_OFF
#endif


/* If ON, the CPU will generate address error exceptions if it tries to
 * access a word or longword at an odd address.
 * NOTE: This is only emulated properly for 68000 mode.
 */
#ifndef M68K_EMULATE_ADDRESS_ERROR
#define M68K_EMULATE_ADDRESS_ERROR  M68K_OPT_OFF
#endif


#ifndef M68K_LOG_ENABLE
#define M68K_LOG_ENABLE             M68K_OPT_OFF
#define M68K_LOG_1010_1111          M68K_OPT_OFF
#define M68K_LOG_TRAP               M68K_OPT_OFF
#define M68K_LOG_FILEHANDLE         some_file_handle
#endif

#ifndef M68K_EMULATE_PMMU
#define M68K_EMULATE_PMMU           M68K_OPT_ON
#endif

/* ----------------------------- COMPATIBILITY ---------------------------- */

/* The following options set optimizations that violate the current ANSI
 * standard, but will be compliant under the forthcoming C9X standard.
 */


/* If ON, the enulation core will use 64-bit integers to speed up some
 * operations.
*/
#ifndef M68K_USE_64_BIT
#define M68K_USE_64_BIT             M68K_OPT_ON
#endif


#endif

/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#endif /* M68KCONF__HEADER */
