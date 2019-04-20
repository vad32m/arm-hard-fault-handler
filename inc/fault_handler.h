/**
 * @file    FaultHandler.h
 * @author  Ferenc Nemeth
 * @date    12 Aug 2018
 * @brief   This is the fault handler, where the program gets, if there is a problem.
 *          In case of a Hard fault, the following actions are going to be executed:
 *          - Prints out the stack frame registers (R0-R3, R12, LR, PC, PSR).
 *          - Prints out the fault status registers (HFSR, CFSR, MMFAR, BFAR, AFSR).
 *          - Prints out the EXC_RETURN register.
 *          - After these, the software analyzes the value of the registers
 *            and prints out a report regarding the fault.
 *          - Stop the execution with a breakpoint.
 *
 *          Copyright (c) 2018 Ferenc Nemeth - https://github.com/ferenc-nemeth
 *          Copyright (c) 2019 Vadym Mishchuk - https://github.com/vad32m
 */

#ifndef FAULTHANDLER_H_
#define FAULTHANDLER_H_

#include <stdint.h>

/* Bit masking. */
#define CHECK_BIT(REG, POS) ((REG) & (1u << (POS)))

/* Hard Fault Status Register. */
#define FORCED              ((uint8_t)30u)
#define VECTTBL             ((uint8_t)1u)

/* MemManage Fault Status Register (MMFSR; 0-7 bits in CFSR). */
#define MMARVALID           ((uint8_t)7u)
#define MLSPERR             ((uint8_t)5u)   /**< Only on ARM Cortex-M4F. */
#define MSTKERR             ((uint8_t)4u)
#define MUNSTKERR           ((uint8_t)3u)
#define DACCVIOL            ((uint8_t)1u)
#define IACCVIOL            ((uint8_t)0u)

/* Bus Fault Status Register (BFSR; 8-15 bits in CFSR). */
#define BFARVALID           ((uint8_t)15u)
#define LSPERR              ((uint8_t)13u)  /**< Only on ARM Cortex-M4F. */
#define STKERR              ((uint8_t)12u)
#define UNSTKERR            ((uint8_t)11u)
#define IMPRECISERR         ((uint8_t)10u)
#define PRECISERR           ((uint8_t)9u)
#define IBUSERR             ((uint8_t)8u)

/* Usage Fault Status Register (BFSR; 16-25 bits in CFSR). */
#define DIVBYZERO           ((uint8_t)25u)  /**< Has to be enabled in CCR. */
#define UNALIGNED           ((uint8_t)24u)  /**< Has to be enabled in CCR. */
#define NOCP                ((uint8_t)19u)
#define INVPC               ((uint8_t)18u)
#define INVSTATE            ((uint8_t)17u)
#define UNDEFINSTR          ((uint8_t)16u)

void ReportStackUsage(uint32_t *stack_frame, uint32_t exc);

void ReportMemanageFault(void);

void ReportBusFault(void);

void ReportUsageFault(void);

void ReportHardFault(void);

#define REPORT_STACK_FRAME	 __asm volatile \
                ( \
  	                "TST    LR, #0b0100;      " \
  	                "ITE    EQ;               " \
 	                "MRSEQ  R0, MSP;          " \
                    "MRSNE  R0, PSP;          " \
                    "MOV    R1, LR;           " \
                    "BL     ReportStackUsage; " \
                );

#endif /* FAULTHANDLER_H_ */

