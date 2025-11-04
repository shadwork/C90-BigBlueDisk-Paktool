#pragma inline
#include "cga_dos.h"
#include <dos.h>

unsigned char far *memory;

void set_mode_text80()
{
    union REGS regs;
    regs.h.ah = 0;
    regs.h.al = 3;
    int86(0x10,&regs,&regs);
}

void set_mode_cga320(){
    union REGS regs;
    regs.h.ah = 0;
    regs.h.al = 4;
    int86(0x10,&regs,&regs);
}

void set_color_reg(unsigned char value){
    outportb(0x3d9,value);
}

void cls(void)
{
    memset(memory, 0, 16000);
}

void fill(int fill_byte)
{
    memset(memory, (unsigned char)fill_byte, 16000);
}

void init_cga(){
    memory = MK_FP(0xB800,0);	
}

void destroy_cga(){

}