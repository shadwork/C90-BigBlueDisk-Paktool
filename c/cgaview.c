#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WINDOWS
#include "cga_win.h"
#include "bios_win.h"
#else
#include "cga_dos.h"
#include <bios.h>
#endif

const int RETURN_SUCCESS = 0;
const int RETURN_ERROR = 1;

const int CGA_LINE = 80;

int color_background, color_palette;

unsigned char get_cga_color_register(unsigned char color, unsigned char palette)
{    
    /*
     * Register Layout:
     * Bit 7: Unused (0)
     * Bit 6: Unused (0)
     * Bit 5: Palette (0 or 1)
     * Bit 4: Unused (0)
     * Bit 3: Intensity (0 or 1)
     * Bit 2: Background R (from 'color')
     * Bit 1: Background G (from 'color')
     * Bit 0: Background B (from 'color')
     */

    return (color & 0x0f)  |  ((palette & 1) << 5);
}

/* Standard C90 main entry point for a console application */
int main(int argc, char* argv[])
{
    FILE *file_input;
    char *file_input_name; 
    int read_counter,current_line;
    unsigned char *line;
    int index,inkey;
    line = (unsigned char*)malloc(CGA_LINE);
    color_background = 0;
    color_palette = 0;

    if (argc < 2)
    {
        printf("cgaview file.pak            -- switch to CGA and show cga image from file\n");        
        return(RETURN_SUCCESS);
    }else if(argc < 3)
    {
        file_input_name = argv[1];            
        file_input = fopen(file_input_name, "rb");
        if(file_input == NULL){
            printf("File %s can not be open\n",file_input_name);
            return(RETURN_ERROR);
        }        
        init_cga();
        set_mode_cga320();
        set_color_reg(get_cga_color_register(color_background,color_palette));
        current_line = 0;
        do{
            read_counter = fread(line,CGA_LINE,1,file_input);
            memcpy(memory + (current_line>>1)*CGA_LINE + (current_line&1)*8192,line,CGA_LINE);
            current_line++;
        }while(read_counter==1 && current_line<200);
        
        fclose(file_input);

        do{
            while(bioskey (1)==0);            
            inkey = bioskey (0);            
            if(inkey == 0x4d00){
                color_palette++;
            }else if(inkey == 0x4b00){
                color_palette--;
            }else if(inkey == 0x4800){
                color_background++;
            }else if(inkey == 0x5000){
                color_background--;
            }

            color_palette = color_palette<0?0:color_palette;
            color_palette = color_palette>1?1:color_palette;

            color_background = color_background<0?0:color_background;
            color_background = color_background>15?15:color_background;

            set_color_reg(get_cga_color_register(color_background,color_palette));
            

        }while(inkey != 0x011b);

        return RETURN_SUCCESS;
    }   
    set_mode_text80();
    destroy_cga();
    return(RETURN_SUCCESS);
}