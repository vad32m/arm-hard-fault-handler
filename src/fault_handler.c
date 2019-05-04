/**
 * @file    FaultHandler.c
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

#include "fault_config.h"

#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/scs.h>
#include <stdint.h>
#include <stdio.h>

/**
 * @brief Macro that should be called to report stack frame
 * and processor status register 
 */
#define REPORT_STACK_FRAME	 __asm volatile \
                ( \
  	                "TST    LR, #0b0100;      " \
  	                "ITE    EQ;               " \
 	                "MRSEQ  R0, MSP;          " \
                    "MRSNE  R0, PSP;          " \
                    "MOV    R1, LR;           " \
                    "BL     ReportStackUsage; " \
                );


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

/**
 * @brief   Prints the registers and gives detailed information about the error(s).
 * Should be invked from REPORT_STACK_FRAME macro.
 * @param   *stack_frame: Stack frame registers (R0-R3, R12, LR, LC, PSR).
 * @param   exc: EXC_RETURN register.
 * @return  void
 */
void
ReportStackUsage(uint32_t *stack_frame, uint32_t exc);

/**
 * @brief  Print data about CFSR bits that relevant to memory management fault
 */
static void
ReportMemanageFault(void);

/**
 * @brief  Print data about CFSR bits that relevant to bus fault
 */
static void
ReportBusFault(void);

/**
 * @brief  Print data about CFSR bits that relevant to usage fault
 */
static void
ReportUsageFault(void);

/**
 * @brief  Print data about HFSR bits
 */
static void
ReportHardFault(void);

/**
 * @brief Trigger breakpoint if debugger is connected.
 * Infinite loop if no debugger connected.
 */
static inline void
HaltExecution(void)
{
    if (SCS_DHCSR & SCS_DHCSR_C_DEBUGEN) {
        /* Breakpoint. */
        __asm volatile("BKPT #0");
    } else {
        /* Infinite loop to stop the execution. */
        while(1);
    }
}

#ifdef MEMMANAGE_FAULT_SYMBOL
void
MEMMANAGE_FAULT_SYMBOL(void)
{
    REPORT_STACK_FRAME
    ReportMemanageFault();
    HaltExecution();
}
#endif

#ifdef HARD_FAULT_SYMBOL
void
HARD_FAULT_SYMBOL(void)
{
    REPORT_STACK_FRAME
    ReportMemanageFault();
    ReportBusFault();
    ReportUsageFault();
    ReportHardFault();
    HaltExecution();
}
#endif

#ifdef BUS_FAULT_SYMBOL
void
BUS_FAULT_SYMBOL(void)
{
    REPORT_STACK_FRAME
    ReportBusFault();
    HaltExecution();
}
#endif

#ifdef USAGE_FAULT_SYMBOL
void
USAGE_FAULT_SYMBOL(void)
{
    REPORT_STACK_FRAME
    ReportUsageFault();
    HaltExecution();
}
#endif

void
ReportStackUsage(uint32_t *stack_frame, uint32_t exc)
{
  uint32_t r0   = stack_frame[0];
  uint32_t r1   = stack_frame[1];
  uint32_t r2   = stack_frame[2];
  uint32_t r3   = stack_frame[3];
  uint32_t r12  = stack_frame[4];
  uint32_t lr   = stack_frame[5];
  uint32_t pc   = stack_frame[6];
  uint32_t psr  = stack_frame[7];
  uint32_t hfsr = SCB_HFSR;
  uint32_t cfsr = SCB_CFSR;
  uint32_t mmar = SCB_MMFAR;
  uint32_t bfar = SCB_BFAR;
  uint32_t afsr = SCB_AFSR;
  printf("\n!!!Fault detected!!!\n");

  printf("\nStack frame:\n");
  printf("R0 :        0x%08lX\n", r0);
  printf("R1 :        0x%08lX\n", r1);
  printf("R2 :        0x%08lX\n", r2);
  printf("R3 :        0x%08lX\n", r3);
  printf("R12:        0x%08lX\n", r12);
  printf("LR :        0x%08lX\n", lr);
  printf("PC :        0x%08lX\n", pc);
  printf("PSR:        0x%08lX\n", psr);

  printf("\nFault status:\n");
  printf("HFSR:       0x%08lX\n", hfsr);
  printf("CFSR:       0x%08lX\n", cfsr);
  printf("MMAR:       0x%08lX\n", mmar);
  printf("BFAR:       0x%08lX\n", bfar);
  printf("AFSR:       0x%08lX\n", afsr);

  printf("\nOther:\n");
  printf("EXC_RETURN: 0x%08lX\n", exc);
}

