//
//  ttc_object.c
//  ttc2ttf
//
//  Created by wonderidea on 2/19/19.
//  Copyright © 2019 wonderidea. All rights reserved.
//

#include "ttc_object.h"
#include <stdlib.h>
#include <string.h>


uint16 get_uint16_value(char str[2])
{
    uint16 tmp = str[0];
    tmp <<= 8;
    tmp |= (str[1]&0xFF);
    return tmp;
}

uint32 get_uint32_value(char str[4])
{
    uint32 tmp = 0;
    for (int i=0; i<4; i++) {
        tmp <<= 8;
        tmp |= (str[i]&0xFF);
    }
    return tmp;
}

void uint16_to_be(char str[2],uint16 value)
{
    str[0] = value>>8;
    str[1] = value&0xFF;
}

void uint32_to_be(char str[4],uint32 value)
{
    str[0] = (value>>24)&0xFF;
    str[1] = (value>>16)&0xFF;
    str[2] = (value>>8)&0xFF;
    str[3] = value&0xFF;
}

uint32 fw_ceil4(uint32 length)
{
    if (length&3) {
        length += 4 ;
        length &= (0xFFFFFFFF-3);
    }
    return length;
}

static char *string_copy(const char *str)
{
    size_t len = strlen(str);
    char *tmp = (char *)malloc(len+1);
    if (tmp) {
        memset(tmp, 0, len+1);
        memcpy(tmp, str, len);
    }
    return tmp;
}

static uint32 CalcTableChecksum(uint32 *Table, uint32 Length)
{
    uint32 Sum = 0L;
    uint32 *Endptr = Table+((Length+3) & ~3) / sizeof(uint32);
    while (Table < Endptr)
        Sum += *Table++;
    return Sum;
}

static void inner_write_ttc_header(FILE *wfp,TTC_Header *header)
{
    char buf[5]={0};
    fseek(wfp, 0, SEEK_SET);
    fwrite(header->tag, 1, 4, wfp);
    uint16_to_be(buf, header->ver_h);
    fwrite(buf, 1, 2, wfp);
    uint16_to_be(buf, header->ver_l);
    fwrite(buf, 1, 2, wfp);
    uint32_to_be(buf, header->directoryCount);
    fwrite(buf, 1, 4, wfp);
    for (int i=0; i<header->directoryCount; i++) {
        uint32 offset = header->directorys[i];
        uint32_to_be(buf, offset);
        fwrite(buf, 1, 4, wfp);
    }
}

static void inner_write_font_dirs_info(FILE *wfp,Font_Directory *_dir)
{
    Offset_Table offset_table =_dir->offset_table;
    char buf[5]={0};
    fwrite(offset_table.tag, 1, 4, wfp);
    uint16_to_be(buf, offset_table.numTables);
    fwrite(buf, 1, 2, wfp);
    uint16_to_be(buf, offset_table.searchRange);
    fwrite(buf, 1, 2, wfp);
    uint16_to_be(buf, offset_table.entrySelector);
    fwrite(buf, 1, 2, wfp);
    uint16_to_be(buf, offset_table.rangeShift);
    fwrite(buf, 1, 2, wfp);
    //table info
    for (int i=0; i<_dir->offset_table.numTables; i++)
    {
        Font_Table *_tb = _dir->font_tables[i];
        fwrite(_tb->tag, 1, 4, wfp);
        uint32_to_be(buf, _tb->checkSum);
        fwrite(buf, 1, 4, wfp);
        uint32_to_be(buf, _tb->offset);
        fwrite(buf, 1, 4, wfp);
        uint32_to_be(buf, _tb->length);
        fwrite(buf, 1, 4, wfp);
    }
}

