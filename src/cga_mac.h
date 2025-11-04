/* ============================================================================
   FILE: cga_mac.h
   ============================================================================ */
#ifndef CGA_MAC_H
#define CGA_MAC_H

/* Public globals */
extern unsigned char *memory;
extern unsigned char color;
extern int key;

/* Public functions */
void set_mode_cga320(void);
void set_mode_text80(void);
void set_color_reg(unsigned char value);
void init_cga(void);
void destroy_cga(void);
void cls(void);
void fill(int fill_byte);

#endif /* CGA_MAC_H */