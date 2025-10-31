#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 

const int RETURN_SUCCESS = 0;
const int RETURN_ERROR = 1;
const int RETURN_EOF = EOF;
const int RETURN_MARKER_A_ERROR = 11;
const int RETURN_MARKER_B_ERROR = 12;
const int RETURN_ERROR_HEADER = 21;

static const char* cga_color_names[16] = {
        "Black", "Blue", "Green", "Cyan",
        "Red", "Magenta", "Brown", "Light Gray",
        "Dark Gray", "Bright Blue", "Bright Green", "Bright Cyan",
        "Bright Red", "Bright Magenta", "Yellow", "Bright White"
};

typedef struct {
    /* Bit 5: Palette select for 320x200 mode. */
    /* 0 = Red, Green, Yellow */
    /* 1 = Magenta, Cyan, White */
    int palette;

    /* Bit 4: Bright foreground for 320x200 mode. */
    /* 0 = Normal Intensity */
    /* 1 = High Intensity */
    int bright_foreground;

    /* Bits 3-0: A 4-bit value selecting one of 16 CGA colors. */
    /* Used for Border, Background, or Foreground depending on the mode. */
    int color_value;
} CGA_03D9;

CGA_03D9 parse_cga_color_register(unsigned char register_byte) {
    CGA_03D9 result;
    result.palette = (register_byte >> 5) & 1;
    result.bright_foreground = (register_byte >> 4) & 1;
    result.color_value = register_byte & 0x0F;
    return result;
}

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

int write_unpacked_chunk(FILE *file,PAK_CHUNK *chunk){
    int status; 
    unsigned char *array = malloc(chunk->count);
    memset(array,chunk->value,chunk->count);
    status = fwrite(array,chunk->count,1,file);
    free(array);
    return status==1;
}

int write_packed_chunk(FILE *file,PAK_CHUNK chunk, PAK_HEADER header){
    int status; 
    if(chunk.count==1){
        if(chunk.value!=header.marker_a && chunk.value!=header.marker_b){
            status = putc(chunk.value,file);            
        }else{
            putc(header.marker_b,file);
            putc(1,file);            
            putc(0,file);                        
            status = putc(chunk.value,file);                        
        }
        return status == chunk.value?RETURN_SUCCESS:RETURN_ERROR;      
    }else if(chunk.count<5){
            putc(header.marker_b,file);
            putc(chunk.count,file);            
            putc(0,file);           
            status = putc(chunk.value,file);                        
    }else if(chunk.count<256){
            putc(header.marker_a,file);
            putc(chunk.count-4,file);                       
            status = putc(chunk.value,file);
    }else{
            putc(header.marker_b,file);
            putc(chunk.count&0xff,file);            
            putc(chunk.count>>8,file);          
            status = putc(chunk.value,file);  
    }    
    return status==chunk.value;
}

int read_unpacked_chunk(FILE *file,PAK_CHUNK *chunk){    
    int data;
    chunk->value = getc(file);
    chunk->count = 0;    
    if(chunk->value == EOF){
        return EOF;
    }
    chunk->count = 1;
    do{
        data = getc(file);
        if(data == EOF){
            break;
        }
        if(data == chunk->value){
            chunk->count++;
        }else{
            data = fseek(file,-1,SEEK_CUR);                        
            return data==0?RETURN_SUCCESS:RETURN_ERROR;
        }
    }while(1);    
    return EOF;
}

void show_unpack_error(int status){
    if(status == RETURN_MARKER_A_ERROR){
            printf("Marker A malformed\n");
    }else if (status == RETURN_MARKER_B_ERROR){
            printf("Marker B malformed\n");
    }
}