static Font_Directory *parse_font_dir_info(Font_Object *_font)
{
    char buffer[2048]={0};
    FILE *rfp = _font->rfp;
    fread(buffer, 1, sizeof(Offset_Table), rfp);
    Font_Directory *_dir = (Font_Directory *)malloc(sizeof(Font_Directory));
    memset(_dir, 0, sizeof(Font_Directory));
    memcpy(_dir->offset_table.tag, buffer, 4);
    _dir->offset_table.numTables = get_uint16_value(buffer+4);
    _dir->offset_table.searchRange = get_uint16_value(buffer+6);
    _dir->offset_table.entrySelector = get_uint16_value(buffer+8);
    _dir->offset_table.rangeShift = get_uint16_value(buffer+10);
    if (_dir->offset_table.numTables)
    {
        _dir->font_tables = (Font_Table **)malloc(_dir->offset_table.numTables*sizeof(Font_Table *));
        for (int i=0; i<_dir->offset_table.numTables; i++)
        {
            fread(buffer, 1, sizeof(Font_Table), rfp);
            Font_Table *_ft = (Font_Table *)malloc(sizeof(Font_Table));
            if (_ft)
            {
                memset(_ft, 0, sizeof(Font_Table));
                memcpy(_ft->tag, buffer, 4);
                _ft->checkSum = get_uint32_value(buffer+4);
                _ft->offset = get_uint32_value(buffer+8);
                _ft->length = get_uint32_value(buffer+12);
            }
            _dir->font_tables[i]=_ft;
        }
    }
    return _dir;
}


static Font_Directory *parse_ttf_dir(const char *file)
{
    char buffer[2048]={0};
    FILE *rfp = fopen(file, "rb");
    fread(buffer, 1, sizeof(Offset_Table), rfp);
    Font_Directory *_dir = (Font_Directory *)malloc(sizeof(Font_Directory));
    memset(_dir, 0, sizeof(Font_Directory));
    memcpy(_dir->offset_table.tag, buffer, 4);
    _dir->offset_table.numTables = get_uint16_value(buffer+4);
    _dir->offset_table.searchRange = get_uint16_value(buffer+6);
    _dir->offset_table.entrySelector = get_uint16_value(buffer+8);
    _dir->offset_table.rangeShift = get_uint16_value(buffer+10);
    if (_dir->offset_table.numTables)
    {
        _dir->font_tables = (Font_Table **)malloc(_dir->offset_table.numTables*sizeof(Font_Table *));
        for (int i=0; i<_dir->offset_table.numTables; i++)
        {
            fread(buffer, 1, sizeof(Font_Table), rfp);
            Font_Table *_ft = (Font_Table *)malloc(sizeof(Font_Table));
            if (_ft)
            {
                memset(_ft, 0, sizeof(Font_Table));
                memcpy(_ft->tag, buffer, 4);
                _ft->checkSum = get_uint32_value(buffer+4);
                _ft->offset = get_uint32_value(buffer+8);
                _ft->length = get_uint32_value(buffer+12);
            }
            _dir->font_tables[i]=_ft;
        }
    }
    fclose(rfp);
    return _dir;
}

static void parse_font_file(const char *file,Font_Object *_font)
{
    if (!file || !_font) {
        return;
    }
    char buffer[2048]={0};
    char tag[5]={0};
    FILE *rfp=NULL;
    rfp = fopen(file, "rb");
    if (rfp) {
        _font->rfp = rfp;
    }
    fseek(rfp, 0, SEEK_SET);
    fread(tag, 1, 4, rfp);
    
    _font->header = (TTC_Header *)malloc(sizeof(TTC_Header));
    if (_font->header) {
        memset(_font->header, 0, sizeof(TTC_Header));  //tag=00,00,00,00 (not ttcf)
    }
    if (!strcmp(tag, "ttcf"))
    {
        //is ttc file
        fseek(rfp, 0, SEEK_SET);
        fread(buffer, 1, 12, rfp);  //tag+version+count
        memcpy(_font->header->tag, buffer, 4);
        _font->header->ver_h = get_uint16_value(buffer+4);
        _font->header->ver_l = get_uint16_value(buffer+6);
        _font->header->directoryCount = get_uint32_value(buffer+8);
        if (_font->header->directoryCount) {
            uint32 *directorys = (uint32 *)malloc(_font->header->directoryCount*sizeof(uint32));
            for (int i=0; i<_font->header->directoryCount; i++)
            {
                fread(buffer, 1, sizeof(uint32), rfp);
                directorys[i]=get_uint32_value(buffer);
            }
            _font->header->directorys = directorys;
            
            fread(buffer, sizeof(Sig_Object), 1, rfp);
            memcpy(tag, buffer, 4);
            if (!strcmp(tag, "DSIG")) {
                //has sig
                _font->sig = (Sig_Object *)malloc(sizeof(Sig_Object));
                memset(_font->sig, 0, sizeof(Sig_Object));
                memcpy(_font->sig->tag, buffer, 4);
                _font->sig->length = get_uint32_value(buffer+4);
                _font->sig->offset = get_uint32_value(buffer+8);
            }
            
            _font->font_dirs = (Font_Directory **)malloc(_font->header->directoryCount*sizeof(Font_Directory *));
            
            for (int i=0; i<_font->header->directoryCount; i++)
            {
                uint32 offset = directorys[i];
                fseek(rfp, offset, SEEK_SET);
                Font_Directory *_dir = parse_font_dir_info(_font);
                _font->font_dirs[i]=_dir;
            }
        }
    }
    else
    {
        //offset table + tables
        int is_ttf=0;
        if (tag[0]==0&&tag[1]==1&&tag[2]==0&&tag[3]==0) {
            is_ttf=1;
        }
        else if (!strcmp(tag, "OTTO") || !strcmp(tag, "otto"))
        {
            is_ttf = 1;
        }
        if (is_ttf)
        {
            _font->header->directoryCount = 1;
            //directorys=NULL;
            fseek(rfp, 0, SEEK_SET);
            _font->font_dirs = (Font_Directory **)malloc(1*sizeof(Font_Directory *));
            if (_font->font_dirs)
            {
                memset(_font->font_dirs, 0, 1*sizeof(Font_Directory *));
                Font_Directory *_dir = parse_font_dir_info(_font);
                _font->font_dirs[0]=_dir;
            }
        }
    }
}

