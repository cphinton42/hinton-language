
#include <sys/mman.h>

void pool_init(Pool_Allocator *pool, u64 block_size)
{
    zero_struct(pool);
    pool->new_block_size = block_size;
}

void *pool_alloc_func(void *data, Allocator_Mode mode, void *old_ptr, u64 old_size, u64 new_size)
{
    Pool_Allocator *pool = static_cast<Pool_Allocator*>(data);
    if(mode == Allocator_Mode::alloc)
    {
        return pool_alloc_(pool, new_size);
    }
    else if(mode == Allocator_Mode::resize)
    {
        // return pool_resize_(pool, old_ptr, old_size, new_size);
        return nullptr;
    }
    else if(mode == Allocator_Mode::dealloc)
    {
        // TODO: perhaps deallocate if the allocation was the most recent?
        assert(false);
        return nullptr;
    }
    
    assert(false);
    return nullptr;
}

internal
Block_Header* allocate_block(u64 block_size)
{
    // TODO: try out allocating large pages?
    print_err("INFO: allocating block\n");
    void *memory = mmap(nullptr, block_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if(memory == MAP_FAILED)
    {
        return nullptr;
    }
    else
    {
        Block_Header *result = static_cast<Block_Header*>(memory);
        result->next = nullptr;
        result->size = block_size;
#if USE_DEBUG_MEMORY_PATTERN
        byte *start = result->memory;
        u64 size = block_size - sizeof(Block_Header);
        fill_memory(start, MEMORY_PATTERN, size);
#endif
        return result;
    }
}

internal
void deallocate_blocks(Block_Header *block)
{
    while(block)
    {
        Block_Header *next_block = block->next;
        munmap(block, block->size);
        block = next_block;
    }
}

internal
Block_Header* pool_get_block(Pool_Allocator *pool, u64 min_size)
{
    if(pool->free_blocks)
    {
        Block_Header *prev_block = nullptr;
        Block_Header *block = pool->free_blocks;
        while(block && block->size < min_size)
        {
            prev_block = block;
            block = block->next;
        }
        if(block)
        {
            if(prev_block)
            {
                prev_block->next = block->next;
            }
            else
            {
                pool->free_blocks = block->next;
            }
            
            block->next = nullptr;
            return block;
        }
    }
    u64 alloc_size = min_size;
    if(alloc_size < pool->new_block_size)
    {
        alloc_size = pool->new_block_size;
    }
    return allocate_block(alloc_size);
}

// TODO: let the pool take from free blocks
void *pool_alloc_(Pool_Allocator *pool, u64 size)
{
    if(!pool->current_block)
    {
        assert(pool->new_block_size > 0);
        
        Block_Header *block = pool_get_block(pool, 0);
        pool->current_block = block;
        pool->current_point = block->memory;
        pool->current_end = block->memory - sizeof(Block_Header) + block->size;
    }
    
    assert(pool->current_block && pool->current_point && pool->current_end);
    
    u64 available_space = pool->current_end - pool->current_point;
    
    if(size > available_space)
    {
        // Retire the old block
        pool->current_block->next = pool->used_blocks;
        pool->used_blocks = pool->current_block;
        pool->mark += available_space;
        
        // Get a new block
        u64 min_size = size + sizeof(Block_Header);
        
        Block_Header *block = pool_get_block(pool, min_size);
        pool->current_block = block;
        pool->current_point = block->memory;
        pool->current_end = block->memory - sizeof(Block_Header) + block->size;
        
        available_space = pool->current_end - pool->current_point;
    }
    
    assert(size <= available_space);
    
    byte *result = pool->current_point;
    uintptr_t unaligned_point = (uintptr_t)(pool->current_point + size);
    pool->current_point = (byte*)((unaligned_point + 7) & (~7));
    
    pool->mark += (pool->current_point - result);
    
    return (void*)result;
}

void *pool_resize_(Pool_Allocator *pool, void *old_ptr, u64 old_size, u64 new_size)
{
    byte *old_memory = (byte*)old_ptr;
    uintptr_t unaligned_point = (uintptr_t)(old_memory + old_size);
    byte *old_end = (byte*)((unaligned_point + 7) & (~7));
    if(old_end == pool->current_point)
    {
        u64 real_old_size = old_end - old_memory;
        pool->current_point = old_memory;
        pool->mark -= real_old_size;
        
        byte *new_memory = (byte*)pool_alloc_(pool, new_size);
        if(new_memory != old_memory)
        {
            copy_memory(new_memory, old_memory, old_size);
#if USE_DEBUG_MEMORY_PATTERN
            fill_memory(old_memory, MEMORY_PATTERN, old_size);
#endif
        }
        return (void*)new_memory;
    }
    else if(new_size == 0)
    {
        return nullptr;
    }
    else
    {
        byte *new_memory = (byte*)pool_alloc_(pool, new_size);
        copy_memory(new_memory, old_memory, old_size);
        return (void*)new_memory;
    }
}

#if 0
// Note: if we don't need this, we could probably save in fragmentation (not sure how much) by retiring the block with smallest waste instead of always retiring in order
void pool_rewind(Pool_Allocator *pool, u64 mark)
{
    assert(mark < pool->mark);
    
    do
    {
        u64 to_rewind = pool->mark - mark;
        u64 in_current = pool->current_point - pool->current_block->memory;
        
        if(in_current <= to_rewind)
        {
            pool->current_point -= to_rewind;
            pool->mark -= to_rewind;
        }
        else
        {
            // Free current block
            pool->current_block->next = pool->free_blocks;
            pool->free_blocks = pool->current_block;
            pool->mark -= in_current;
            
            // Get next block from the used list
            assert(pool->used_blocks);
            
            Block_Header *block = pool->used_blocks;
            pool->used_blocks = pool->used_blocks->next;
            block->next = nullptr;
            
            pool->current_block = block;
            pool->current_point = block->memory;
            pool->current_end = block->memory - sizeof(Block_Header) + block->size;
        }
    }
    while(mark < pool->mark);
    
    assert(mark == pool->mark);
}
#endif

void pool_reset(Pool_Allocator *pool)
{
    if(pool->current_block)
    {
        pool->current_point = pool->current_block->memory;
#if USE_DEBUG_MEMORY_PATTERN
        u64 size = pool->current_end - pool->current_point;
        fill_memory(pool->current_point, MEMORY_PATTERN, size);
#endif
    }
    if(pool->used_blocks)
    {
        Block_Header *block = pool->used_blocks;
        
        while(block->next)
        {
#if USE_DEBUG_MEMORY_PATTERN
            u64 size = block->size - sizeof(Block_Header);
            fill_memory(block->memory, MEMORY_PATTERN, size);
#endif
            block = block->next;
        }
#if USE_DEBUG_MEMORY_PATTERN
        u64 size = block->size - sizeof(Block_Header);
        fill_memory(block->memory, MEMORY_PATTERN, size);
#endif
        block->next = pool->free_blocks;
        pool->free_blocks = pool->used_blocks;
        pool->used_blocks = nullptr;
    }
    pool->mark = 0;
}

void pool_release(Pool_Allocator *pool)
{
    if(pool->current_block)
    {
        deallocate_blocks(pool->current_block);
        pool->current_block = nullptr;
    }
    if(pool->used_blocks)
    {
        deallocate_blocks(pool->used_blocks);
        pool->used_blocks = nullptr;
    }
    if(pool->free_blocks)
    {
        deallocate_blocks(pool->free_blocks);
        pool->free_blocks = nullptr;
    }
    
    pool->current_point = nullptr;
    pool->current_end = nullptr;
    pool->mark = 0;
}
