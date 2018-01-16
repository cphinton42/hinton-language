#ifndef POOL_ALLOCATOR_H
#define POOL_ALLOCATOR_H

struct Block_Header
{
    Block_Header *next;
    u64 length;
    byte memory[];
};

struct Pool_Allocator
{
    Block_Header *current_block;
    byte *current_point;
    byte *current_end;
    
    Block_Header *used_blocks;
    Block_Header *free_blocks;
    
    u64 new_block_size;
};

#define pool_alloc(type, pool, n) \
((type*)pool_alloc_((pool),(n)*sizeof(type)))
#define pool_resize(pool, old_ptr, old_n, new_n) \
((decltype(old_ptr))pool_resize_((pool),(old_ptr), (old_n)*sizeof(decltype(*(old_ptr))), (new_n)*sizeof(decltype(*(old_ptr)))))

void init_pool(Pool_Allocator *pool, u64 block_size);

void *pool_alloc_(Pool_Allocator *pool, u64 size);
void *pool_resize_(Pool_Allocator *pool, void *old_ptr, u64 old_size, u64 new_size);

void pool_reset(Pool_Allocator *pool);
void pool_release(Pool_Allocator *pool);

#endif // POOL_ALLOCATOR_H
