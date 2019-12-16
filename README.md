# arm-hard-fault-handler

See original sourcee [here](https://github.com/ferenc-nemeth/arm-hard-fault-handler/blob/master/README.md)

### How to use it
Attach this repo as a submodule to your project and include fault_handler.c file into your build system.
If this is not an option for you then just copy fault_handler.c to your project.

In order this fault handler to do the job config file (fault_config.h) shall be provided.
Minimalistic config may look like this:
```c
#define HARD_FAULT_SYMBOL hard_fault_handler

#define FAULT_PRINTLN(VAR)           puts(VAR)
#define FAULT_PRINT(VAR)             printf(VAR)
#define FAULT_PRINT_HEX(VAR)         printf("0x%08lX", VAR)
#define FAULT_NEWLINE()              puts(" ")

#define FAULT_STOP

``` 
In this case `hard_fault_handler` is the function name of the handler function for Hard Fault.
This name comes from libopencm3 sources. If, for example, you use stm32 and cube libraries, the name
shall be `HardFault_Handler`. See interrupt vector table of your project for correct symbol names.

More advanced config looks like this:  

```c
#define MEMMANAGE_FAULT_SYMBOL mem_manage_handler

#define HARD_FAULT_SYMBOL hard_fault_handler

#define BUS_FAULT_SYMBOL bus_fault_handler

#define USAGE_FAULT_SYMBOL usage_fault_handler

#define FAULT_PRINTLN(VAR)           puts(VAR)
#define FAULT_PRINT(VAR)             printf(VAR)
#define FAULT_PRINT_HEX(VAR)         printf("0x%08lX", VAR)
#define FAULT_NEWLINE()              puts(" ")

#define FAULT_BREAKPOINT
#define FAULT_STOP
``` 
In this case you shall enable and configure not only hard fault, but other faults:
- bus fault
- memory management fault
- usage fault

If `FAULT_STOP` is defined, then function will not leave the fault handler and loop until you reboot the controller. 
If `FAULT_BREAK` is defined, then `bkpt` instruction is inserted at the end of each handler and breakpoint will be automatically
hit in your debugger view. Notice, that if no debugger connected and `bkpt` instuction is executed it will cause harf fault again.
`FAULT_PRINT...` macros are used for printing handler output. They shall alias some sort of logging functions (ITM trace or UART output).
Using any FS logging or functions that rely on DMA or interrupts for this purpose is bad idea - they may not work when the system is processing fault interrupt. 