Font_Object *font_object_alloc(const char *file)
{
    Font_Object *_font = (Font_Object *)malloc(sizeof(Font_Object));
    if (_font) {
        memset(_font, 0, sizeof(Font_Object));
        if (file) {
            _font->file_count = 1;
            _font->font_file[0] = string_copy(file);
        }
        parse_font_file(file,_font);
    }
    return _font;
}

static Font_Directory *get_ttf_directory_info(Font_Directory *from_dir)
{
    Font_Directory *_dir = (Font_Directory *)malloc(sizeof(Font_Directory));
    memcpy(&(_dir->offset_table), &(from_dir->offset_table), sizeof(Offset_Table));
    _dir->font_tables = (Font_Table **)malloc(_dir->offset_table.numTables * sizeof(Font_Table *));
    for (int i=0; i<_dir->offset_table.numTables; i++)
    {
        Font_Table *_from_tb = from_dir->font_tables[i];
        Font_Table *_to_tb = (Font_Table *)malloc(sizeof(Font_Table));
        memcpy(_to_tb, _from_tb, sizeof(Font_Table));
        _dir->font_tables[i] = _to_tb;
    }
    return _dir;
}

/* ttf = offset table + N*font table + data*/
void font_object_unpack(Font_Object *_font,const char *to_file,int index)
{
    if (index<_font->header->directoryCount)
    {
        Font_Directory *_dir = get_ttf_directory_info(_font->font_dirs[index]);
        
        FILE *wfp = fopen(to_file, "wb");
        if (wfp) {
            fclose(wfp);
        }
        wfp = fopen(to_file, "r+b");
        //int offset_pos = sizeof(Offset_Table)+_dir->offset_table.numTables*sizeof(Font_Table);
        if (wfp) {
            char buffer[2048]={0};
            inner_write_font_dirs_info(wfp,_dir);
            /*
            //tag
            fwrite(_dir->offset_table.tag, 1, 4, wfp);
            //numTables
            uint16_to_be(buffer, _dir->offset_table.numTables);
            fwrite(buffer, 1, 2, wfp);
            //searchRange
            uint16_to_be(buffer, _dir->offset_table.searchRange);
            fwrite(buffer, 1, 2, wfp);
            //entrySelector
            uint16_to_be(buffer, _dir->offset_table.entrySelector);
            fwrite(buffer, 1, 2, wfp);
            //rangeShift
            uint16_to_be(buffer, _dir->offset_table.rangeShift);
            fwrite(buffer, 1, 2, wfp);
             
            for (int i=0; i<_dir->offset_table.numTables; i++)
            {
                fwrite(_dir->font_tables[i], sizeof(Font_Table), 1, wfp);
            }
             */
            for (int i=0; i<_dir->offset_table.numTables; i++)
            {
                Font_Table *_tb = _dir->font_tables[i];
                _tb->offset = (unsigned int)ftell(wfp);
                Font_Table *_from_tb = _font->font_dirs[index]->font_tables[i];
                fseek(_font->rfp, _from_tb->offset, SEEK_SET);
                /* 这些需要解释一下，数据块的个数必须是4的倍数 */
                int content_len = fw_ceil4(_from_tb->length);
                char *data = (char *)malloc(content_len);
                if (data)
                {
                    memset(data, 0, content_len);
                    fread(data, 1, _from_tb->length, _font->rfp);
                    fwrite(data, 1, content_len, wfp);
                    free(data);
                }
                else
                {
                    while (1)
                    {
                        size_t red_len = fread(buffer, 1, 2048, _font->rfp);
                        if (red_len) {
                            fwrite(buffer, 1, red_len, wfp);
                        }
                        else
                        {
                            break;
                        }
                        if (red_len!=2048) {
                            break;
                        }
                    }
                    if (content_len>_from_tb->length) {
                        memset(buffer, 0, 2048);
                        fwrite(buffer, 1, content_len-_from_tb->length, wfp);
                    }
                }
            }
            fseek(wfp, sizeof(Offset_Table), SEEK_SET);
            //table info
            for (int i=0; i<_dir->offset_table.numTables; i++)
            {
                Font_Table *_tb = _dir->font_tables[i];
                fwrite(_tb->tag, 1, 4, wfp);
                uint32_to_be(buffer, _tb->checkSum);
                fwrite(buffer, 1, 4, wfp);
                uint32_to_be(buffer, _tb->offset);
                fwrite(buffer, 1, 4, wfp);
                uint32_to_be(buffer, _tb->length);
                fwrite(buffer, 1, 4, wfp);
            }
            fclose(wfp);
        }
        
    }
}

