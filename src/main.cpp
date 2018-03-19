
inline
u64 read_tsc()
{
    register u64 rax asm("rax");
    register u64 rdx asm("rdx");
    asm volatile ("rdtsc"
                  : "=r" (rax), "=r" (rdx));
    rax |= rdx << 32;
    return rax;
}

// Note: success means 'ast' and all children have resolved types
// Must check resolved_type to check if just 'ast' does, but not the children

// TODO: what exactly should the pre-condition and post-condition be for this function?

#if 0
Typecheck_Result infer_types(Infer_Context *ctx, Expr_AST *type_to_match, AST *ast)
{
    Typecheck_Result r1 = typecheck_result(true,false,false);
    Typecheck_Result r2 = r1;
    
    if(ast->flags & AST_FLAG_TYPE_INFERRED_TRANSITIVE)
    {
        return r1;
    }
    defer {
        if(r1.ok && !r1.waiting)
        {
            ast->flags |= AST_FLAG_TYPE_INFERRED_TRANSITIVE;
        }
    };
    
    switch(ast->type)
    {
        case AST_Type::bool_ast: {
            Bool_AST *bool_ast = static_cast<Bool_AST*>(ast);
            
            if(type_to_match)
            {
                Primitive_AST *primitive_ast = nullptr;
                r2 = get_as_primitive_type(type_to_match, &primitive_ast);
                r1 = combine_typecheck_result(r1, r2);
                if(!r1.ok || r2.waiting)
                {
                    return r1;
                }
                
                u64 primitive = primitive_ast->primitive;
                
                if(!(primitive & PRIM_BOOLLIKE))
                {
                    report_error("Type mismatch: got bool", ast);
                    r1.ok = false;
                    return r1;
                }
                
                u64 size = primitive & PRIM_SIZE_MASK;
                
                switch(size)
                {
                    case PRIM_NO_SIZE:
                    case PRIM_SIZE1: {
                        bool_ast->resolved_type = &bool8_t_ast;
                    } break;
                    case PRIM_SIZE2: {
                        bool_ast->resolved_type = &bool16_t_ast;
                    } break;
                    case PRIM_SIZE4: {
                        bool_ast->resolved_type = &bool32_t_ast;
                    } break;
                    case PRIM_SIZE8: {
                        bool_ast->resolved_type = &bool64_t_ast;
                    } break;
                    default: {
                        assert(false);
                    } break;
                }
                return r1;
            }
            else
            {
                bool_ast->resolved_type = &bool8_t_ast;
                return r1;
            }
        } break;
    }
    
    r1.ok = false;
    return r1;
}

#endif

void report_if_expr_untyped(Expr_AST *expr)
{
    if(!expr->resolved_type)
    {
        report_error("Untyped AST", expr);
    }
}

