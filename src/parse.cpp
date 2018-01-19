
void init_parsing_context(Parsing_Context *ctx, String program_text, Array<Token> tokens, u64 pool_block_size)
{
    ctx->program_text = program_text;
    ctx->tokens = tokens;
    init_pool(&ctx->ast_pool, pool_block_size);
}

void report_error(Parsing_Context *ctx, Token *start_section, Token *current,const byte *error_text)
{
    byte *start_highlight = start_section->contents.data;
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
    
    bool error_at_eof = (current == &ctx->tokens[ctx->tokens.count - 1]);
    byte *end_program = ctx->program_text.data + ctx->program_text.count;
    
    byte *start_error;
    byte *end_highlight;
    byte *end;
    
    if(error_at_eof)
    {
        start_error = end_program;
        end_highlight = end_program;
        end = end_program;
    }
    else
    {
        start_error = current->contents.data;
        end_highlight = start_error + current->contents.count;
        end = end_highlight;
        
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
    print_err_indented(start_highlight, start_error, false);
    
    if(error_at_eof)
    {
        print_err("\x1B[1;31mEOF\x1B[0m\n");
    }
    else
    {
        print_err("\x1B[1;31m");
        print_err_indented(start_error, end_highlight, false);
        
        print_err("\x1B[0m");
        print_err_indented(end_highlight, end, false);
    }
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


internal Block_AST *parse_statement_block(Parsing_Context *ctx, Token **current_ptr);
internal AST *parse_base_expr(Parsing_Context *ctx, Token **current_ptr);
internal AST* parse_expr(Parsing_Context *ctx, Token **current_ptr, u32 precedence = 3)
{
    Token *current = *current_ptr;
    Token *start_section = current;
    
    AST *lhs = parse_base_expr(ctx, &current);
    if(!lhs)
    {
        return nullptr;
    }
    
    while(true)
    {
        Binary_Operator op;
        bool found_op;
        u32 rhs_precedence;
        
        switch(precedence)
        {
            case 3:
            if(current->type == Token_Type::add)
            {
                op = Binary_Operator::add;
                found_op = true;
                rhs_precedence = 2;
                break;
            }
            if(current->type == Token_Type::sub)
            {
                op = Binary_Operator::sub;
                found_op = true;
                rhs_precedence = 2;
                break;
            }
            case 2:
            if(current->type == Token_Type::mul)
            {
                op = Binary_Operator::mul;
                found_op = true;
                rhs_precedence = 1;
                break;
            }
            if(current->type == Token_Type::div)
            {
                op = Binary_Operator::div;
                found_op = true;
                rhs_precedence = 1;
                break;
            }
            case 1:
            if(current->type == Token_Type::open_paren)
            {
                op = Binary_Operator::call;
                
                ++current;
                AST *rhs = parse_expr(ctx, &current);
                if(rhs)
                {
                    if(current->type == Token_Type::close_paren)
                    {
                        ++current;
                        
                        Binary_Operator_AST *result = pool_alloc(Binary_Operator_AST, &ctx->ast_pool, 1);
                        result->type = AST_Type::binary_operator_ast;
                        result->flags = 0;
                        result->line_number = lhs->line_number;
                        result->line_offset = lhs->line_offset;
                        result->op = op;
                        result->lhs = lhs;
                        result->rhs = rhs;
                        
                        lhs = result;
                        
                        continue;
                    }
                    else
                    {
                        report_error(ctx, start_section, current, "Expected ')'");
                        return nullptr;
                    }
                }
                else
                {
                    return nullptr;
                }
                
                break;
            }
            if(current->type == Token_Type::open_sqr)
            {
                op = Binary_Operator::subscript;
                
                ++current;
                AST *rhs = parse_expr(ctx, &current);
                if(rhs)
                {
                    if(current->type == Token_Type::close_sqr)
                    {
                        ++current;
                        
                        Binary_Operator_AST *result = pool_alloc(Binary_Operator_AST, &ctx->ast_pool, 1);
                        result->type = AST_Type::binary_operator_ast;
                        result->flags = 0;
                        result->line_number = lhs->line_number;
                        result->line_offset = lhs->line_offset;
                        result->op = op;
                        result->lhs = lhs;
                        result->rhs = rhs;
                        
                        lhs = result;
                        
                        continue;
                    }
                    else
                    {
                        report_error(ctx, start_section, current, "Expected ']'");
                        return nullptr;
                    }
                }
                else
                {
                    return nullptr;
                }
                
                break;
            }
            if(current->type == Token_Type::dot)
            {
                op = Binary_Operator::access;
                
                ++current;
                if(current->type == Token_Type::ident)
                {
                    Ident_AST *rhs = pool_alloc(Ident_AST, &ctx->ast_pool, 1);
                    *rhs = make_ident_ast(*current);
                    ++current;
                    
                    Binary_Operator_AST *result = pool_alloc(Binary_Operator_AST, &ctx->ast_pool, 1);
                    result->type = AST_Type::binary_operator_ast;
                    result->flags = 0;
                    result->line_number = lhs->line_number;
                    result->line_offset = lhs->line_offset;
                    result->op = op;
                    result->lhs = lhs;
                    result->rhs = rhs;
                    
                    lhs = result;
                    
                    continue;
                }
                else
                {
                    report_error(ctx, start_section, current, "Expected identifier");
                    return nullptr;
                }
                
                break;
            }
            /*
                if(current->type == Token_Type::double_plus)
                {
                    break;
                }
                if(current->type == Token_Type::double_minus)
                {
                    break;
                }
                */
            case 0:
            found_op = false;
            break;
            default: {
                assert(false);
            } break;
        }
        
        if(found_op)
        {
            ++current;
            AST *rhs = parse_expr(ctx, &current, rhs_precedence);
            if(rhs)
            {
                Binary_Operator_AST *result = pool_alloc(Binary_Operator_AST, &ctx->ast_pool, 1);
                result->type = AST_Type::binary_operator_ast;
                result->flags = 0;
                result->line_number = lhs->line_number;
                result->line_offset = lhs->line_offset;
                result->op = op;
                result->lhs = lhs;
                result->rhs = rhs;
                
                lhs = result;
            }
            else
            {
                return nullptr;
            }
        }
        else
        {
            *current_ptr = current;
            return lhs;
        }
    }
}

internal AST *parse_base_expr(Parsing_Context *ctx, Token **current_ptr)
{
    Token *current = *current_ptr;
    Token *start_section = current;
    AST *result = nullptr;
    
    // TODO: lambdas/function literals ?
    if(current->type == Token_Type::ident)
    {
        Ident_AST *result_ident = pool_alloc(Ident_AST, &ctx->ast_pool, 1);
        *result_ident = make_ident_ast(*current);
        result = result_ident;
        ++current;
    }
    else if(current->type == Token_Type::open_paren)
    {
        u32 line_number = current->line_number;
        u32 line_offset = current->line_offset;
        
        ++current;
        bool expect_func = false;
        Ident_AST **param_names = nullptr; 
        AST **param_types = nullptr;
        u64 n_parameters = 0;
        
        if(current->type == Token_Type::close_paren)
        {
            // expect function literal with no parameters
            ++current;
            expect_func = true;
        }
        else
        {
            AST *first_expr = parse_expr(ctx, &current);
            if(current->type == Token_Type::close_paren)
            {
                // Just a parethesized expression
                ++current;
                result = first_expr;
            }
            else
            {
                expect_func = true;
                
                if(first_expr->type != AST_Type::ident_ast)
                {
                    report_error(ctx, start_section, current, "Expected ')'");
                    return nullptr;
                }
                if(current->type != Token_Type::colon)
                {
                    report_error(ctx, start_section, current, "Expected ':' in parameter list");
                    return nullptr;
                }
                
                ++current;
                
                Ident_AST *ident = static_cast<Ident_AST*>(first_expr);
                AST *type = parse_expr(ctx, &current);
                if(!type)
                {
                    return nullptr;
                }
                
                // TODO create a temporary allocator for things like this ?
                // In any case, this is currently a memory leak
                Dynamic_Array<Parameter_AST> parameters = {0};
                array_add(&parameters, {ident,type});
                
                while(true)
                {
                    if(current->type == Token_Type::comma)
                    {
                        ++current;
                        Parameter_AST param;
                        if(current->type == Token_Type::ident)
                        {
                            Ident_AST *ident = pool_alloc(Ident_AST, &ctx->ast_pool, 1);
                            *ident = make_ident_ast(*current);
                            param.name = ident;
                            ++current;
                            if(current->type == Token_Type::colon)
                            {
                                ++current;
                                param.type= parse_expr(ctx, &current);
                                if(param.type)
                                {
                                    array_add(&parameters, param);
                                }
                                else
                                {
                                    return nullptr;
                                }
                            }
                            else
                            {
                                report_error(ctx, start_section, current, "Expected ':'");
                                return nullptr;
                            }
                        }
                        else
                        {
                            report_error(ctx, start_section, current, "Expected identifier");
                            return nullptr;
                        }
                    }
                    else if(current->type == Token_Type::close_paren)
                    {
                        ++current;
                        break;
                    }
                    else
                    {
                        report_error(ctx, start_section, current, "Expected ',' or ')'");
                        return nullptr;
                    }
                }
                
                n_parameters = parameters.count;
                param_names = pool_alloc(Ident_AST*, &ctx->ast_pool, n_parameters);
                param_types = pool_alloc(AST*, &ctx->ast_pool, n_parameters);
                
                for(u64 i = 0; i < n_parameters; ++i)
                {
                    param_names[i] = parameters[i].name;
                    param_types[i] = parameters[i].type;
                }
            }
        }
        
        if(expect_func)
        {
            // TODO review code here
            // TODO: parse return type here, if expect_func_body
            // TODO: parse block here, if expect_func_body
            
            AST *return_type = nullptr;
            
            if(current->type == Token_Type::arrow)
            {
                ++current;
                return_type = parse_expr(ctx, &current);
                if(!return_type)
                {
                    return nullptr;
                }
            }
            
            Block_AST *block = parse_statement_block(ctx, &current);
            if(block)
            {
                
                Function_Type_AST *func_type = pool_alloc(Function_Type_AST, &ctx->ast_pool, 1);
                
                func_type->type = AST_Type::function_type_ast;
                func_type->flags = 0;
                func_type->line_number = line_number;
                func_type->line_offset = line_offset;
                
                func_type->parameter_types.count = n_parameters;
                func_type->parameter_types.data = param_types;
                if(return_type)
                {
                    func_type->return_types.count = 1;
                    func_type->return_types.data = pool_alloc(AST*, &ctx->ast_pool, 1);
                    func_type->return_types[0] = return_type;
                }
                else
                {
                    func_type->return_types = {0};
                }
                
                Function_AST *result_func = pool_alloc(Function_AST, &ctx->ast_pool, 1);
                result_func->type = AST_Type::function_ast;
                result_func->flags = 0;
                result_func->line_number = line_number;
                result_func->line_offset = line_offset;
                result_func->prototype = func_type;
                result_func->param_names.count = n_parameters;
                result_func->param_names.data = param_names;
                result_func->block = block;
                result = result_func;
            }
            else
            {
                return nullptr;
            }
        }
    }
    else if(current->type == Token_Type::number)
    {
        Number_AST *result_number = pool_alloc(Number_AST, &ctx->ast_pool, 1);
        *result_number = make_number_ast(*current);
        result = result_number;
        ++current;
    }
    
    if(result)
    {
        *current_ptr = current;
    }
    else
    {
        report_error(ctx, start_section, current, "Expected expression");
    }
    return result;
}

internal Block_AST *parse_statement_block(Parsing_Context *ctx, Token **current_ptr)
{
    Token *current = *current_ptr;
    Token *start_section = current;
    Block_AST *result = nullptr;
    
    u32 line_number = current->line_number;
    u32 line_offset = current->line_offset;
    
    if(current->type == Token_Type::open_brace)
    {
        ++current;
        
        // TODO: parse statements
        
        if(current->type == Token_Type::close_brace)
        {
            ++current;
            result = pool_alloc(Block_AST, &ctx->ast_pool, 1);
            zero_struct(result);
            
            result->type = AST_Type::block_ast;
            result->flags = 0;
            result->line_number = line_number;
            result->line_offset = line_offset;
        }
    }
    else
    {
        report_error(ctx, start_section, current, "Expected '{' to begin statement block");
    }
    
    if(result)
    {
        *current_ptr = current;
    }
    return result;
}


Dynamic_Array<Decl_AST> parse_tokens(Parsing_Context *ctx)
{
    Dynamic_Array<Decl_AST> result = {0};
    
    Token *current = ctx->tokens.data;
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
                
                AST *type = parse_expr(ctx, &current);
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
                    
                    AST *expr = parse_expr(ctx, &current);
                    if(expr)
                    {
                        new_ast.expr = expr;
                    }
                    else
                    {
                        break;
                    }
                    if(current->type == Token_Type::semicolon)
                    {
                        ++current;
                        array_add(&result, new_ast);
                        start_section = current;
                    }
                    else
                    {
                        report_error(ctx, start_section, current, "Expected ';' after declaration");
                    }
                }
                else
                {
                    report_error(ctx, start_section, current, "Declaration with no value. Expected '='");
                    break;
                }
            }
            else if(current->type == Token_Type::double_colon)
            {
                report_error(ctx, start_section, current, "Not yet implemented, type is required");
                ++current;
                new_ast.decl_type = nullptr;
                
                break;
            }
            else if(current->type == Token_Type::colon_eq)
            {
                report_error(ctx, start_section, current, "Not yet implemented, type is required");
                ++current;
                new_ast.decl_type = nullptr;
                
                break;
            }
            else
            {
                report_error(ctx, start_section, current, "Expected ':=', or '::' to make a declaration");
                break;
            }
        }
        else
        {
            report_error(ctx, start_section, current, "Expected a top-level declaration");
            // TODO: could skip to semicolon for a better recovery
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
            
            for(u64 i = 0; i < block_ast->statements.count; ++i)
            {
                print_dot_child(pb, block_ast->statements[i], this_serial, serial);
            }
        } break;
        case AST_Type::function_type_ast: {
            Function_Type_AST *type_ast = static_cast<Function_Type_AST*>(ast);
            u64 this_serial = (*serial)++;
            print_buf(pb, "n%ld[label=\"Function Type\"];\n", this_serial);
            
            for(u64 i = 0; i < type_ast->parameter_types.count; ++i)
            {
                print_dot_child(pb, type_ast->parameter_types[i], this_serial, serial);
            }
            for(u64 i = 0; i < type_ast->return_types.count; ++i)
            {
                print_dot_child(pb, type_ast->return_types[i], this_serial, serial);
            }
        } break;
        case AST_Type::function_ast: {
            Function_AST *function_ast = static_cast<Function_AST*>(ast);
            
            u64 this_serial = (*serial)++;
            print_buf(pb, "n%ld[label=\"Function\"];\n", this_serial);
            
            print_dot_child(pb, function_ast->prototype, this_serial, serial);
            for(u64 i = 0; i < function_ast->param_names.count; ++i)
            {
                print_dot_child(pb, function_ast->param_names[i], this_serial, serial);
            }
            print_dot_child(pb, function_ast->block, this_serial, serial);
        } break;
        case AST_Type::binary_operator_ast: {
            Binary_Operator_AST *bin_ast = static_cast<Binary_Operator_AST*>(ast);
            
            u64 this_serial = (*serial)++;
            print_buf(pb, "n%ld[label=\"%s\"];\n", this_serial, binary_operator_names[(u64)bin_ast->op]);
            print_dot_child(pb, bin_ast->lhs, this_serial, serial);
            print_dot_child(pb, bin_ast->rhs, this_serial, serial);
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