
u64 fnv1a_64(String s)
{
    const u64 offset_basis = 14695981039346656037UL;
    const u64 FNV_prime = 1099511628211UL;
    
    u64 hash = offset_basis;
    
    for(u64 i = 0; i < s.count; ++i)
    {
        hash ^= s.data[i];
        hash *= FNV_prime;
    }
    return hash;
}

u64 compute_hash64(Atom a) {
    u64 x = (u64)a.str;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9UL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebUL;
    x = x ^ (x >> 31);
    return x;
}

void init_hashed_scope(Hashed_Scope *hs, Hashed_Scope *parent_scope, u64 index_in_parent, u64 initial_size)
{
    hs->parent_scope = parent_scope;
    hs->index_in_parent = index_in_parent;
    init_hash_set(&hs->entry_set, initial_size);
}

bool scope_insert(Hashed_Scope *hs, Scope_Entry entry)
{
    return set_insert(&hs->entry_set, entry);
}
bool scope_insert(Hashed_Scope *hs, Scope_Entry entry, u64 hash)
{
    return set_insert(&hs->entry_set, entry, hash);
}
void scope_resize(Hashed_Scope *hs, u64 new_size)
{
    set_resize(&hs->entry_set, new_size);
}
Expr_AST *scope_find(Hashed_Scope *hs, Atom key, u64 scope_index, bool recurse)
{
    return scope_find(hs, key, scope_index, compute_hash64(key), recurse);
}

Expr_AST *scope_find(Hashed_Scope *hs, Atom key, u64 scope_index, u64 hash, bool recurse)
{
    do
    {
        Scope_Entry *entry = set_find(&hs->entry_set, key, hash);
        if(entry && scope_index >= entry->index)
        {
            return entry->definition;
        }
        else
        {
            scope_index = hs->index_in_parent;
            hs = hs->parent_scope;
        }
    }
    while(hs && recurse);
    return nullptr;
}

void init_atom_table(Atom_Table *at, u64 initial_set_size, u64 block_size)
{
    init_hash_set(&at->atom_set, initial_set_size);
    pool_init(&at->atom_pool, block_size);
}

Atom atomize_string(Atom_Table *at, String str)
{
    u64 hash = fnv1a_64(str);
    
    u64 slot = set_find_slot(&at->atom_set, str, hash);
    if(at->atom_set.hashes[slot] == 0)
    {
        String *new_str = pool_alloc(String, &at->atom_pool);
        assert(new_str);
        new_str->count = str.count;
        new_str->data = pool_alloc(byte, str.count, &at->atom_pool);
        copy_memory(new_str->data, str.data, str.count);
        
        Atom result = {new_str};
        set_insert_into_slot(&at->atom_set, slot, hash, result);
        
        return result;
    }
    else
    {
        return at->atom_set.entries[slot];
    }
}



