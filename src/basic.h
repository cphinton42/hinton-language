#ifndef BASIC_H
#define BASIC_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

//
// Primitive type typedefs
//

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef char byte;

typedef float f32;
typedef double f64;

template <typename T>
struct Array
{
    u64 count;
    T *data;
    
    T &operator[](u64 idx);
};

typedef Array<byte> String;

template <typename T>
struct Dynamic_Array
{
    union
    {
        Array<T> array;
        struct
        {
            u64 count;
            T *data;
        };
    };
    u64 allocated;
    
    T &operator[](u64 idx);
};

template<typename T>
bool operator==(Array<T> a1, Array<T> a2)
{
    if(a1.count == a2.count)
    {
        for(u64 i = 0; i < a1.count; ++i)
        {
            if(a1[i] != a2[i])
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        return false;
    }
}

template<typename T>
T &Array<T>::operator[](u64 idx)
{
    return this->data[idx];
}

template<typename T>
T &Dynamic_Array<T>::operator[](u64 idx)
{
    return this->data[idx];
}

template<typename T>
Array<T> make_array(u64 count, T *data)
{
    return {count, data};
}

template<typename T>
void array_add(Dynamic_Array<T> *arr, T element)
{
    (*arr)[arr->count] = element;
    ++arr->count;
}

template<typename T>
void array_add(Dynamic_Array<T> *arr, T *element)
{
    (*arr)[arr->count] = *element;
    ++arr->count;
}


#define global static
#define internal static

#define zero_memory(ptr, size) \
zero_memory_((ptr), (size)*sizeof(decltype(*(ptr))))
#define zero_struct(ptr) \
zero_memory_((ptr), sizeof(decltype(*(ptr))))
#define copy_memory(dest, src, count) \
copy_memory_((dest), (src), (count)*sizeof(decltype(*(dest))))
#define fill_memory(ptr, val, count) \
fill_memory_((ptr), (val), (count)*sizeof(decltype(*(ptr))))


internal inline void zero_memory_(void *ptr, u64 size);
internal inline void copy_memory_(void *dest, void *src, u64 size);
internal inline void fill_memory_(void *dest, u8 value, u64 size);


//
// Allocation functions (to be replaced later)
//

#define mem_alloc(type, n) \
((type*)mem_alloc_((n)*sizeof(type)))
#define mem_resize(old_ptr, old_n, new_n) \
((decltype(old_ptr))mem_resize_((old_ptr), (old_n)*sizeof(decltype(*(old_ptr))), (new_n)*sizeof(decltype(*(old_ptr)))))
#define mem_dealloc(ptr, n) \
(mem_dealloc_(ptr, (n)*sizeof(decltype(*(ptr)))))

internal inline void *mem_alloc_(u64 size);
internal inline void *mem_realloc_(void *old_ptr, u64 old_size, u64 new_size);
internal inline void mem_dealloc_(void *ptr, u64 size);


//
// Common Macros
//

#define str_lit(str) \
((String){ sizeof(str)-1, (byte*)(str) })

#define static_array_size(arr) \
(sizeof((arr)) / sizeof(decltype((arr)[0])))

#define CONCAT_IMPL(s1,s2) s1##s2
#define CONCAT(s1,s2) CONCAT_IMPL(s1, s2)

#define STRINGIFY_IMPL(s) #s
#define STRINGIFY(s) STRINGIFY_IMPL(s)

#ifdef __COUNTER__
#define ANONYMOUS_VARIABLE(str)					\
CONCAT(str, __COUNTER__)
#else
#define ANONYMOUS_VARIABLE(str)					\
CONCAT(str, __LINE__)
#endif


//
// Inline functions
//

#include <stdlib.h>
#include <string.h>

internal inline void zero_memory_(void *ptr, u64 size)
{
    memset(ptr, 0, size);
}
internal inline void copy_memory_(void *dest, void *src, u64 size)
{
    memcpy(dest, src, size);
}
internal inline void fill_memory_(void *dest, u8 value, u64 size)
{
    memset(dest, value, size);
}

internal inline void *mem_alloc_(u64 size)
{
    return malloc(size);
}
internal inline void *mem_resize_(void *old_ptr, u64 old_size, u64 new_size)
{
    return realloc(old_ptr, new_size);
}
internal inline void mem_dealloc_(void *ptr, u64 size)
{
    free(ptr);
}

#endif // BASIC_H
