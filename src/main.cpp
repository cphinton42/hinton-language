

AST *find_ident(Array<Decl_AST*> globals, String ident, Parent_Scope scope)
{
    while(true)
    {
        if(scope.ast)
        {
            AST *parent_ast = scope.ast;
            switch(parent_ast->type)
            {
                case AST_Type::decl_ast: {
                    Decl_AST *decl_ast = static_cast<Decl_AST*>(parent_ast);
                    if(decl_ast->ident.ident == ident)
                    {
                        return decl_ast;
                    }
                    scope = decl_ast->parent;
                } break;
                case AST_Type::block_ast: {
                    Block_AST *block_ast = static_cast<Block_AST*>(parent_ast);
                    for(u64 i = 0; i <= scope.index; ++i)
                    {
                        assert(i < block_ast->statements.count);
                        if(block_ast->statements[i]->type == AST_Type::decl_ast)
                        {
                            Decl_AST *decl_ast = static_cast<Decl_AST*>(block_ast->statements[i]);
                            if(decl_ast->ident.ident == ident)
                            {
                                return decl_ast;
                            }
                        }
                    }
                    scope = block_ast->parent;
                } break;
                case AST_Type::function_ast: {
                    Function_AST *function_ast = static_cast<Function_AST*>(parent_ast);
                    for(u64 i = 0; i < function_ast->param_names.count; ++i)
                    {
                        if(function_ast->param_names[i] && function_ast->param_names[i]->ident == ident)
                        {
                            return function_ast->param_names[i];
                        }
                    }
                    // TODO: this is difference between regular functions and closures
                    // scope = function_ast->parent;
                    scope = {0};
                } break;
                case AST_Type::for_ast: {
                    For_AST *for_ast = static_cast<For_AST*>(parent_ast);
                    if(for_ast->induction_var->ident == ident)
                    {
                        return for_ast->induction_var;
                    }
                    else if(for_ast->flags & FOR_FLAG_OVER_ARRAY && for_ast->index_var->ident == ident)
                    {
                        return for_ast->index_var;
                    }
                    scope = for_ast->parent;
                } break;
                default: {
                    assert(false);
                } break;
            }
        }
        else
        {
            for(u64 i = 0; i < globals.count; ++i)
            {
                if(globals[i]->ident.ident == ident)
                {
                    return globals[i];
                }
            }
            return nullptr;
        }
    }
}

