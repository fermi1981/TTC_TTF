//
//  main.c
//  ttc2ttf
//
//  Created by wonderidea on 2/19/19.
//  Copyright © 2019 wonderidea. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ttc_object.h"


int main(int argc, const char * argv[]) {
	
    int help=1;
    if(argc>=3)
    {
        const char *cmd = argv[1];
        if(!strcmp(cmd,"--unpack"))
        {
            const char *from_file = argv[2];
            
            //ttc2ttf(from_file);
            char path[2048]={0};
            char file_name[100]={0};
            memcpy(path, from_file, strlen(from_file));
            char *name=strrchr(path, '/');
            *name=0;
            name++;
            memcpy(file_name, name, strlen(name));
            char *dot=strrchr(file_name, '.');
            *dot = 0;
            
            printf("path=%s\n",path);
            printf("file name=%s\n",file_name);
            
            //unpack
            Font_Object *_font = font_object_alloc(from_file);
            if (_font) {
                int dir_count = _font->header->directoryCount;
                char to_file[2048];
                for (int i=0; i<dir_count; i++) {
                    memset(to_file, 0, 2048);
                    sprintf(to_file, "%s/%s_test_%d.ttf",path,file_name,i);
                    font_object_unpack(_font, to_file, i);
                }
                font_object_free(_font);
                help = 0;
            }
        }
        else if(!strcmp(cmd,"--pack"))
        {
            
            //pack (没有优化)
            Font_Object *_font = font_object_alloc(NULL);
            if (_font)
            {
                //暂时只添加三个ttf做测试用，有需要的修改这里
                font_object_add_font(_font, argv[2]);
                font_object_add_font(_font, argv[3]);
                font_object_add_font(_font, argv[4]);
                
                font_object_pack(_font, argv[5]);
                font_object_free(_font);
                help = 0;
            }
        }
    }
    
    if (help) {
        printf("Usage:\n");
        printf("{exe} --unpack {ttc_file}\n");
        printf("{exe} --pack {ttf_file1} {ttf_file2} {ttf_file3} {ttc_file}\n");
    }
    
    return 0;
}
