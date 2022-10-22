#ifndef program_H
#define program_H

extern void *DTB_START;

void exec(char *addr);
int loadp(char *thefilepath);

#endif