static void
ReportMemanageFault(void)
{
    uint32_t cfsr = SCB_CFSR;

    printf("MemManage fault status:\n");

    if (CHECK_BIT(cfsr, MMARVALID)) {
        printf(" - MMAR holds a valid address.\n");
    } else {
        printf(" - MMAR holds an invalid address.\n");
    }

    if (CHECK_BIT(cfsr, MLSPERR)) {
        printf(" - Fault occurred during floating-point lazy state preservation.\n");
    }

    if (CHECK_BIT(cfsr, MSTKERR)) {
        printf(" - Stacking has caused an access violation.\n");
    }

    if (CHECK_BIT(cfsr, MUNSTKERR)) {
        printf(" - Unstacking has caused an access violation.\n");
    }

    if (CHECK_BIT(cfsr, DACCVIOL)) {
        printf(" - Load or store at a location that does not permit the operation.\n");
    }

    if (CHECK_BIT(cfsr, IACCVIOL)) {
        printf(" - Instruction fetch from a location that does not permit execution.\n");
    }
}

static void
ReportBusFault(void)
{
    uint32_t cfsr = SCB_CFSR;

    printf("Bus fault status:\n");

    if (CHECK_BIT(cfsr, BFARVALID)) {
        printf(" - BFAR holds a valid address.\n");
    } else {
        printf(" - BFAR holds an invalid address.\n");
    }

    if (CHECK_BIT(cfsr, LSPERR)) {
        printf(" - Fault occurred during floating-point lazy state preservation.\n");
    }

    if (CHECK_BIT(cfsr, STKERR)) {
        printf(" - Stacking has caused a Bus fault.\n");
    }

    if (CHECK_BIT(cfsr, UNSTKERR)) {
        printf(" - Unstacking has caused a Bus fault.\n");
    }

    if (CHECK_BIT(cfsr, IMPRECISERR)) {
        printf(" - Data bus error has occurred, but the return address in the stack is not related to the fault.\n");
    }

    if (CHECK_BIT(cfsr, PRECISERR)) {
        printf(" - Data bus error has occurred, and the return address points to the instruction that caused the fault.\n");
    }

    if (CHECK_BIT(cfsr, IBUSERR)) {
        printf(" - Instruction bus error.\n");
    }
}

static void
ReportUsageFault(void)
{
    uint32_t cfsr = SCB_CFSR;

    printf("Usage fault status:\n");

    if (CHECK_BIT(cfsr, DIVBYZERO)) {
        printf(" - The processor has executed an SDIV or UDIV instruction with a divisor of 0.\n");
    }

    if (CHECK_BIT(cfsr, UNALIGNED)) {
        printf(" - The processor has made an unaligned memory access.\n");
    }

    if (CHECK_BIT(cfsr, NOCP)) {
        printf(" - Attempted to access a coprocessor.\n");
    }

    if (CHECK_BIT(cfsr, INVPC)) {
        printf(" - Illegal attempt to load of EXC_RETURN to the PC.\n");
    }

    if (CHECK_BIT(cfsr, INVSTATE)) {
        printf(" - Attempted to execute an instruction that makes illegal use of the EPSR.\n");
    }

    if (CHECK_BIT(cfsr, UNDEFINSTR)) {
        printf(" - The processor has attempted to execute an undefined instruction.\n");
    }
}

static void
ReportHardFault(void)
{
    uint32_t hfsr = SCB_HFSR;

    printf("\nDetails of the fault status:\n");
    printf("Hard fault status:\n");

    if (CHECK_BIT(hfsr, FORCED))
    {
        printf(" - Forced Hard fault.\n");
    }

    if (CHECK_BIT(hfsr, VECTTBL))
    {
        printf(" - Bus fault on vector table read.\n");
    }
}
