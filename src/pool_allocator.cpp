
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
    return allocate_block(block_size);
}

internal inline Block_Header* pool_get_any_block(Pool_Allocator *pool)
{
    if(pool->free_blocks)
    {
        Block_Header *result = pool->free_blocks;
        pool->free_blocks = result->next;
        result->next = nullptr;
        return result;
    }
    else
    {
        return allocate_block(pool->new_block_size);
    }
}

// TODO: let the pool take from free blocks
void *pool_alloc_(Pool_Allocator *pool, u64 size)
{
    if(!pool->current_block)
    {
        assert(pool->new_block_size > 0);
        
        Block_Header *block = pool_get_any_block(pool);
        pool->current_block = block;
        pool->current_point = block->memory;
        pool->current_end = block->memory + block->length - sizeof(Block_Header);
    }
    
    assert(pool->current_block && pool->current_point && pool->current_end);
    
    u64 available_space = pool->current_end - pool->current_point;
    
    if(size <= available_space)
    {
        // There is enough space for the allocation
        
        void *result = pool->current_point;
        uintptr_t unaligned_point = (uintptr_t)(pool->current_point + size);
        pool->current_point = (byte*)((unaligned_point + 7) & (~7));
        return result;
    }
    else
    {
        u64 block_size = size + sizeof(Block_Header);
        if(pool->new_block_size > block_size)
        {
            block_size = pool->new_block_size;
        }
        
        Block_Header *block = pool_get_block(pool, block_size);
        void *result = block->memory;
        
        u64 potential_waste = block_size - size - sizeof(Block_Header);
        if(potential_waste <= available_space)
        {
            // Less fragmentation if we put the new block on the used_blocks list
            
            block->next = pool->used_blocks;
            pool->used_blocks = block;
        }
        else
        {
            // Less fragmentation if we replace the current block with the newly allocated one
            
            pool->current_block->next = pool->used_blocks;
            pool->used_blocks = pool->current_block;
            
            pool->current_block = block;
            uintptr_t unaligned_point = (uintptr_t)(block->memory + size);
            pool->current_point = (byte*)((unaligned_point + 7) & (~7));
            pool->current_end = block->memory + block->length - sizeof(Block_Header);
        }
        
        return result;
    }
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
}
