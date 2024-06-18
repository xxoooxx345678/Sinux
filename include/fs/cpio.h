#ifndef CPIO_H
#define CPIO_H

#include <drivers/uart.h>
#include <utils.h>
#include <string.h>

struct cpio_newc_header 
{
    char c_magic[6];            //magic   The string	"070701".
    char c_ino[8];
    char c_mode[8];
    char c_uid[8];
    char c_gid[8];
    char c_nlink[8];
    char c_mtime[8];
    char c_filesize[8];
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8];
    char c_check[8];            //check   This field is always set to zero by writers and ignored by	readers.  
};

int cpio_newc_parse_header(struct cpio_newc_header *this_header_pointer,
        char **pathname, unsigned int *filesize, char **data,
        struct cpio_newc_header **next_header_pointer);

int ls(char* working_dir);
int cat(char* thefilepath);
char* get_file_start(char *thefilepath);
unsigned int get_file_size(char *thefilepath);

#endif