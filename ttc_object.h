//
//  ttc_object.h
//  ttc2ttf
//
//  Created by wonderidea on 2/19/19.
//  Copyright © 2019 wonderidea. All rights reserved.
//

#ifndef ttc_object_h
#define ttc_object_h

#include <stdio.h>

#ifndef uint16
typedef short uint16;
#endif

#ifndef uint32
typedef unsigned int uint32;
#endif

typedef struct tag_TTC_Header
{
    char tag[4];
    uint16 ver_h;
    uint16 ver_l;
    uint32 directoryCount;
    uint32 *directorys;
}TTC_Header;

typedef struct tag_Sig_Object
{
    char tag[4];
    uint32 length;
    uint32 offset;
}Sig_Object;

typedef struct tag_Offset_Table
{
    char tag[4];  //00 01 00 00 or 'OTTO'
    uint16 numTables;   //19 font table
    uint16 searchRange;  //256 (Maximum power of 2 <= numTables) x 16.
    uint16 entrySelector;  //4 Log2(maximum power of 2 <= numTables).
    uint16 rangeShift;   //48 NumTables x 16-searchRange.
}Offset_Table;


typedef struct tag_Font_Table
{
    char tag[4];
    uint32 checkSum;
    uint32 offset;
    uint32 length;
}Font_Table;

typedef struct tag_Font_Directory
{
    Offset_Table offset_table;
    Font_Table **font_tables;
}Font_Directory;

typedef struct tag_Font_Object
{
    int file_count;
    char *font_file[10];
    FILE *rfp;
    TTC_Header *header;
    Sig_Object *sig;
    Font_Directory **font_dirs;
}Font_Object;

uint16 get_uint16_value(char str[2]);
uint32 get_uint32_value(char str[4]);
void uint16_to_be(char str[2],uint16 value);
void uint32_to_be(char str[4],uint32 value);
uint32 fw_ceil4(uint32 length);
void ttc2ttf(const char *from_file);

Font_Object *font_object_alloc(const char *file);

//unpack
void font_object_unpack(Font_Object *_font,const char *to_file,int index);

//pack
void font_object_add_font(Font_Object *_font,const char *ttf_file);
void font_object_pack(Font_Object *_font,const char *ttc_file);

void font_object_free(Font_Object *obj);

#endif /* ttc_object_h */