int main(int argc, char *argv[])
{
    FILE *file_input, *file_output;
    char *file_input_name,*file_output_name;
    int reader,unpacked_index;
    PAK_HEADER header;  
    PAK_CHUNK chunk;     
    CGA_03D9 colors;

    if (argc < 2)
    {
        printf("Big Blue Disk pak format utility. Version 1.0.0 By ImpPossible 2025 \n");
        printf(" paktool file.pak            -- analyze file.pak and show info\n");
        printf(" paktool file.pak file.bin   -- unpack file.pak to file.bin\n");
        printf(" paktool file.pak file.bin 0 -- pack file.bin to file.pak with color byte 0\n");        
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
        colors = parse_cga_color_register(header.color);
        printf("Palette: [%s]",colors.palette?"Magenta, Cyan, White" : "Red, Green, Yellow");
        printf(" Bright: [%s]",colors.bright_foreground?"High Intensity" : "Normal Intensity");
        printf(" Border: [%s]\n",cga_color_names[colors.color_value]);
        printf("RLE Markers: [%i] [%i]\n",header.marker_a,header.marker_b);
        unpacked_index = 0;
        while(1){
            reader = read_chunk(file_input,header,&chunk);
            if(reader == RETURN_SUCCESS){
                unpacked_index = unpacked_index + chunk.count;
            }else if (reader == RETURN_EOF){
                fclose(file_input);
                break;
            }else{
                fclose(file_input);
                show_unpack_error(reader);
                printf("Input file decoded to size %i bytes\n",unpacked_index);                 
                return(RETURN_ERROR);
            }           
        }
        fclose(file_input);
        printf("Input file reached the end, decoded size %i bytes\n",unpacked_index);        
        return(RETURN_SUCCESS);
    }else if(argc < 4){
        file_input_name = argv[1];            
        file_input = fopen(file_input_name, "rb");
        if(file_input == NULL){
            printf("File %s can not be open\n",file_input_name);
            return(RETURN_ERROR);
        }                      
        file_output_name = argv[2];            
        file_output = fopen(file_output_name, "wb");
        if(file_output == NULL){
            printf("File %s can not be open\n",file_output_name);
            return(RETURN_ERROR);
        }            
        reader = fread(&header,sizeof(header),1,file_input);
        if(reader<1){
                printf("File %s too short for PAK format\n",file_input_name);
                return(RETURN_ERROR);
        }
        unpacked_index = 0;
        while(1){
            reader = read_chunk(file_input,header,&chunk);
            if(reader == RETURN_SUCCESS){
                unpacked_index = unpacked_index + chunk.count;
            }else if (reader == RETURN_EOF){
                fclose(file_input);
                fclose(file_output);
                break;
            }else{
                fclose(file_input);
                fclose(file_output);
                show_unpack_error(reader);
                printf("Input file decoded to size %i bytes\n",unpacked_index);                 
                return(RETURN_ERROR);
            }
            if(!write_unpacked_chunk(file_output,&chunk)){
                printf("Output file write error\n"); 
                fclose(file_input);
                fclose(file_output);
                return(RETURN_ERROR);
            }            
        }       
        return(RETURN_SUCCESS);
    }else if(argc < 5){
        file_output_name = argv[1];            
        file_output = fopen(file_output_name, "wb");
        if(file_output == NULL){
            printf("File %s can not be open\n",file_output_name);
            return(RETURN_ERROR);
        }                      
        file_input_name = argv[2];            
        file_input = fopen(file_input_name, "rb");
        if(file_input == NULL){
            printf("File %s can not be open\n",file_input_name);
            return(RETURN_ERROR);
        }   
        header.color = atoi(argv[3]);
        header.marker_a =  0xf0;
        header.marker_b =  0x05;
        reader = fwrite(&header,sizeof(header),1,file_output);
        if(reader!=1){
            printf("File %s can not be writen\n",file_output_name);
            fclose(file_input);
            fclose(file_output);
            return(RETURN_ERROR_HEADER);
        }
        do{
            reader = read_unpacked_chunk(file_input,&chunk);
            if(chunk.count>0){
                write_packed_chunk(file_output,chunk,header);
            }
        }while(reader==RETURN_SUCCESS);
        fclose(file_input);
        fclose(file_output);
    }
    return(RETURN_SUCCESS);
}