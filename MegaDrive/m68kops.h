#ifndef M68KOPS__HEADER
#define M68KOPS__HEADER

/* ======================================================================== */
/* ======================================================================== */



extern int (*const m68ki_instruction_jump_table[0x10000])(void);
extern const unsigned char m68ki_cycles[][0x10000];

typedef int (*m68k_op_fn)(void);
#define MD_DISP_NREG 190
#define MD_DISP_NIRR 27
extern const unsigned char *m68k_disp_l1, *m68k_disp_slot;
extern const m68k_op_fn (*m68k_disp_reg)[17];
extern const m68k_op_fn (*m68k_disp_irr)[64];


/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#endif
