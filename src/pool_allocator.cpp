
#include <sys/mman.h>

internal inline Block_Header* allocate_block(u64 block_size)
{
    void *memory = mmap(nullptr, block_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if(memory == MAP_FAILED)
    {
        return nullptr;
    }
    else
    {
        Block_Header *result = static_cast<Block_Header*>(memory);
        result->next = nullptr;
        result->length = block_size;
        return result;
    }
}

internal inline void deallocate_blocks(Block_Header *block)
{
    while(block)
    {
        Block_Header *next_block = block->next;
        munmap(block, block->length);
        block = next_block;
    }
}

void init_pool(Pool_Allocator *pool, u64 block_size)
{
    zero_struct(pool);
    pool->new_block_size = block_size;
}

internal inline Block_Header* pool_get_block(Pool_Allocator *pool, u64 block_size)
{
    if(pool->free_blocks)
    {
        Block_Header *prev_block = nullptr;
        Block_Header *block = pool->free_blocks;
        while(block && block->length < block_size)
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
    if(block_size < pool->new_block_size)
    {
        block_size = pool->new_block_size;
    }
    return allocate_block(block_size);
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
        pool->current_end = block->memory - sizeof(Block_Header) + block->length;
    }
    
    assert(pool->current_block && pool->current_point && pool->current_end);
    
    u64 available_space = pool->current_end - pool->current_point;
    
    if(size > available_space)
    {
        u64 block_size = size + sizeof(Block_Header);
        
        Block_Header *block = pool_get_block(pool, block_size);
        block_size = block->length;
        
        // Retire the old block
        pool->current_block->next = pool->used_blocks;
        pool->used_blocks = pool->current_block;
        pool->mark += available_space;
        
        pool->current_block = block;
        pool->current_point = block->memory;
        pool->current_end = block->memory - sizeof(Block_Header) + block->length;
        
        available_space = pool->current_end - pool->current_point;
    }
    
    assert(size <= available_space);
    
    byte *result = pool->current_point;
    uintptr_t unaligned_point = (uintptr_t)(pool->current_point + size);
    pool->current_point = (byte*)((unaligned_point + 7) & (~7));
    
    pool->mark += (pool->current_point - result);
    
    return (void*)result;
}

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
            pool->current_end = block->memory - sizeof(Block_Header) + block->length;
        }
    }
    while(mark < pool->mark);
    
    assert(mark == pool->mark);
}

void pool_reset(Pool_Allocator *pool)
{
    if(pool->current_block)
    {
        pool->current_point = pool->current_block->memory;
    }
    if(pool->used_blocks)
    {
        Block_Header *block = pool->used_blocks;
        
        while(block->next)
        {
            block = block->next;
        }
        
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