void link_ast(Array<Decl_AST*> globals, AST *current)
{
    
    switch(current->type)
    {
        case AST_Type::ident_ast: {
            Ident_AST *ident_ast = static_cast<Ident_AST*>(current);
            ident_ast->referred_to = find_ident(globals, ident_ast->ident, ident_ast->parent);
            if(!ident_ast->referred_to)
            {
                // TODO: better error reporting
                print_err("Error: %d:%d: Undeclared identifier \"%.*s\"\n", ident_ast->line_number, ident_ast->line_offset, ident_ast->ident.count, ident_ast->ident.data);
            }
        } break;
        case AST_Type::decl_ast: {
            Decl_AST *decl_ast = static_cast<Decl_AST*>(current);
            if(decl_ast->decl_type)
            {
                link_ast(globals, decl_ast->decl_type);
            }
            if(decl_ast->expr)
            {
                link_ast(globals, decl_ast->expr);
            }
        } break;
        case AST_Type::block_ast: {
            Block_AST *block_ast = static_cast<Block_AST*>(current);
            for(u64 i = 0; i < block_ast->statements.count; ++i)
            {
                link_ast(globals, block_ast->statements[i]);
            }
        } break;
        case AST_Type::function_type_ast: {
            Function_Type_AST *function_type_ast = static_cast<Function_Type_AST*>(current);
            for(u64 i = 0; i < function_type_ast->parameter_types.count; ++i)
            {
                if(function_type_ast->parameter_types[i])
                {
                    link_ast(globals, function_type_ast->parameter_types[i]);
                }
            }
            for(u64 i = 0; i < function_type_ast->return_types.count; ++i)
            {
                if(function_type_ast->return_types[i])
                {
                    link_ast(globals, function_type_ast->return_types[i]);
                }
            }
        } break;
        case AST_Type::function_ast: {
            Function_AST *function_ast = static_cast<Function_AST*>(current);
            link_ast(globals, function_ast->prototype);
            for(u64 i = 0; i < function_ast->default_values.count; ++i)
            {
                if(function_ast->default_values[i])
                {
                    link_ast(globals, function_ast->default_values[i]);
                }
            }
            link_ast(globals, function_ast->block);
        } break;
        case AST_Type::function_call_ast: {
            Function_Call_AST *function_call_ast = static_cast<Function_Call_AST*>(current);
            for(u64 i = 0; i < function_call_ast->args.count; ++i)
            {
                link_ast(globals, function_call_ast->args[i]);
            }
            link_ast(globals, function_call_ast->function);
        } break;
        case AST_Type::binary_operator_ast: {
            Binary_Operator_AST *binary_operator_ast = static_cast<Binary_Operator_AST*>(current);
            link_ast(globals, binary_operator_ast->lhs);
            if(binary_operator_ast->op != Binary_Operator::access)
            {
                link_ast(globals, binary_operator_ast->rhs);
            }
        } break;
        case AST_Type::while_ast: {
            While_AST *while_ast = static_cast<While_AST*>(current);
            link_ast(globals, while_ast->guard);
            link_ast(globals, while_ast->body);
        } break;
        case AST_Type::for_ast: {
            For_AST *for_ast = static_cast<For_AST*>(current);
            if(for_ast->flags & FOR_FLAG_OVER_ARRAY)
            {
                link_ast(globals, for_ast->array_expr);
            }
            else
            {
                link_ast(globals, for_ast->low_expr);
                link_ast(globals, for_ast->high_expr);
            }
            link_ast(globals, for_ast->body);
        } break;
        case AST_Type::if_ast: {
            If_AST *if_ast = static_cast<If_AST*>(current);
            link_ast(globals, if_ast->guard);
            link_ast(globals, if_ast->then_block);
            if(if_ast->else_block)
            {
                link_ast(globals, if_ast->else_block);
            }
        } break;
        case AST_Type::enum_ast: {
            Enum_AST *enum_ast = static_cast<Enum_AST*>(current);
            for(u64 i = 0; i < enum_ast->values.count; ++i)
            {
                link_ast(globals, enum_ast->values[i]);
            }
        } break;
        case AST_Type::struct_ast: {
            Struct_AST *struct_ast = static_cast<Struct_AST*>(current);
            for(u64 i = 0; i < struct_ast->constants.count; ++i)
            {
                link_ast(globals, struct_ast->constants[i]);
            }
            for(u64 i = 0; i < struct_ast->fields.count; ++i)
            {
                link_ast(globals, struct_ast->fields[i]);
            }
        } break;
        case AST_Type::assign_ast: {
            Assign_AST *assign_ast = static_cast<Assign_AST*>(current);
            link_ast(globals, &assign_ast->ident);
            link_ast(globals, assign_ast->rhs);
        } break;
        case AST_Type::unary_ast: {
            Unary_Operator_AST *unary_operator_ast = static_cast<Unary_Operator_AST*>(current);
            link_ast(globals, unary_operator_ast->operand);
        } break;
        case AST_Type::return_ast: {
            Return_AST *return_ast = static_cast<Return_AST*>(current);
            link_ast(globals, return_ast->expr);
        } break;
        case AST_Type::number_ast: 
        case AST_Type::primitive_ast:
        case AST_Type::string_ast:
        case AST_Type::bool_ast: {
        } break;
        default: {
            assert(false);
        } break;
    }
}

void link_all(Array<Decl_AST*> globals)
{
    for(u64 i = 0; i < globals.count; ++i)
    {
        link_ast(globals, globals[i]);
    }
}

int main(int argc, char **argv)
{
    // Note: if this becomes multi-threaded, we can get rid of the globals (writes smaller than 4K are supposed to be atomic IIRC)
    // Alternatively, formatting could occur in thread-local buffers, and output is guarded by a global mutex
    init_std_print_buffers();
    
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
    
    Parsing_Context ctx;
    init_parsing_context(&ctx, file_contents, tokens.array, 4096);
    Dynamic_Array<Decl_AST*> decls = parse_tokens(&ctx);
    
    link_all(decls.array);
    
    print_dot(&stdout_buf, decls.array);
    
    return 0;
}
