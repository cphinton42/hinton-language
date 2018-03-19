#ifndef SCOPE_H
#define SCOPE_H

#include "basic.h"
#include "pool_allocator.h"

// TODO: support custom allocator?
// Note: K==Key, GK==get_key, H==hash, Eq==equality
template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
struct Hash_Set
{
    u64 set_size;
    u64 count;
    u64 *hashes;
    T *entries;
};

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
void init_hash_set(Hash_Set<T,K,GK,H,Eq> *hs, u64 initial_size = 0);

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
bool set_insert(Hash_Set<T,K,GK,H,Eq> *hs, T entry);

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
bool set_insert(Hash_Set<T,K,GK,H,Eq> *hs, T entry, u64 hash);

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
void set_resize(Hash_Set<T,K,GK,H,Eq> *hs, u64 new_size);

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
T *set_find(Hash_Set<T,K,GK,H,Eq> *hs, K key);

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
T *set_find(Hash_Set<T,K,GK,H,Eq> *hs, K key, u64 hash);

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
u64 set_find_slot(Hash_Set<T,K,GK,H,Eq> *hs, K key);

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
u64 set_find_slot(Hash_Set<T,K,GK,H,Eq> *hs, K key, u64 hash);

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
void set_insert_into_slot(Hash_Set<T,K,GK,H,Eq> *hs, u64 slot, u64 hash, T entry);



struct Atom
{
    String *str;
};

inline
bool operator==(Atom a1, Atom a2) { return a1.str == a2.str; }
inline
String get_str(Atom &a) { return *a.str; }
u64 fnv1a_64(String s);
u64 compute_hash64(Atom a);

struct Atom_Table
{
    Hash_Set<Atom,String,get_str,fnv1a_64,operator==> atom_set;
    Pool_Allocator atom_pool;
};

void init_atom_table(Atom_Table *at, u64 initial_set_size, u64 block_size);
Atom atomize_string(Atom_Table *at, String str);



struct Expr_AST;
struct Scope_Entry
{
    Atom key;
    u64 index;
    Expr_AST *definition;
};

inline
Atom get_key(Scope_Entry &entry) { return entry.key; }

struct Hashed_Scope
{
    Hashed_Scope *parent_scope;
    u64 index_in_parent;
    Hash_Set<Scope_Entry,Atom,get_key,compute_hash64,operator==> entry_set;
};

void init_hashed_scope(Hashed_Scope *hs, Hashed_Scope *parent_scope, u64 index_in_parent, u64 initial_size = 8);
bool scope_insert(Hashed_Scope *hs, Scope_Entry entry);
bool scope_insert(Hashed_Scope *hs, Scope_Entry entry, u64 hash);
void scope_resize(Hashed_Scope *hs, u64 new_size);
Expr_AST *scope_find(Hashed_Scope *hs, Atom key, u64 scope_index, bool recurse = true);
Expr_AST *scope_find(Hashed_Scope *hs, Atom key, u64 scope_index, u64 hash, bool recurse = true);



template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
void init_hash_set(Hash_Set<T,K,GK,H,Eq> *hs, u64 initial_size)
{
    hs->set_size = initial_size;
    hs->count = 0;
    
    if(initial_size > 0)
    {
        // TODO: could replace with SOA allocation
        hs->hashes = mem_alloc(u64, initial_size);
        hs->entries = mem_alloc(T, initial_size);
        
        assert(hs->hashes);
        assert(hs->entries);
        
        zero_memory(hs->hashes, initial_size);
    }
    else
    {
        hs->hashes = nullptr;
        hs->entries = nullptr;
    }
}

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
bool set_insert(Hash_Set<T,K,GK,H,Eq> *hs, T entry)
{
    return set_insert(hs, entry, H(GK(entry)));
}

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
bool set_insert(Hash_Set<T,K,GK,H,Eq> *hs, T entry, u64 hash)
{
    u64 slot = set_find_slot(hs, GK(entry), hash);
    if(hs->hashes[slot] == 0)
    {
        set_insert_into_slot(hs, slot, hash, entry);
        return true;
    }
    else
    {
        return false;
    }
}

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
void set_resize(Hash_Set<T,K,GK,H,Eq> *hs, u64 new_size)
{
    assert(new_size >= hs->count);
    
    u64 old_size = hs->set_size;
    u64 *old_hashes = hs->hashes;
    T *old_entries = hs->entries;
    
    init_hash_set(hs, new_size);
    
    for(u64 i = 0; i < old_size; ++i)
    {
        if(old_hashes[i] != 0)
        {
            set_insert(hs, old_entries[i], old_hashes[i]);
        }
    }
    
    mem_dealloc(old_hashes, old_size);
    mem_dealloc(old_entries, old_size);
}

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
T *set_find(Hash_Set<T,K,GK,H,Eq> *hs, K key)
{
    return set_find(hs, key, H(key));
}

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
T *set_find(Hash_Set<T,K,GK,H,Eq> *hs, K key, u64 hash)
{
    u64 slot = set_find_slot(hs, key, hash);
    if(hs->hashes[slot] == 0)
    {
        return nullptr;
    }
    else
    {
        return &hs->entries[slot];
    }
}

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
u64 set_find_slot(Hash_Set<T,K,GK,H,Eq> *hs, K key)
{
    return set_find_slot(hs, key, H(key));
}

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
u64 set_find_slot(Hash_Set<T,K,GK,H,Eq> *hs, K key, u64 hash)
{
    if(hash == 0)
    {
        hash = 1;
    }
    
    u64 mask = hs->set_size - 1;
    u64 idx = hash & mask;
    u64 inc = 0;
    
    while(true)
    {
        if(hs->hashes[idx] == 0)
        {
            return idx;
        }
        else if(hs->hashes[idx] == hash && Eq(GK(hs->entries[idx]), key))
        {
            return idx;
        }
        else
        {
            ++inc;
            idx = (idx + inc) & mask;
        }
    }
}

template<typename T, typename K, K (*GK)(T&), u64 (*H)(K), bool (*Eq)(K,K)>
void set_insert_into_slot(Hash_Set<T,K,GK,H,Eq> *hs, u64 slot, u64 hash, T entry)
{
    if(hash == 0)
    {
        hash = 1;
    }
    hs->hashes[slot] = hash;
    hs->entries[slot] = entry;
    ++hs->count;
    if(4*hs->count > 3*hs->set_size)
    {
        set_resize(hs, 4*hs->set_size);
    }
}

struct Scoping_Context
{
    Atom_Table *atom_table;
    Pool_Allocator *ast_pool;
    bool success;
};

struct Decl_AST;
bool create_scope_metadata(Scoping_Context *ctx, Array<Decl_AST*> decls);


#endif // SCOPE_H