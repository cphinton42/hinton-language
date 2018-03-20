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

inline void zero_memory_(void *ptr, u64 size);
inline void copy_memory_(void *dest, void *src, u64 size);
inline void fill_memory_(void *dest, u8 value, u64 size);

// Note: reset/release all is useful, but not really usable in generic context(?)
enum class Allocator_Mode
{
    alloc,
    resize,
    dealloc
};

typedef void*(*Allocator_Function)(void *data, Allocator_Mode mode, void *old_ptr, u64 old_size, u64 new_size);

// I would make it a base class with virtual member functions, but I love POD
struct Allocator {
    Allocator_Function function;
    void *data;
};

#ifndef MEMORY_PATTERN
#define MEMORY_PATTERN 0xCC
#endif

void *libc_alloc_func(void *data, Allocator_Mode mode, void *old_ptr, u64 old_size, u64 new_size);

Allocator libc_allocator = {libc_alloc_func, nullptr};
Allocator default_allocator = libc_allocator;

// Allocation functions (to be replaced ?)
#define GET_MACRO(_1,_2,_3,_4,NAME,...) NAME
#define mem_alloc(...) \
GET_MACRO(__VA_ARGS__,dummy,mem_alloc3,mem_alloc2,mem_alloc1)(__VA_ARGS__)
#define mem_resize(...) \
GET_MACRO(__VA_ARGS__,mem_resize4,mem_resize3)(__VA_ARGS__)
#define mem_dealloc(...) \
GET_MACRO(__VA_ARGS__,dummy,mem_dealloc3,mem_dealloc2,mem_dealloc1)(__VA_ARGS__)

#define mem_alloc1(type) \
mem_alloc3(type,1,default_allocator)
#define mem_alloc2(type, n) \
mem_alloc3(type,n,default_allocator)
#define mem_alloc3(type, n, a) \
((type*)mem_alloc_((n)*sizeof(type),(a)))

#define mem_resize3(old_ptr, old_n, new_n) \
mem_resize4(old_ptr,old_n,new_n,default_allocator)
#define mem_resize4(old_ptr, old_n, new_n, a) \
((decltype(old_ptr))mem_resize_((old_ptr), (old_n)*sizeof(decltype(*(old_ptr))), (new_n)*sizeof(decltype(*(old_ptr))),(a)))

#define mem_dealloc1(ptr) \
mem_dealloc3(ptr,1,default_allocator)
#define mem_dealloc2(ptr, n) \
mem_dealloc3(ptr,n,default_allocator)
#define mem_dealloc3(ptr, n, a) \
(mem_dealloc_(ptr, (n)*sizeof(decltype(*(ptr))),(a)))

inline
void *mem_alloc_(u64 size, Allocator a);
inline
void *mem_resize_(void *old_ptr, u64 old_n, u64 new_n, Allocator a);
inline
void mem_dealloc_(void *old_ptr, u64 old_n, Allocator a);

template<typename T>
T max(T t1, T t2);

#define defer \
auto ANONYMOUS_VARIABLE(DEFER_TO_EXIT) = Defer_To_Exit() + [&]()


template <typename T>
struct Dynamic_Array
{
    union {
        Array<T> array;
        struct {
            u64 count;
            T *data;
        };
    };
    u64 allocated;
    
    T &operator[](u64 idx);
};

template<typename T>
bool operator==(Array<T> a1, Array<T> a2);

template<typename T>
Array<T> make_array(u64 count, T *data);

template<typename T>
void array_add(Dynamic_Array<T> *arr, T element, Allocator a = default_allocator);
template<typename T>
void array_resize(Dynamic_Array<T> *arr, u64 new_size, Allocator a = default_allocator);
template<typename T>
void array_trim(Dynamic_Array<T> *arr, Allocator a = default_allocator);
template<typename T>
Array<T> array_copy(Array<T> arr, Allocator a = default_allocator);

