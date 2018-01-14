
Parsing_Context make_parsing_context(String program_text, Array<Token> tokens)
{
    Parsing_Context result = {0};
    result.program_text = program_text;
    result.begin_tokens = tokens.data;
    result.end_tokens = tokens.data + tokens.count;
    
    result.start_section = tokens.data;
    result.current = tokens.data;
    
    return result;
}

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

void report_error(Parsing_Context *ctx, const byte *error_text)
{
    assert(ctx->start_section < ctx->end_tokens);
    assert(ctx->current < ctx->end_tokens);
    
    Token *first_tok = ctx->start_section;
    Token *last_tok = ctx->current;
    
    byte *start_highlight = first_tok->contents.data;
    byte *start = start_highlight;
    byte *start_program = ctx->program_text.data;
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
    
    if(last_tok == ctx->end_tokens - 1)
    {
        assert(ctx->begin_tokens != ctx->end_tokens);
        
        Token *snd_last_tok = last_tok - 1;
        byte *start_error = snd_last_tok->contents.data;
        byte *end_highlight = start_error + snd_last_tok->contents.count;
        byte *end = end_highlight;
    }
    else
    {
        start_error = last_tok->contents.data;
        end_highlight = start_error + last_tok->contents.count;
        end = end_highlight;
        
        byte *end_program = ctx->program_text.data + ctx->program_text.count;
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
    
    print_err("\x1B[1;31mError\x1B[0m: %d:%d:\n    %s\n", last_tok->line_number, last_tok->line_offset, error_text);
    
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
internal AST* parse_expr(Parsing_Context *ctx, bool required)
{
    Parsing_Context sub_ctx;
    AST *lhs = nullptr;
    
    if(ctx->current->type == Token_Type::ident)
    {
        Ident_AST *lhs_ident = mem_alloc(Ident_AST, 1);
        *lhs_ident = make_ident_ast(*ctx->current);
        lhs = lhs_ident;
        ++ctx->current;
        
        // TODO: parse function call
    }
    else if(ctx->current->type == Token_Type::open_paren)
    {
        ++ctx->current;
        
        sub_ctx = *ctx;
        lhs = parse_expr(&sub_ctx, required);
        if(lhs)
        {
            *ctx = sub_ctx;
            
            if(ctx->current->type == Token_Type::close_paren)
            {
                ++ctx->current;
            }
            else
            {
                if(required)
                {
                    report_error(ctx, "Expected close parenthesis");
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
    else if(ctx->current->type == Token_Type::number)
    {
        Number_AST *lhs_number = mem_alloc(Number_AST, 1);
        *lhs_number = make_number_ast(*ctx->current);
        lhs = lhs_number;
        ++ctx->current;
    }
    
    return lhs;
}

Dynamic_Array<Decl_AST> parse_tokens(Parsing_Context *ctx)
{
    Dynamic_Array<Decl_AST> result = {0};
    Parsing_Context sub_ctx;
    
    while(ctx->current->type != Token_Type::eof)
    {
        if(ctx->current->type == Token_Type::ident)
        {
            Decl_AST new_ast;
            zero_struct(&new_ast);
            new_ast.type = AST_Type::decl_ast;
            new_ast.line_number = ctx->current->line_number;
            new_ast.line_offset = ctx->current->line_offset;
            new_ast.ident = make_ident_ast(*ctx->current);
            
            ++ctx->current;
            
            if(ctx->current->type == Token_Type::colon)
            {
                ++ctx->current;
                
                sub_ctx = *ctx;
                AST *type = parse_expr(&sub_ctx, true);
                if(type)
                {
                    *ctx = sub_ctx;
                    new_ast.decl_type = type;
                }
                else
                {
                    break;
                }
                
                if(ctx->current->type == Token_Type::equal)
                {
                    ++ctx->current;
                    
                    sub_ctx = *ctx;
                    AST *expr = parse_expr(&sub_ctx, true);
                    if(expr)
                    {
                        *ctx = sub_ctx;
                        new_ast.expr = expr;
                    }
                    else
                    {
                        break;
                    }
                    
                    array_add(&result, new_ast);
                    ctx->start_section = ctx->current;
                }
                else
                {
                    report_error(ctx, "Declaration with no value. Expected '='");
                    break;
                }
            }
            else if(ctx->current->type == Token_Type::double_colon)
            {
                report_error(ctx, "Not yet implemented, type is required");
                ++ctx->current;
                new_ast.decl_type = nullptr;
                
                break;
            }
            else if(ctx->current->type == Token_Type::colon_eq)
            {
                report_error(ctx, "Not yet implemented, type is required");
                ++ctx->current;
                new_ast.decl_type = nullptr;
                
                break;
            }
            else
            {
                report_error(ctx, "Expected ':=', or '::' to make a declaration");
                break;
            }
        }
        else
        {
            report_error(ctx, "Expected a top-level declaration");
            ++ctx->current;
            ctx->start_section = ctx->current;
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