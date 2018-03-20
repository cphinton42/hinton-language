#ifndef POOL_ALLOCATOR_H
#define POOL_ALLOCATOR_H

struct Block_Header
{
    Block_Header *next;
    u64 size;
    byte memory[];
};

struct Pool_Allocator
{
    Block_Header *current_block;
    byte *current_point;
    byte *current_end;
    u64 mark;
    
    Block_Header *used_blocks;
    Block_Header *free_blocks;
    
    u64 new_block_size;
};

#define pool_alloc(...) \
GET_MACRO(__VA_ARGS__,dummy,pool_alloc3,pool_alloc2)(__VA_ARGS__)

#define pool_alloc2(type,pool) \
pool_alloc3(type,1,pool)
#define pool_alloc3(type,n,pool) \
((type*)pool_alloc_((pool),(n)*sizeof(type)))
#define pool_resize(old_ptr,old_n,new_n,pool) \
((decltype(old_ptr))pool_resize_((pool),(old_ptr), (old_n)*sizeof(decltype(*(old_ptr))), (new_n)*sizeof(decltype(*(old_ptr)))))

void pool_init(Pool_Allocator *pool, u64 block_size);

void *pool_alloc_func(void *data, Allocator_Mode mode, void *old_ptr, u64 old_size, u64 new_size);

void *pool_alloc_(Pool_Allocator *pool, u64 size);
void *pool_resize_(Pool_Allocator *pool, void *old_ptr, u64 old_size, u64 new_size);

void pool_reset(Pool_Allocator *pool);
void pool_release(Pool_Allocator *pool);

#endif // POOL_ALLOCATOR_H
