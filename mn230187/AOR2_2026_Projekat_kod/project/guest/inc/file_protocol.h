#ifndef FILE_PROTOCOL_H
#define FILE_PROTOCOL_H

#define FILE_IO_PORT 0x0278

/* Operacije koje guest traži od hipervizora */
#define FILE_OP_OPEN   1
#define FILE_OP_CLOSE  2
#define FILE_OP_READ   3
#define FILE_OP_WRITE  4
#define FILE_OP_LSEEK  5

/* Indikatori za open */
#define FILE_O_RD      1
#define FILE_O_WR      2
#define FILE_O_RDWR    4
#define FILE_O_CREATE  8

/* Indikatori za lseek */
#define FILE_SEEK_SET 1
#define FILE_SEEK_END 2

#endif