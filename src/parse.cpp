
internal Ident_AST make_ident_ast(Token ident)
{
    Ident_AST result;
    result.type = AST_Type::ident_ast;
    result.flags = 0;
    result.line_number = ident.line_number;
    result.line_offset = ident.line_offset;
    result.ident = ident.contents;
    return result;
}

internal AST* parse_type_expr(Array<Token> tokens, u64 i, u64 *i_out)
{
    if(tokens[i].type == Token_Type::ident)
    {
        Ident_AST *result = mem_alloc(Ident_AST, 1);
        *result = make_ident_ast(tokens[i]);
        
        *i_out = ++i;
        return result;
    }
    else
    {
        return nullptr;
    }
}

Dynamic_Array<Decl_AST> parse_tokens(Array<Token> tokens)
{
    Dynamic_Array<Decl_AST> result = {0};
    
    u64 i = 0;
    while(i < tokens.count)
    {
        if(tokens[i].type == Token_Type::ident)
        {
            Decl_AST new_ast;
            zero_struct(&new_ast);
            new_ast.type = AST_Type::decl_ast;
            new_ast.line_number = tokens[i].line_number;
            new_ast.line_offset = tokens[i].line_offset;
            new_ast.ident = make_ident_ast(tokens[i]);
            
            if(++i == tokens.count)
            {
                // TODO: report error
                print_err("parse error 1\n");
                break;
            }
            
            if(tokens[i].type == Token_Type::colon)
            {
                if(++i == tokens.count)
                {
                    // TODO: report error
                    print_err("parse error 2\n");
                    break;
                }
                
                AST *type = parse_type_expr(tokens, i, &i);
                if(type)
                {
                    new_ast.decl_type = type;
                }
                else
                {
                    // TODO: report error
                    print_err("parse error 3\n");
                }
                
                if(i == tokens.count)
                {
                    print_err("parse error 3.1\n");
                }
                if(tokens[i].type == Token_Type::equal)
                {
                    if(++i == tokens.count)
                    {
                        // TODO: report error
                        print_err("parse error 3.2\n");
                        break;
                    }
                    
                    AST *expr = parse_type_expr(tokens, i, &i);
                    if(expr)
                    {
                        new_ast.expr = expr;
                    }
                    else
                    {
                        print_err("parse error 3.3\n");
                    }
                    
                    array_add(&result, new_ast);
                }
            }
            else if(tokens[i].type == Token_Type::double_colon)
            {
                new_ast.decl_type = nullptr;
                if(++i == tokens.count)
                {
                    // TODO: report error
                    print_err("parse error 4\n");
                    break;
                }
            }
            else if(tokens[i].type == Token_Type::colon_eq)
            {
                new_ast.decl_type = nullptr;
                if(++i == tokens.count)
                {
                    // TODO: report error
                    print_err("parse error 5\n");
                    break;
                }
            }
            else
            {
                // TODO: report error
                print_err("parse error 6\n");
                ++i;
            }
        }
        else
        {
            // TODO: report error
            print_err("parse error 7\n");
            ++i;
        }
    }
    
    return result;
}

internal void print_dot_rec(Print_Buffer *pb, AST *ast, u64 *serial);
internal inline void print_dot_child(Print_Buffer *pb, AST *child, u64 parent_serial, u64 *serial)
{
    u64 child_serial = *serial;
    print_buf(pb, "n%ld->n%ld;\n", parent_serial, child_serial);
    print_dot_rec(pb, child, serial);
}

internal void print_dot_rec(Print_Buffer *pb, AST *ast, u64 *serial)
{
    switch(ast->type)
    {
        case AST_Type::ident_ast: {
            Ident_AST *ident_ast = static_cast<Ident_AST*>(ast);
            print_buf(pb, "n%ld[label=\"%.*s\"];\n", (*serial)++, (u32)ident_ast->ident.count, ident_ast->ident.data);
        } break;
        case AST_Type::decl_ast: {
            Decl_AST *decl_ast = static_cast<Decl_AST*>(ast);
            
            u64 this_serial = (*serial)++;
            print_buf(pb, "n%ld[label=\"Declare %.*s\"];\n", this_serial, (u32)decl_ast->ident.ident.count, decl_ast->ident.ident.data);
            if(decl_ast->decl_type)
            {
                print_dot_child(pb, decl_ast->decl_type, this_serial, serial);
            }
            assert(decl_ast->expr);
            print_dot_child(pb, decl_ast->expr, this_serial, serial);
        } break;
        case AST_Type::block_ast: {
            Block_AST *block_ast = static_cast<Block_AST*>(ast);
            
            u64 this_serial = (*serial)++;
            print_buf(pb, "n%ld[label=\"Block\"];\n", this_serial);
            
            for(u64 i = 0; i < block_ast->n_statements; ++i)
            {
                print_dot_child(pb, &block_ast->statements[i], this_serial, serial);
            }
        } break;
        case AST_Type::function_ast: {
            Function_AST *function_ast = static_cast<Function_AST*>(ast);
            
            u64 this_serial = (*serial)++;
            print_buf(pb, "n%ld[label=\"Function\"];\n", this_serial);
            
            for(u64 i = 0; i < function_ast->n_arguments; ++i)
            {
                print_dot_child(pb, &function_ast->arg_names[i], this_serial, serial);
                print_dot_child(pb, &function_ast->arg_types[i], this_serial, serial);
            }
        } break;
        case AST_Type::binary_operator_ast: {
        } break;
        case AST_Type::number_ast: {
            Number_AST *number_ast = static_cast<Number_AST*>(ast);
            print_buf(pb, "n%ld[label=\"%.*s\"];\n", (*serial)++, (u32)number_ast->literal.count, number_ast->literal.data);
        } break;
        default: {
            print_err("Unknown AST type in dot printer\n");
        } break;
    }
}

void print_dot(Print_Buffer *pb, Array<Decl_AST> decls)
{
    print_buf(pb, "digraph decls {\n");
    
    u64 s = 0;
    for(u64 i = 0; i < decls.count; ++i)
    {
        print_dot_rec(pb, &decls[i], &s);
    }
    
    print_buf(pb, "}\n");
    flush_buffer(pb);
}