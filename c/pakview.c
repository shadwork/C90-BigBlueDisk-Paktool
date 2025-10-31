#include <stdio.h>
#include <stdlib.h>
#ifdef WINDOWS
#include "cga_win.h"
#else
#include "cga_dos.h"
#endif

unsigned char inkey;

const int RETURN_SUCCESS = 0;
const int RETURN_ERROR = 1;
const int RETURN_EOF = EOF;
const int RETURN_MARKER_A_ERROR = 11;
const int RETURN_MARKER_B_ERROR = 12;

typedef struct {
    unsigned char color;
    unsigned char marker_a;
    unsigned char marker_b;
} PAK_HEADER;

typedef struct {
    int count;
    unsigned char value;    
} PAK_CHUNK;

int read_chunk(FILE *file,PAK_HEADER header,PAK_CHUNK *chunk){
    int read_byte,rle_count_byte,rle_data_byte;
    unsigned char read_word[2]; 

    read_byte = fgetc(file);
    if(read_byte==EOF){
        return RETURN_EOF;
    };
    if(read_byte == header.marker_a){     
        rle_count_byte = fgetc(file);
        if(rle_count_byte==EOF){
            return(RETURN_MARKER_A_ERROR);
        };
        rle_data_byte = fgetc(file);
        if(rle_data_byte==EOF){
            return(RETURN_MARKER_A_ERROR);
        };
        chunk->count = rle_count_byte + 4;
        chunk->value = rle_data_byte;                
    }else if(read_byte == header.marker_b){
        read_byte = fread(read_word,2,1,file);     
        if(read_byte<1){
            return(RETURN_MARKER_B_ERROR);
        };        
        rle_data_byte = fgetc(file);
        if(rle_data_byte==EOF){
            return(RETURN_MARKER_B_ERROR);
        };
        chunk->count = read_word[0] + 256*read_word[1];
        chunk->value = rle_data_byte;                               
    }else{      
        chunk->count = 1;
        chunk->value = read_byte;  
    } 
    return RETURN_SUCCESS;            
}

void show_unpack_error(int status){
    if(status == RETURN_MARKER_A_ERROR){
            printf("Marker A malformed\n");
    }else if (status == RETURN_MARKER_B_ERROR){
            printf("Marker B malformed\n");
    }
}

/* Standard C90 main entry point for a console application */
int main(int argc, char* argv[])
{
    FILE *file_input;
    char *file_input_name;
    int reader,unpacked_index;
    PAK_HEADER header;  
    PAK_CHUNK chunk;
    int ptr,line,bank,index,fill_start, fill_stop;     

    if (argc < 2)
    {
        printf("pakview file.pak            -- switch to CGA and show unpacked image\n");        
        return(RETURN_SUCCESS);
    }else if(argc < 3)
    {
        file_input_name = argv[1];            
        file_input = fopen(file_input_name, "rb");
        if(file_input == NULL){
            printf("File %s can not be open\n",file_input_name);
            return(RETURN_ERROR);
        }        
        reader = fread(&header,sizeof(header),1,file_input);
        if(reader<1){
                printf("File %s too short for PAK format\n",file_input_name);
                return(RETURN_ERROR);
        }
        unpacked_index = 0;
        init_cga();
        set_mode_cga320();
        set_color_reg(header.color);
        set_color_reg(21);        
        while(1){
            reader = read_chunk(file_input,header,&chunk);
            if(reader == RETURN_SUCCESS){
                fill_start = unpacked_index;
                fill_stop = unpacked_index+chunk.count;
                if(fill_stop>16000){
                    fill_stop = 16000;
                }
                for(index = fill_start;index<fill_stop;index++){
                    ptr = index % 80;
                    line = index / 80;
                    bank = (line&1)*8192;
                    memory[80*(line/2)+bank +ptr] = chunk.value;
                }
                unpacked_index = unpacked_index + chunk.count;                
            }else if (reader == RETURN_EOF){
                inkey = getchar();             
                fclose(file_input);                
                break;
            }else{
                fclose(file_input);
                set_mode_text80();          
                destroy_cga();      
                show_unpack_error(reader);                
                return(RETURN_ERROR);
            }
            if(unpacked_index>=16000){
                inkey = getchar();
                set_mode_text80();
                destroy_cga();
                fclose(file_input);
                return(RETURN_SUCCESS);
            };            
        }
        set_mode_text80();
        destroy_cga();
        return(RETURN_SUCCESS);
    }
    set_mode_text80();
    destroy_cga();
    return(RETURN_SUCCESS);
}

