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

#include <stdint.h>

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

/* Registers */
#define HFSR         (*((uint32_t*)0xe000ed2c))
#define CFSR         (*((uint32_t*)0xe000ed28))
#define MMFAR        (*((uint32_t*)0xe000ed34))
#define BFAR         (*((uint32_t*)0xe000ed38))
#define AFSR         (*((uint32_t*)0xe000ed3c))
#define DHCSR        (*((uint32_t*)0xe000edf0))
#define AIRCR        (*((uint32_t*)0xe000ed0c))

/* Application Interrupt and Reset Control Register */
#define AIRCR_RESETREQ      ((uint32_t)0x05fa0040)

/* Debug Halting Control and Status Register */
#define DEBUGEN             ((uint8_t)1u)

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
#ifdef FAULT_BREAKPOINT
    /* Breakpoint*/
    __asm volatile("BKPT #0");
#endif

#ifdef FAULT_REBOOT
    /* Reboot system */
    AIRCR = AIRCR_RESETREQ;
#endif

#ifdef FAULT_STOP
    /* Infinite loop to stop the execution. */
    while(1);
#endif
}


#ifdef MEMMANAGE_FAULT_SYMBOL
void
MEMMANAGE_FAULT_SYMBOL(void)
{
    REPORT_STACK_FRAME
    ReportMemanageFault();
#ifdef MEMMANAGE_FAULT_HOOK
    MEMMANAGE_FAULT_HOOK()
#endif
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
#ifdef HARD_FAULT_HOOK
    HARD_FAULT_HOOK()
#endif
    HaltExecution();
}
#endif

#ifdef BUS_FAULT_SYMBOL
void
BUS_FAULT_SYMBOL(void)
{
    REPORT_STACK_FRAME
    ReportBusFault();
#ifdef BUS_FAULT_HOOK
    BUS_FAULT_HOOK()
#endif
    HaltExecution();
}
#endif

#ifdef USAGE_FAULT_SYMBOL
void
USAGE_FAULT_SYMBOL(void)
{
    REPORT_STACK_FRAME
    ReportUsageFault();
#ifdef USAGE_FAULT_HOOK
    USAGE_FAULT_HOOK()
#endif
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
  uint32_t hfsr = HFSR;
  uint32_t cfsr = CFSR;
  uint32_t mmar = MMFAR;
  uint32_t bfar = BFAR;
  uint32_t afsr = AFSR;
  FAULT_PRINTLN("!!!Fault detected!!!");

  FAULT_PRINTLN("Stack frame:");
  FAULT_PRINT("R0 :    "); FAULT_PRINT_HEX(r0); FAULT_NEWLINE();
  FAULT_PRINT("R1 :    "); FAULT_PRINT_HEX(r1); FAULT_NEWLINE();
  FAULT_PRINT("R2 :    "); FAULT_PRINT_HEX(r2); FAULT_NEWLINE();
  FAULT_PRINT("R3 :    "); FAULT_PRINT_HEX(r3); FAULT_NEWLINE();
  FAULT_PRINT("R12:    "); FAULT_PRINT_HEX(r12); FAULT_NEWLINE();
  FAULT_PRINT("LR :    "); FAULT_PRINT_HEX(lr); FAULT_NEWLINE();
  FAULT_PRINT("PC :    "); FAULT_PRINT_HEX(pc); FAULT_NEWLINE();
  FAULT_PRINT("PSR:    "); FAULT_PRINT_HEX(psr); FAULT_NEWLINE();

  FAULT_PRINTLN("Fault status:");
  FAULT_PRINT("HFSR:    "); FAULT_PRINT_HEX(hfsr); FAULT_NEWLINE();
  FAULT_PRINT("CFSR:    "); FAULT_PRINT_HEX(cfsr); FAULT_NEWLINE();
  FAULT_PRINT("MMAR:    "); FAULT_PRINT_HEX(mmar); FAULT_NEWLINE();
  FAULT_PRINT("BFAR:    "); FAULT_PRINT_HEX(bfar); FAULT_NEWLINE();
  FAULT_PRINT("AFSR:    "); FAULT_PRINT_HEX(afsr); FAULT_NEWLINE();

  FAULT_PRINTLN("Other:");
  FAULT_PRINT("EXC_RETURN: "); FAULT_PRINT_HEX(exc); FAULT_NEWLINE();
}

