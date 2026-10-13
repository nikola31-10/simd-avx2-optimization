#ifndef FILES_H
#define FILES_H

#include "file_protocol.h"

int open(const char *path, int flags);
int close(int fd);
int read(int fd, char *buf, int count);
int write(int fd, const char *buf, int count);
int lseek(int fd, const int offset, int off_flag);

#endif