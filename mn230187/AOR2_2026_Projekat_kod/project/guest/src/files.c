#include "files.h"
#include "io.h"

#include <stddef.h>
#include <stdint.h>

int open(const char *path, int flags)
{
	uint32_t length = 0;

	if (path == NULL)
		return -1;

	while (path[length] != '\0')
		length++;

	outl(FILE_IO_PORT, FILE_OP_OPEN);
	outl(FILE_IO_PORT, (uint32_t)flags);
	outl(FILE_IO_PORT, length);

	for (uint32_t i = 0; i < length; i++)
		outb(FILE_IO_PORT, (uint8_t)path[i]);

	return (int32_t)inl(FILE_IO_PORT);
}
int close(int fd)
{
	outl(FILE_IO_PORT, FILE_OP_CLOSE);
	outl(FILE_IO_PORT, (uint32_t)fd);

	return (int32_t)inl(FILE_IO_PORT);
}
int write(int fd, const char *buf, int count)
{
	int i;

	if (buf == NULL || count < 0)
		return -1;

	outl(FILE_IO_PORT, FILE_OP_WRITE);
	outl(FILE_IO_PORT, (uint32_t)fd);
	outl(FILE_IO_PORT, (uint32_t)count);

	for (i = 0; i < count; i++)
		outb(FILE_IO_PORT, (uint8_t)buf[i]);

	return (int32_t)inl(FILE_IO_PORT);
}
int read(int fd, char *buf, int count)
{
	int result;
	int i;

	if (buf == NULL || count < 0)
		return -1;

	outl(FILE_IO_PORT, FILE_OP_READ);
	outl(FILE_IO_PORT, (uint32_t)fd);
	outl(FILE_IO_PORT, (uint32_t)count);

	result = (int32_t)inl(FILE_IO_PORT);

	if (result < 0)
		return -1;

	for (i = 0; i < result; i++)
		buf[i] = (char)inb(FILE_IO_PORT);

	return result;
}
int lseek(int fd, const int offset, int off_flag)
{
	outl(FILE_IO_PORT, FILE_OP_LSEEK);
	outl(FILE_IO_PORT, (uint32_t)fd);
	outl(FILE_IO_PORT, (uint32_t)offset);
	outl(FILE_IO_PORT, (uint32_t)off_flag);

	return (int32_t)inl(FILE_IO_PORT);
}