void font_object_add_font(Font_Object *_font,const char *ttf_file)
{
    if (!_font || !ttf_file) {
        return;
    }
    _font->font_file[_font->file_count] = string_copy(ttf_file);
    _font->file_count++;
}

static int is_ttc_file(const char *file)
{
    if (!file) {
        return 0;
    }
    int flag = 0;
    char tag[5]={0};
    FILE *rfp = fopen(file, "rb");
    if (rfp)
    {
        fread(tag, 1, 4, rfp);
        if (!strcmp(tag, "ttcf"))
        {
            flag=1;
        }
        fclose(rfp);
    }
    return flag;
}

static int count_ttf_file(Font_Object *_font)
{
    int count = 0;
    char buffer[2048];
    char tag[5]={0};
    for (int i=0; i<_font->file_count; i++) {
        memset(buffer, 0, 2048);
        char *tfile = _font->font_file[i];
        FILE *rfp = fopen(tfile, "rb");
        if (rfp)
        {
            fread(buffer, 1, sizeof(TTC_Header)-sizeof(uint32), rfp);
            //读取12个字节
            memcpy(tag, buffer, 4);
            if (!strcmp(tag, "ttcf"))
            {
                //is ttc
                int numFiles = get_uint32_value(buffer+8);
                count += numFiles;
            }
            else
            {
                count++;
            }
            fclose(rfp);
        }
    }
    return count;
}

static void inner_write_dir(FILE *rfp,Font_Directory *from_dir,FILE *wfp)
{
    if (!rfp || !from_dir || !wfp) {
        return;
    }
    char buffer[2048]={0};
    Font_Directory *to_dir = get_ttf_directory_info(from_dir);
    if (!to_dir) {
        return;
    }
    size_t file_offset = ftell(wfp);
    file_offset += sizeof(Offset_Table) + sizeof(Font_Table)*to_dir->offset_table.numTables;
    for (int i=0; i<to_dir->offset_table.numTables; i++) {
        Font_Table *_tb = to_dir->font_tables[i];
        uint32 at_len = fw_ceil4(_tb->length);
        _tb->offset = (uint32)file_offset;
        file_offset += at_len;
    }
    inner_write_font_dirs_info(wfp, to_dir);
    for (int i=0; i<from_dir->offset_table.numTables; i++) {
        Font_Table *_tb = from_dir->font_tables[i];
        uint32 at_len = fw_ceil4(_tb->length);
        fseek(rfp, _tb->offset, SEEK_SET);
        char *data = (char *)malloc(at_len);
        if (data)
        {
            memset(data, 0, at_len);
            fread(data, 1, _tb->length, rfp);
            fwrite(data, 1, at_len, wfp);
        }
        else
        {
            while (1) {
                size_t read_len = fread(buffer, 1, 2048, rfp);
                if (read_len) {
                    fwrite(buffer, 1, read_len, wfp);
                }
                if (read_len<2048) {
                    break;
                }
            }
            if (at_len>_tb->length) {
                memset(buffer, 0, 2048);
                fwrite(buffer, 1, at_len-_tb->length, wfp);
            }
        }
    }
    for (int i=0; i<to_dir->offset_table.numTables; i++) {
        Font_Table *_tb = to_dir->font_tables[i];
        if (_tb) {
            free(_tb);
        }
        to_dir->font_tables[i] = NULL;
    }
    free(to_dir);
}