//
// Common Macros
//

#define str_lit(str) \
((String){ sizeof(str)-1, (byte*)(str) })

#define static_array_size(arr) \
(sizeof((arr)) / sizeof(decltype((arr)[0])))

// Note: Declares an Array<T> with a static T[N].
// N must be supplied manually, there will be an error if it is too small
#define declare_static_array(type, name,num) \
declare_static_array_(type,name,num,CONCAT(name,_backing_array))
#define declare_static_array_(type,name,num,arr) \
extern type arr[]; \
Array<type> name = make_array(num, arr); \
type arr[num]


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
// Inline and template function implementations
//

#include <stdlib.h>
#include <string.h>

inline
void zero_memory_(void *ptr, u64 size)
{
    memset(ptr, 0, size);
}
inline
void copy_memory_(void *dest, void *src, u64 size)
{
    memcpy(dest, src, size);
}
inline
void fill_memory_(void *dest, u8 value, u64 size)
{
    memset(dest, value, size);
}

inline
void *mem_alloc_(u64 size, Allocator a)
{
    return a.function(a.data, Allocator_Mode::alloc, nullptr, 0, size);
}
inline
void *mem_resize_(void *old_ptr, u64 old_n, u64 new_n, Allocator a)
{
    return a.function(a.data, Allocator_Mode::resize, old_ptr, old_n, new_n);
}
inline
void mem_dealloc_(void *old_ptr, u64 old_n, Allocator a)
{
    a.function(a.data, Allocator_Mode::dealloc, old_ptr, old_n, 0);
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
Array<T> make_array(u64 count, T *data)
{
    return {count, data};
}

template<typename T>
void array_add(Dynamic_Array<T> *arr, T element, Allocator a)
{
    if(arr->count == arr->allocated)
    {
        u64 new_size = 4 * arr->allocated;
        if(new_size == 0)
        {
            new_size = 4;
        }
        array_resize(arr, new_size, a);
    }
    
    (*arr)[arr->count] = element;
    ++arr->count;
}

template<typename T>
void array_resize(Dynamic_Array<T> *arr, u64 new_size, Allocator a)
{
    // TODO: take more care with OOM errors?
    if(arr->data)
    {
        if(new_size == 0)
        {
            mem_dealloc(arr->data, arr->allocated, a);
            arr->count = 0;
            arr->data = nullptr;
            arr->allocated = 0;
        }
        else
        {
            arr->data = mem_resize(arr->data, arr->allocated, new_size, a);
            assert(arr->data);
            arr->allocated = new_size;
            if(arr->count > new_size)
            {
                arr->count = new_size;
            }
        }
    }
    else
    {
        assert(arr->count == 0);
        
        arr->data = mem_alloc(T, new_size, a);
        assert(arr->data);
        arr->allocated = new_size;
    }
}

template<typename T>
void array_trim(Dynamic_Array<T> *arr, Allocator a)
{
    array_resize(arr, arr->count, a);
}

template<typename T>
Array<T> array_copy(Array<T> arr, Allocator a)
{
    Array<T> result;
    result.count = arr.count;
    result.data = mem_alloc(T, arr.count, a);
    assert(result.data);
    copy_memory(result.data, arr.data, arr.count);
    return result;
}

template<typename T>
T max(T t1, T t2)
{
    return t1 > t2 ? t1 : t2;
}

// Defer implementation
template<typename Fn>
struct Deferred_Lambda
{
    Deferred_Lambda(Fn &&fn) : fn(static_cast<Fn&&>(fn)) {};
    ~Deferred_Lambda() { fn(); }
    
    Fn fn;
};

enum class Defer_To_Exit {};

template<typename Fn>
Deferred_Lambda<Fn> operator+(Defer_To_Exit, Fn&& fn)
{
    return Deferred_Lambda<Fn>(static_cast<Fn&&>(fn));
}

#endif // BASIC_H
