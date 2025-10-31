#include "bios_win.h"

int key_stored = 0;

int	bioskey(int cmd){
    int ret_value;
    switch (cmd)
    {
    case 0:        
        ret_value = key_stored;
        key_stored = 0;
        key=0;
        return ret_value;
        break;
    case 1:
        if(key!=0){
            key_stored = key;
        }
        return key_stored;
        break;
    case 2:
        return 0;
        break;            
    default:
        break;
    }
    return 0;
}