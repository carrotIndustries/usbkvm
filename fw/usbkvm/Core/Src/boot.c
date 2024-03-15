#include "boot.h"
#include <stdint.h>
#include "stm32f0xx.h"

static uint32_t bootflag __attribute__ ((section (".noinit")));
#define BOOTFLAG 0x1badb007

void enter_bootloader()
{
    bootflag = BOOTFLAG;
    NVIC_SystemReset();
    while(1);
}

void jump_to_bootloader()
{
    if(bootflag != BOOTFLAG)
        return;
    void (*jump_target)(void);
    uint32_t addr = 0x1FFFC400;
    jump_target= (void (*)(void)) (*((uint32_t *)(addr + 4)));
    __set_MSP(*(uint32_t *)addr);
	jump_target();
}
