

internal void print_err_indented(byte *start, byte *end)
{
    while(start < end)
    {
        byte *end_section = start;
        while(end_section < end)
        {
            byte b = *end_section;
            ++end_section;
            
            if(b == '\r')
            {
                if(end_section < end && *end_section == '\n')
                {
                    ++end_section;
                }
                break;
            }
            else if(b == '\n')
            {
                break;
            }
        }
        
        print_err("    %.*s", (u32)(end_section - start), start);
        start = end_section;
    }
}

void report_error(Lexed_String *str, Token *start_section, Token *current,const byte *error_text)
{
    byte *start_highlight = start_section->contents.data;
    byte *start = start_highlight;
    byte *start_program = str->program_text.data;
    while(start > start_program)
    {
        if(*start == '\n' || *start == '\r')
        {
            ++start;
            break;
        }
        else
        {
            --start;
        }
    }
    
    
    byte *start_error;
    byte *end_highlight;
    byte *end;
    
    if(current == &str->tokens[str->tokens.count - 1])
    {
        assert(str->tokens.count > 0);
        
        Token *previous = current - 1;
        byte *start_error = previous->contents.data;
        byte *end_highlight = start_error + previous->contents.count;
        byte *end = end_highlight;
    }
    else
    {
        start_error = current->contents.data;
        end_highlight = start_error + current->contents.count;
        end = end_highlight;
        
        byte *end_program = str->program_text.data + str->program_text.count;
        while(end < end_program)
        {
            if(*end == '\r')
            {
                ++end;
                if(end < end_program && *end == '\n')
                {
                    ++end;
                }
                break;
            }
            else if(*end == '\n')
            {
                ++end;
                break;
            }
            else
            {
                ++end;
            }
        }
    }
    
    print_err("\x1B[1;31mError\x1B[0m: %d:%d:\n    %s\n", current->line_number, current->line_offset, error_text);
    
    print_err_indented(start, start_highlight);
    
    print_err("\x1B[1;33m");
    print_err_indented(start_highlight, start_error);
    
    print_err("\x1B[1;31m");
    print_err_indented(start_error, end_highlight);
    
    print_err("\x1B[0m");
    print_err_indented(end_highlight, end);
}

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

internal Number_AST make_number_ast(Token number)
{
    Number_AST result;
    result.type = AST_Type::number_ast;
    result.flags = 0;
    result.line_number = number.line_number;
    result.line_offset = number.line_offset;
    result.literal = number.contents;
    return result;
}

// TODO: fix memory leaks. Make parser allocator


/* Recall: Weakest binding operators are at the 'top' of the grammar
*
* expr := mul_expr + expr 
*       | mul_expr - expr
*       | mul_expr
* mul_expr := root_expr * expr
*           | root_expr / expr
*           | root_expr
* root_expr := (expr)
*              | number
*              | function_call
*              | ident
* function_call := ident ( arg_list )
*/
internal AST* parse_expr(Lexed_String *str, Token **current_ptr, bool required)
{
    Token *current = *current_ptr;
    Token *start_section = current;
    AST *lhs = nullptr;
    
    if(current->type == Token_Type::ident)
    {
        Ident_AST *lhs_ident = mem_alloc(Ident_AST, 1);
        *lhs_ident = make_ident_ast(*current);
        lhs = lhs_ident;
        ++current;
        
        // TODO: parse function call
    }
    else if(current->type == Token_Type::open_paren)
    {
        ++current;
        
        lhs = parse_expr(str, &current, required);
        if(lhs)
        {
            if(current->type == Token_Type::close_paren)
            {
                ++current;
            }
            else
            {
                if(required)
                {
                    report_error(str, start_section, current, "Expected close parenthesis");
                }
                return nullptr;
            }
        }
        else
        {
            return nullptr;
        }
        
        // TODO: lambdas/function literals ?
    }
    else if(current->type == Token_Type::number)
    {
        Number_AST *lhs_number = mem_alloc(Number_AST, 1);
        *lhs_number = make_number_ast(*current);
        lhs = lhs_number;
        ++current;
    }
    
    *current_ptr = current;
    return lhs;
}

Dynamic_Array<Decl_AST> parse_tokens(Lexed_String *str)
{
    Dynamic_Array<Decl_AST> result = {0};
    
    Token *current = str->tokens.data;
    Token *start_section = current;
    
    while(current->type != Token_Type::eof)
    {
        if(current->type == Token_Type::ident)
        {
            Decl_AST new_ast;
            zero_struct(&new_ast);
            new_ast.type = AST_Type::decl_ast;
            new_ast.line_number = current->line_number;
            new_ast.line_offset = current->line_offset;
            new_ast.ident = make_ident_ast(*current);
            
            ++current;
            
            if(current->type == Token_Type::colon)
            {
                ++current;
                
                AST *type = parse_expr(str, &current, true);
                if(type)
                {
                    new_ast.decl_type = type;
                }
                else
                {
                    break;
                }
                
                if(current->type == Token_Type::equal)
                {
                    ++current;
                    
                    AST *expr = parse_expr(str, &current, true);
                    if(expr)
                    {
                        new_ast.expr = expr;
                    }
                    else
                    {
                        break;
                    }
                    
                    array_add(&result, new_ast);
                    start_section = current;
                }
                else
                {
                    report_error(str, start_section, current, "Declaration with no value. Expected '='");
                    break;
                }
            }
            else if(current->type == Token_Type::double_colon)
            {
                report_error(str, start_section, current, "Not yet implemented, type is required");
                ++current;
                new_ast.decl_type = nullptr;
                
                break;
            }
            else if(current->type == Token_Type::colon_eq)
            {
                report_error(str, start_section, current, "Not yet implemented, type is required");
                ++current;
                new_ast.decl_type = nullptr;
                
                break;
            }
            else
            {
                report_error(str, start_section, current, "Expected ':=', or '::' to make a declaration");
                break;
            }
        }
        else
        {
            report_error(str, start_section, current, "Expected a top-level declaration");
            ++current;
            start_section = current;
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