internal
void create_scope_metadata(Scoping_Context *ctx, Function_AST *func, u64 type, Hashed_Scope *scope, u64 scope_index, AST *ast)
{
    if(!ast)
    {
        return;
    }
    
    switch(ast->type)
    {
        case AST_Type::decl_ast: {
            Decl_AST *decl_ast = static_cast<Decl_AST*>(ast);
            create_scope_metadata(ctx, func, IDENT_DECL, scope, scope_index, &decl_ast->ident);
            create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, decl_ast->decl_type);
            create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, decl_ast->expr);
        } break;
        case AST_Type::block_ast: {
            Block_AST *block_ast = static_cast<Block_AST*>(ast);
            
            Hashed_Scope *block_scope = pool_alloc(Hashed_Scope, ctx->ast_pool);
            init_hashed_scope(block_scope, scope, scope_index, 8);
            
            for(u64 i = 0; i < block_ast->statements.count; ++i)
            {
                create_scope_metadata(ctx, func, IDENT_REFERENCE, block_scope, i, block_ast->statements[i]);
            }
        } break;
        case AST_Type::while_ast: {
            While_AST *while_ast = static_cast<While_AST*>(ast);
            create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, while_ast->guard);
            create_scope_metadata(ctx, func, IDENT_DECL, scope, scope_index, while_ast->body);
        } break;
        case AST_Type::for_ast: {
            For_AST *for_ast = static_cast<For_AST*>(ast);
            
            Hashed_Scope *for_scope = pool_alloc(Hashed_Scope, ctx->ast_pool);
            init_hashed_scope(for_scope, scope, scope_index, 4);
            
            create_scope_metadata(ctx, func, IDENT_LOOP_VAR, for_scope, 0, for_ast->induction_var);
            
            if(for_ast->flags & FOR_FLAG_OVER_ARRAY)
            {
                create_scope_metadata(ctx, func, IDENT_LOOP_VAR, for_scope, 0, for_ast->index_var);
                create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, for_ast->array_expr);
            }
            else
            {
                create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, for_ast->low_expr);
                create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, for_ast->high_expr);
            }
            create_scope_metadata(ctx, func, IDENT_REFERENCE, for_scope, 1, for_ast->body);
        } break;
        case AST_Type::if_ast: {
            If_AST *if_ast = static_cast<If_AST*>(ast);
            create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, if_ast->guard);
            create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, if_ast->then_block);
            create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, if_ast->else_block);
        } break;
        case AST_Type::assign_ast: {
            Assign_AST *assign_ast = static_cast<Assign_AST*>(ast);
            create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, assign_ast->lhs);
            create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, assign_ast->rhs);
        } break;
        case AST_Type::return_ast: {
            Return_AST *return_ast = static_cast<Return_AST*>(ast);
            return_ast->function = func;
            create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, return_ast->expr);
        } break;
        case AST_Type::ident_ast: {
            Ident_AST *ident_ast = static_cast<Ident_AST*>(ast);
            
            String str = ident_ast->ident;
            Atom atom = atomize_string(ctx->atom_table, str);
            
            ident_ast->atom = atom;
            ident_ast->tagged_expr_ptr = type; // need to know this
            ident_ast->scope_index = scope_index;
            ident_ast->scope = scope;
            
            if(type != IDENT_REFERENCE)
            {
                Scope_Entry entry;
                entry.key = atom;
                entry.index = scope_index;
                entry.definition = ident_ast;
                bool success = scope_insert(scope, entry);
                if(!success)
                {
                    // TODO: better error reporting
                    Expr_AST *prev = scope_find(scope, atom, scope_index);
                    assert(prev);
                    print_err("Error: %d:%d: Redeclared identifier '%.*s'\nPrevious declaration at %d:%d\n", ident_ast->line_number, ident_ast->line_offset, str.count, str.data, prev->line_number, prev->line_offset);
                    ctx->success = false;
                }
            }
        } break;
        case AST_Type::function_type_ast: {
            Function_Type_AST *function_type_ast = static_cast<Function_Type_AST*>(ast);
            for(u64 i = 0; i < function_type_ast->parameter_types.count; ++i)
            {
                create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, function_type_ast->parameter_types[i]);
            }
            for(u64 i = 0; i < function_type_ast->return_types.count; ++i)
            {
                create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, function_type_ast->return_types[i]);
            }
        } break;
        case AST_Type::function_ast: {
            Function_AST *function_ast = static_cast<Function_AST*>(ast);
            
            Hashed_Scope *func_scope = pool_alloc(Hashed_Scope, ctx->ast_pool);
            init_hashed_scope(func_scope, scope, scope_index, 8);
            
            create_scope_metadata(ctx, function_ast, IDENT_REFERENCE, func_scope, 0, function_ast->prototype);
            
            for(u64 i = 0; i < function_ast->param_names.count; ++i)
            {
                create_scope_metadata(ctx, function_ast, IDENT_PARAM, func_scope, 0, function_ast->param_names[i]);
            }
            for(u64 i = 0; i < function_ast->default_values.count; ++i)
            {
                create_scope_metadata(ctx, function_ast, IDENT_REFERENCE, func_scope, 1, function_ast->default_values[i]);
            }
            create_scope_metadata(ctx, function_ast, IDENT_REFERENCE, func_scope, 2, function_ast->block);
        } break;
        case AST_Type::function_call_ast: {
            Function_Call_AST *function_call_ast = static_cast<Function_Call_AST*>(ast);
            
            create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, function_call_ast->function);
            
            for(u64 i = 0; i < function_call_ast->args.count; ++i)
            {
                create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, function_call_ast->args[i]);
            }
        } break;
        case AST_Type::access_ast: {
            Access_AST *access_ast = static_cast<Access_AST*>(ast);
            
            String str = access_ast->ident;
            Atom atom = atomize_string(ctx->atom_table, str);
            access_ast->atom = atom;
            access_ast->expr = nullptr;
            create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, access_ast->lhs);
        } break;
        case AST_Type::binary_operator_ast: {
            Binary_Operator_AST *binop_ast = static_cast<Binary_Operator_AST*>(ast);
            create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, binop_ast->lhs);
            create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, binop_ast->rhs);
        } break;
        case AST_Type::enum_ast: {
            Enum_AST *enum_ast = static_cast<Enum_AST*>(ast);
            
            Hashed_Scope *values_scope = pool_alloc(Hashed_Scope, ctx->ast_pool);
            init_hashed_scope(values_scope, scope, scope_index, 8);
            
            for(u64 i = 0; i < enum_ast->values.count; ++i)
            {
                // TODO: is IDENT_DECL best for this?
                create_scope_metadata(ctx, func, IDENT_REFERENCE, values_scope, 0, enum_ast->values[i]);
            }
            enum_ast->scope = values_scope;
        } break;
        case AST_Type::struct_ast: {
            Struct_AST *struct_ast = static_cast<Struct_AST*>(ast);
            
            Hashed_Scope *constants_scope = pool_alloc(Hashed_Scope, ctx->ast_pool);
            init_hashed_scope(constants_scope, scope, scope_index, 8);
            Hashed_Scope *fields_scope = pool_alloc(Hashed_Scope, ctx->ast_pool);
            init_hashed_scope(fields_scope, constants_scope, 0, 8);
            
            for(u64 i = 0; i < struct_ast->constants.count; ++i)
            {
                create_scope_metadata(ctx, func, IDENT_DECL, constants_scope, 0, struct_ast->constants[i]);
            }
            for(u64 i = 0; i < struct_ast->fields.count; ++i)
            {
                create_scope_metadata(ctx, func, IDENT_FIELD, fields_scope, 0, struct_ast->fields[i]);
            }
            struct_ast->constant_scope = constants_scope;
            struct_ast->field_scope = fields_scope;
        } break;
        case AST_Type::unary_ast: {
            Unary_Operator_AST *unop_ast = static_cast<Unary_Operator_AST*>(ast);
            create_scope_metadata(ctx, func, IDENT_REFERENCE, scope, scope_index, unop_ast->operand);
        } break;
        case AST_Type::number_ast:
        case AST_Type::primitive_ast:
        case AST_Type::string_ast:
        case AST_Type::bool_ast: {
            // Do nothing
        } break;
    }
}

bool create_scope_metadata(Scoping_Context *ctx, Array<Decl_AST*> decls)
{
    ctx->success = true;
    
    Hashed_Scope *file_scope = pool_alloc(Hashed_Scope, ctx->ast_pool);
    init_hashed_scope(file_scope, nullptr, 0, 16);
    
    for(u64 i = 0; i < decls.count; ++i)
    {
        create_scope_metadata(ctx, nullptr, IDENT_DECL, file_scope, 0, decls[i]);
    }
    return ctx->success;
}
