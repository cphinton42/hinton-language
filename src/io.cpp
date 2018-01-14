
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// TODO: defer
// would be nice for e.g. close(fd)

String read_entire_file(const byte *file_name)
{
    String result = {0};
    
    int fd = open(file_name, O_CLOEXEC | O_RDONLY, 0);
    if(fd < 0)
    {
        return result;
    }
    
    struct stat file_info;
    
    int status = fstat(fd, &file_info);
    if(status < 0)
    {
        close(fd);
        return result;
    }
    
    u64 len = file_info.st_size;
    
    byte *memory = mem_alloc(byte, len+1);
    if(!memory)
    {
        close(fd);
        return result;
    }
    
    u64 bytes_read = read(fd, memory, len);
    if(bytes_read < len)
    {
        close(fd);
        mem_dealloc(memory, len+1);
        return result;
    }
    
    result.count = len+1;
    result.data = memory;
    result[len] = '\0';
    
    close(fd);
    return result;
}


Print_Buffer make_print_buffer(int fd, u64 buffer_size)
{
    Print_Buffer result;
    result.fd = fd;
    result.buffer.count = 0;
    result.buffer.data = mem_alloc(byte, buffer_size);
    result.buffer.allocated = buffer_size;
    
    assert(result.buffer.data);
    
    return result;
}

Print_Buffer make_file_print_buffer(const byte *file_name, u64 buffer_size)
{
    // Note: let umask decide the file permissions (except execute)
    int file = open(file_name, O_CLOEXEC | O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    
    if(file < 0)
    {
        Print_Buffer result = {0};
        result.fd = file;
        return result;
    }
    else
    {
        return make_print_buffer(file, buffer_size);
    }
}

Print_Buffer stdout_buf;
Print_Buffer stderr_buf;

void init_std_print_buffers(u64 stdout_size, u64 stderr_size)
{
    stdout_buf = make_print_buffer(1, stdout_size);
    stderr_buf = make_print_buffer(2, stderr_size);
}



internal inline byte *get_printable_memory(Print_Buffer *pb)
{
    if(pb->buffer.count + STB_SPRINTF_MIN < pb->buffer.allocated)
    {
        flush_buffer(pb);
    }
    return &pb->buffer[pb->buffer.count];
}

byte *print_callback(byte * formatted_buffer, void *user, s32 len)
{
    Print_Buffer *pb = static_cast<Print_Buffer*>(user);
    pb->buffer.count += len;
    return get_printable_memory(pb);
}


internal inline void vprint_buf(Print_Buffer *pb, const byte *fmt, va_list args)
{
    stbsp_vsprintfcb(print_callback, pb, get_printable_memory(pb), fmt, args);
    flush_buffer(pb); // TODO: control flushing behavior
}

void print(const byte *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprint_buf(&stdout_buf, fmt, args);
    va_end(args);
}

void print_err(const byte *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprint_buf(&stderr_buf, fmt, args);
    va_end(args);
}

void print_buf(Print_Buffer *pb, const byte *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprint_buf(pb, fmt, args);
    va_end(args);
}


void flush_buffer(Print_Buffer *pb)
{
    assert(pb->fd >= 0);
    assert(pb->buffer.data);
    
    if(pb->buffer.count)
    {
        // TODO: handle error and/or return error code ?
        s64 result = write(pb->fd, pb->buffer.data, pb->buffer.count);
        pb->buffer.count = 0;
    }
}

void free_buffer(Print_Buffer *pb, bool close_file)
{
    if(close_file && pb->fd >= 0)
    {
        close(pb->fd);
        pb->fd = -1;
    }
    if(pb->buffer.data)
    {
        mem_dealloc(pb->buffer.data, pb->buffer.allocated);
        zero_struct(&pb->buffer);
    }
}
