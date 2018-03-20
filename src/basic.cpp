

void *libc_alloc_func(void *data, Allocator_Mode mode, void *old_ptr, u64 old_size, u64 new_size)
{
    if(mode == Allocator_Mode::alloc)
    {
        byte *memory = (byte*)malloc(new_size);
#if USE_DEBUG_MEMORY_PATTERN
        fill_memory(memory, MEMORY_PATTERN, new_size);
#endif
        return (void*)memory;
    }
    else if(mode == Allocator_Mode::resize)
    {
        byte *memory = (byte*)realloc(old_ptr, new_size);
#if USE_DEBUG_MEMORY_PATTERN
        if(new_size > old_size)
        {
            byte *start = memory + old_size;
            fill_memory(start, MEMORY_PATTERN, new_size-old_size);
        }
#endif
        return (void*)memory;
    }
    else if(mode == Allocator_Mode::dealloc)
    {
        free(old_ptr);
        return nullptr;
    }
    
    assert(false);
    return nullptr;
}