void font_object_pack(Font_Object *_font,const char *ttc_file)
{
    //char buffer[2048]={0};
    if (!_font || !ttc_file) {
        return;
    }
    TTC_Header *header = (TTC_Header *)malloc(sizeof(TTC_Header));
    if (!header) {
        return;
    }
    memset(header, 0, sizeof(TTC_Header));
    memcpy(header->tag, "ttcf", 4);
    header->ver_h = 1;
    header->ver_l = 0;
    header->directoryCount = count_ttf_file(_font);
    header->directorys = (uint32 *)malloc(sizeof(uint32)*header->directoryCount);
    if (header->directorys) {
        memset(header->directorys, 0, sizeof(uint32)*header->directoryCount);
    }
    _font->header = header;
    _font->font_dirs = (Font_Directory **)malloc(sizeof(Font_Directory *)*header->directoryCount);
    if (_font->font_dirs) {
        memset(_font->font_dirs, 0, sizeof(Font_Directory *)*header->directoryCount);
    }
    FILE *wfp = fopen(ttc_file, "wb");
    if (wfp) {
        fclose(wfp);
    }
    wfp = fopen(ttc_file, "r+b");
    _font->rfp = wfp;
    //write header
    fseek(wfp, 0, SEEK_SET);
    inner_write_ttc_header(wfp, header);  //font_dirs offset use 0 fill them
    //uint32 offset = header->directoryCount*sizeof(uint32) + sizeof(TTC_Header) - sizeof(uint32);  //header + n*offset
    
    int dir_index = 0;
    for (int i=0; i<_font->file_count; i++) {
        char *file = _font->font_file[i];
        if (file)
        {
            if (is_ttc_file(file))
            {
                Font_Object *tmp_font = font_object_alloc(file);
                if (!tmp_font) {
                    continue;
                }
                for (int j=0; j<tmp_font->header->directoryCount; j++) {
                    Font_Directory *_dir = tmp_font->font_dirs[j];
                    size_t file_offset = ftell(wfp);
                    _font->header->directorys[dir_index]=(uint32)file_offset;
                    inner_write_dir(tmp_font->rfp, _dir, wfp);
                    dir_index++;
                }
                font_object_free(tmp_font);
            }
            else
            {
                Font_Directory *_dir = parse_ttf_dir(file);
                size_t file_offset = ftell(wfp);
                _font->header->directorys[dir_index]=(uint32)file_offset;
                FILE *rfp = fopen(file, "rb");
                if (rfp) {
                    inner_write_dir(rfp, _dir, wfp);
                    fclose(rfp);
                }
                dir_index++;
            }
        }
    }
    
    fseek(wfp, 0, SEEK_SET);
    inner_write_ttc_header(wfp, header);  //font_dirs offset use 0 fill them
}

void font_object_free(Font_Object *_font)
{
    if (_font)
    {
        if (_font->rfp)
        {
            fclose(_font->rfp);
            _font->rfp=NULL;
        }
        if (_font->file_count)
        {
            for (int i=0; i<10; i++)
            {
                char *tfile = _font->font_file[i];
                if (tfile) {
                    free(tfile);
                }
                _font->font_file[i] = NULL;
            }
            _font->file_count = 0;
        }
        int dir_count=0;
        if (_font->header) {
            dir_count = _font->header->directoryCount;
            if (_font->header->directorys) {
                free(_font->header->directorys);
                _font->header->directorys=NULL;
            }
            free(_font->header);
            _font->header=NULL;
        }
        if (_font->sig) {
            free(_font->sig);
            _font->sig=NULL;
        }
        if (_font->font_dirs) {
            if (dir_count)
            {
                for (int i=0; i<dir_count; i++)
                {
                    Font_Directory *_dir = _font->font_dirs[i];
                    if (_dir)
                    {
                        int num_tables = _dir->offset_table.numTables;
                        if (_dir->font_tables)
                        {
                            for (int j=0; j<num_tables; j++)
                            {
                                Font_Table *_tb = _dir->font_tables[j];
                                if (_tb) {
                                    free(_tb);
                                }
                            }
                            free(_dir->font_tables);
                        }
                        free(_dir);
                    }
                    _font->font_dirs[i]=NULL;
                }
            }
            free(_font->font_dirs);
            _font->font_dirs=NULL;
        }
        free(_font);
    }
}

