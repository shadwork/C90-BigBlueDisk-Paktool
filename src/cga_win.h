#ifndef CGA_WIN_H
#define CGA_WIN_H

/* * Public globals.
 * These are DEFINED in cga_win.c and used by pakview.c
 */

/* The 16,000 byte CGA video buffer */
extern unsigned char *memory;

/* The 3D9h Color Select Register */
extern unsigned char color;

extern int key;

/* * Public functions.
 * These are called by pakview.c
 */

void set_mode_cga320();

void set_mode_text80();

void set_color_reg(unsigned char value);

void init_cga();
void destroy_cga();

void cls(void);

void fill(int fill_byte);


#endif /* CGA_WIN_H */