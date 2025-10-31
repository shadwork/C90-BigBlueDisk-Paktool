#ifndef CGA_DOS_H
#define CGA_DOS_H

extern unsigned char far *memory;

void set_mode_cga320();
void set_mode_text80();
void set_color_reg(unsigned char value);
void cls(void);
void fill(int fill_byte);

#endif /* CGA_WIN_H */