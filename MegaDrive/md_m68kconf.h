/* ============================================================================
 *  md_m68kconf.h - 2026-11-20 MegaDrive for TTGO VGA32: the Musashi settings for the Mega Drive 68000.
 *  Included by m68kconf.h before its defaults (every option there is #ifndef-guarded).
 *  GPLv3 (part of this sketch, LICENSE). Musashi's own files (m68k.h, m68kconf.h, m68kcpu.c/.h, m68kops.c/.h) are under the MIT license.
 * ============================================================================ */
#ifndef MD_M68KCONF_H
#define MD_M68KCONF_H

#define M68K_EMULATE_010            M68K_OPT_OFF
#define M68K_EMULATE_EC020          M68K_OPT_OFF
#define M68K_EMULATE_020            M68K_OPT_OFF
#define M68K_EMULATE_030            M68K_OPT_OFF
#define M68K_EMULATE_040            M68K_OPT_OFF
#define M68K_EMULATE_PMMU           M68K_OPT_OFF

#define M68K_SEPARATE_READS         M68K_OPT_ON

#define M68K_EMULATE_INT_ACK        M68K_OPT_ON

#define M68K_USE_64_BIT             M68K_OPT_OFF

#ifndef MD_M68K_CONST
#define MD_M68K_CONST 1
#endif

#ifndef MD_M68K_FPU
#define MD_M68K_FPU 0
#endif
#ifndef MD_M68K_BUS_ERROR
#define MD_M68K_BUS_ERROR 0
#endif

#endif