//testing （from python version）
//python version: https://github.com/yhchen/ttc2ttf
void ttc2ttf(const char *from_file)
{
    char buffer[2048]={0};
    char path[2048]={0};
    char file_name[100]={0};
    memcpy(path, from_file, strlen(from_file));
    char *name=strrchr(path, '/');
    *name=0;
    name++;
    memcpy(file_name, name, strlen(name));
    char *dot=strrchr(file_name, '.');
    *dot = 0;
    
    printf("path=%s\n",buffer);
    printf("file name=%s\n",file_name);
    
    char tag[5]={0};
    FILE *rfp = fopen(from_file, "rb");
    if (!rfp) {
        return;
    }
    fread(tag, 1, 4, rfp);
    
    if (!strcmp(tag, "ttcf"))
    {
        fseek(rfp, 0x08, SEEK_SET);
        fread(buffer, 1, sizeof(uint32), rfp);
        int ttf_count = get_uint32_value(buffer);
        printf("Number of included TTF files: %d\n",ttf_count);
        uint32 *ttf_offset_array = (uint32 *)malloc(sizeof(uint32)*ttf_count);
        if (!ttf_offset_array) {
            return;
        }
        fseek(rfp, 0x0C, SEEK_SET);
        fread(buffer, 1, sizeof(uint32)*ttf_count, rfp);
        
        for (int i=0; i<ttf_count; i++) {
            ttf_offset_array[i]=get_uint32_value(buffer+i*4);
        }
        for (int i=0; i<ttf_count; i++)
        {
            printf("Extract TTF #%d:\n",(i+1));
            uint32 table_header_offset = ttf_offset_array[i];
            printf("\tHeader starts at byte %d\n" , table_header_offset);
            fseek(rfp, table_header_offset+0x04, SEEK_SET);
            fread(buffer, 1, sizeof(uint16), rfp);
            short table_count = get_uint16_value(buffer);
            int header_length = sizeof(Offset_Table) + table_count * sizeof(Font_Table);
            printf("\tHeader Length: %d Byte\n" , header_length);
            unsigned long long table_length = 0;
            for (int j=0; j<table_count; j++)
            {
                fseek(rfp, table_header_offset+sizeof(Offset_Table)+j*sizeof(Font_Table)+0x0C, SEEK_SET);
                fread(buffer, 1, 4, rfp);
                unsigned int length = get_uint32_value(buffer);
                table_length += fw_ceil4(length);
            }
            printf("\tTable length: %lld\n" , table_length);
            
            unsigned long long total_length = header_length + table_length;
            
            char *new_buf = malloc(total_length);
            fseek(rfp, table_header_offset, SEEK_SET);
            fread(buffer, 1, header_length, rfp);
            memcpy(new_buf, buffer, header_length);
            int current_offset = header_length;
            
            for (int j=0; j<table_count; j++)
            {
                fseek(rfp, table_header_offset+sizeof(Offset_Table)+j*sizeof(Font_Table), SEEK_SET);
                fread(buffer, sizeof(Font_Table), 1, rfp);
                int offset = get_uint32_value(buffer+0x08);
                int length = get_uint32_value(buffer+0x0C);
                uint32_to_be(buffer, current_offset);
                memcpy(new_buf+sizeof(Offset_Table)+j*sizeof(Font_Table)+0x08, buffer, 4);
                fseek(rfp, offset, SEEK_SET);
                fread(new_buf+current_offset, 1, length, rfp);
                current_offset += fw_ceil4(length);
            }
            char to_file[2048];
            memset(to_file, 0, 2048);
            sprintf(to_file, "%s/%s_%d.ttf",path,file_name,i);
            FILE *wfp = fopen(to_file, "wb");
            if (!wfp) {
                continue;
            }
            fwrite(new_buf, 1, total_length, wfp);
            fclose(wfp);
        }
        free(ttf_offset_array);
    }
    fclose(rfp);
}

