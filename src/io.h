#ifndef IO_H
#define IO_H

#include "basic.h"

#define STB_SPRINTF_MIN 512
#include "stb/stb_sprintf.h"
#include "stdarg.h"

// Note: Posix only for now
struct Print_Buffer
{
    int fd;
    Dynamic_Array<byte> buffer;
};

Print_Buffer make_print_buffer(int fd, u64 buffer_size = 1024);
Print_Buffer make_file_print_buffer(const byte *file_name, u64 buffer_size = 1024);
void init_std_print_buffers(u64 stdout_size = 1024, u64 stderr_size = 1024);

void print(const byte *fmt, ...);
void print_err(const byte *fmt, ...);
void print_buf(Print_Buffer *pb, const byte *fmt, ...);

void flush_buffer(Print_Buffer *pb);
void free_buffer(Print_Buffer *pb, bool close_file = true);

extern Print_Buffer stdout_buf;
extern Print_Buffer stderr_buf;

#endif // IO_H