void check_for_untyped(AST *ast)
{
    if(ast)
    {
        switch(ast->type)
        {
            case AST_Type::decl_ast: {
                Decl_AST *decl_ast = static_cast<Decl_AST*>(ast);
                check_for_untyped(&decl_ast->ident);
                check_for_untyped(decl_ast->decl_type);
                check_for_untyped(decl_ast->expr);
            } break;
            case AST_Type::block_ast: {
                Block_AST *block_ast = static_cast<Block_AST*>(ast);
                for(u64 i = 0; i < block_ast->statements.count; ++i)
                {
                    check_for_untyped(block_ast->statements[i]);
                }
            } break;
            case AST_Type::while_ast: {
                While_AST *while_ast = static_cast<While_AST*>(ast);
                check_for_untyped(while_ast->guard);
                check_for_untyped(while_ast->body);
            } break;
            case AST_Type::for_ast: {
                For_AST *for_ast = static_cast<For_AST*>(ast);
                check_for_untyped(for_ast->induction_var);
                if(for_ast->flags & FOR_FLAG_OVER_ARRAY)
                {
                    check_for_untyped(for_ast->index_var);
                    check_for_untyped(for_ast->array_expr);
                }
                else
                {
                    check_for_untyped(for_ast->low_expr);
                    check_for_untyped(for_ast->high_expr);
                }
                check_for_untyped(for_ast->body);
            } break;
            case AST_Type::if_ast: {
                If_AST *if_ast = static_cast<If_AST*>(ast);
                check_for_untyped(if_ast->guard);
                check_for_untyped(if_ast->then_block);
                check_for_untyped(if_ast->else_block);
            } break;
            case AST_Type::assign_ast: {
                Assign_AST *assign_ast = static_cast<Assign_AST*>(ast);
                check_for_untyped(assign_ast->lhs);
                check_for_untyped(assign_ast->rhs);
            } break;
            case AST_Type::return_ast: {
                Return_AST *return_ast = static_cast<Return_AST*>(ast);
                check_for_untyped(return_ast->expr);
            } break;
            case AST_Type::ident_ast: {
                Ident_AST *ident_ast = static_cast<Ident_AST*>(ast);
                report_if_expr_untyped(ident_ast);
            } break;
            case AST_Type::function_type_ast: {
                Function_Type_AST *function_type_ast = static_cast<Function_Type_AST*>(ast);
                report_if_expr_untyped(function_type_ast);
                for(u64 i = 0; i < function_type_ast->parameter_types.count; ++i)
                {
                    check_for_untyped(function_type_ast->parameter_types[i]);
                }
                for(u64 i = 0; i < function_type_ast->return_types.count; ++i)
                {
                    check_for_untyped(function_type_ast->return_types[i]);
                }
            } break;
            case AST_Type::function_ast: {
                Function_AST *function_ast = static_cast<Function_AST*>(ast);
                report_if_expr_untyped(function_ast);
                check_for_untyped(function_ast->prototype);
                for(u64 i = 0; i < function_ast->param_names.count; ++i)
                {
                    check_for_untyped(function_ast->param_names[i]);
                    check_for_untyped(function_ast->default_values[i]);
                }
                check_for_untyped(function_ast->block);
            } break;
            case AST_Type::function_call_ast: {
                Function_Call_AST *function_call_ast = static_cast<Function_Call_AST*>(ast);
                report_if_expr_untyped(function_call_ast);
                check_for_untyped(function_call_ast->function);
                for(u64 i = 0; i < function_call_ast->args.count; ++i)
                {
                    check_for_untyped(function_call_ast->args[i]);
                }
            } break;
            case AST_Type::access_ast: {
                Access_AST *access_ast = static_cast<Access_AST*>(ast);
                report_if_expr_untyped(access_ast);
                check_for_untyped(access_ast->lhs);
            } break;
            case AST_Type::binary_operator_ast: {
                Binary_Operator_AST *binop_ast = static_cast<Binary_Operator_AST*>(ast);
                report_if_expr_untyped(binop_ast);
                check_for_untyped(binop_ast->lhs);
                check_for_untyped(binop_ast->rhs);
            } break;
            case AST_Type::number_ast: {
                Number_AST *number_ast = static_cast<Number_AST*>(ast);
                report_if_expr_untyped(number_ast);
            } break;
            case AST_Type::enum_ast: {
                Enum_AST *enum_ast = static_cast<Enum_AST*>(ast);
                report_if_expr_untyped(enum_ast);
                for(u64 i = 0; i < enum_ast->values.count; ++i)
                {
                    check_for_untyped(enum_ast->values[i]);
                }
            } break;
            case AST_Type::struct_ast: {
                Struct_AST *struct_ast = static_cast<Struct_AST*>(ast);
                report_if_expr_untyped(struct_ast);
                for(u64 i = 0; i < struct_ast->constants.count; ++i)
                {
                    check_for_untyped(struct_ast->constants[i]);
                }
                for(u64 i = 0; i < struct_ast->fields.count; ++i)
                {
                    check_for_untyped(struct_ast->fields[i]);
                }
            } break;
            case AST_Type::unary_ast: {
                Unary_Operator_AST *unop_ast = static_cast<Unary_Operator_AST*>(ast);
                report_if_expr_untyped(unop_ast);
                check_for_untyped(unop_ast->operand);
            } break;
            case AST_Type::primitive_ast: {
                Primitive_AST *prim_ast = static_cast<Primitive_AST*>(ast);
                report_if_expr_untyped(prim_ast);
            } break;
            case AST_Type::string_ast: {
                String_AST *string_ast = static_cast<String_AST*>(ast);
                report_if_expr_untyped(string_ast);
            } break;
            case AST_Type::bool_ast: {
                Bool_AST *bool_ast = static_cast<Bool_AST*>(ast);
                report_if_expr_untyped(bool_ast);
            } break;
        }
    }
}


int main(int argc, char **argv)
{
    // Note: if this becomes multi-threaded, we can get rid of the globals (writes smaller than 4K are supposed to be atomic IIRC)
    // Alternatively, formatting could occur in thread-local buffers, and output is guarded by a global mutex
    init_std_print_buffers();
    init_primitive_types();
    
    String file_contents = read_entire_file("test.txt");
    if(!file_contents.data)
    {
        print_err("Unable to read test.txt\n");
    }
    
    Dynamic_Array<Token> tokens = lex_string(file_contents);
    if(!tokens.data)
    {
        return 1;
    }
    
    Pool_Allocator ast_pool;
    Parsing_Context ctx;
    
    init_pool(&ast_pool, 4096);
    init_parsing_context(&ctx, file_contents, tokens.array, &ast_pool);
    Dynamic_Array<Decl_AST*> decls = parse_tokens(&ctx);
    
    array_trim(&decls);
    
    Atom_Table atom_table;
    init_atom_table(&atom_table, 128, 4096);
    
    Scoping_Context scoping_ctx;
    scoping_ctx.atom_table = &atom_table;
    scoping_ctx.ast_pool = &ast_pool;
    bool success = create_scope_metadata(&scoping_ctx, decls.array);
    if(!success)
    {
        return 1;
    }
    success = typecheck_all(&ast_pool, decls.array);
    if(!success)
    {
        print_err("No success\n");
        return 1;
    }
    
    for(u64 i = 0; i < decls.count; ++i)
    {
        check_for_untyped(decls[i]);
    }
    
    return 0;
}