static void
ReportMemanageFault(void)
{
    uint32_t cfsr = CFSR;

    FAULT_PRINTLN("MemManage fault status:");

    if (CHECK_BIT(cfsr, MMARVALID)) {
        FAULT_PRINTLN(" - MMAR holds a valid address.");
    } else {
        FAULT_PRINTLN(" - MMAR holds an invalid address.");
    }

    if (CHECK_BIT(cfsr, MLSPERR)) {
        FAULT_PRINTLN(" - Fault occurred during floating-point lazy state preservation.");
    }

    if (CHECK_BIT(cfsr, MSTKERR)) {
        FAULT_PRINTLN(" - Stacking has caused an access violation.");
    }

    if (CHECK_BIT(cfsr, MUNSTKERR)) {
        FAULT_PRINTLN(" - Unstacking has caused an access violation.");
    }

    if (CHECK_BIT(cfsr, DACCVIOL)) {
        FAULT_PRINTLN(" - Load or store at a location that does not permit the operation.");
    }

    if (CHECK_BIT(cfsr, IACCVIOL)) {
        FAULT_PRINTLN(" - Instruction fetch from a location that does not permit execution.");
    }
}

static void
ReportBusFault(void)
{
    uint32_t cfsr = CFSR;

    FAULT_PRINTLN("Bus fault status:\n");

    if (CHECK_BIT(cfsr, BFARVALID)) {
        FAULT_PRINTLN(" - BFAR holds a valid address.");
    } else {
        FAULT_PRINTLN(" - BFAR holds an invalid address.");
    }

    if (CHECK_BIT(cfsr, LSPERR)) {
        FAULT_PRINTLN(" - Fault occurred during floating-point lazy state preservation.");
    }

    if (CHECK_BIT(cfsr, STKERR)) {
        FAULT_PRINTLN(" - Stacking has caused a Bus fault.");
    }

    if (CHECK_BIT(cfsr, UNSTKERR)) {
        FAULT_PRINTLN(" - Unstacking has caused a Bus fault.");
    }

    if (CHECK_BIT(cfsr, IMPRECISERR)) {
        FAULT_PRINTLN(" - Data bus error has occurred, but the return address in the stack is not related to the fault.");
    }

    if (CHECK_BIT(cfsr, PRECISERR)) {
        FAULT_PRINTLN(" - Data bus error has occurred, and the return address points to the instruction that caused the fault.");
    }

    if (CHECK_BIT(cfsr, IBUSERR)) {
        FAULT_PRINTLN(" - Instruction bus error.");
    }
}

static void
ReportUsageFault(void)
{
    uint32_t cfsr = CFSR;

    FAULT_PRINTLN("Usage fault status:");

    if (CHECK_BIT(cfsr, DIVBYZERO)) {
        FAULT_PRINTLN(" - The processor has executed an SDIV or UDIV instruction with a divisor of 0.");
    }

    if (CHECK_BIT(cfsr, UNALIGNED)) {
        FAULT_PRINTLN(" - The processor has made an unaligned memory access.");
    }

    if (CHECK_BIT(cfsr, NOCP)) {
        FAULT_PRINTLN(" - Attempted to access a coprocessor.");
    }

    if (CHECK_BIT(cfsr, INVPC)) {
        FAULT_PRINTLN(" - Illegal attempt to load of EXC_RETURN to the PC.");
    }

    if (CHECK_BIT(cfsr, INVSTATE)) {
        FAULT_PRINTLN(" - Attempted to execute an instruction that makes illegal use of the EPSR.");
    }

    if (CHECK_BIT(cfsr, UNDEFINSTR)) {
        FAULT_PRINTLN(" - The processor has attempted to execute an undefined instruction.");
    }
}

static void
ReportHardFault(void)
{
    uint32_t hfsr = HFSR;

    FAULT_PRINTLN("Hard fault status:");

    if (CHECK_BIT(hfsr, FORCED)) {
        FAULT_PRINTLN(" - Forced Hard fault.");
    }

    if (CHECK_BIT(hfsr, VECTTBL)) {
        FAULT_PRINTLN(" - Bus fault on vector table read.");
    }
}
