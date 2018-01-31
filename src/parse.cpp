
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

enum class Decl_Type
{
    Statement,
    Struct,
    Enum,
};

internal Decl_AST *parse_decl(Parsing_Context *ctx, Token **current_ptr, Decl_Type type);
internal AST *parse_statement(Parsing_Context *ctx, Token **current_ptr);
internal Block_AST *parse_statement_block(Parsing_Context *ctx, Token **current_ptr);
internal AST *parse_base_expr(Parsing_Context *ctx, Token **current_ptr);

internal AST* parse_expr(Parsing_Context *ctx, Token **current_ptr, u32 precedence = 3)
{
    Token *current = *current_ptr;
    Token *start_section = current;
    defer {
        *current_ptr = current;
    };
    
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
                rhs_precedence = precedence - 1;
                break;
            }
            if(current->type == Token_Type::sub)
            {
                op = Binary_Operator::sub;
                found_op = true;
                rhs_precedence = precedence - 1;
                break;
            }
            case 2:
            if(current->type == Token_Type::mul)
            {
                op = Binary_Operator::mul;
                found_op = true;
                rhs_precedence = precedence - 1;
                break;
            }
            if(current->type == Token_Type::div)
            {
                op = Binary_Operator::div;
                found_op = true;
                rhs_precedence = precedence - 1;
                break;
            }
            case 1:
            if(current->type == Token_Type::open_paren)
            {
                ++current;
                
                Dynamic_Array<AST*> args = {0};
                
                if(current->type != Token_Type::eof && current->type != Token_Type::close_paren)
                {
                    AST *first_expr = parse_expr(ctx, &current);
                    if(!first_expr)
                    {
                        return nullptr;
                    }
                    array_add(&args, first_expr);
                }
                
                while(current->type != Token_Type::eof && current->type != Token_Type::close_paren)
                {
                    if(current->type != Token_Type::comma)
                    {
                        report_error(ctx, start_section, current, "Expected ','");
                        return nullptr;
                    }
                    ++current;
                    
                    AST *expr = parse_expr(ctx, &current);
                    if(!expr)
                    {
                        return nullptr;
                    }
                    array_add(&args, expr);
                }
                if(current->type != Token_Type::close_paren)
                {
                    report_error(ctx, start_section, current, "Expected ')'");
                    return nullptr;
                }
                ++current;
                
                Function_Call_AST *call_ast = pool_alloc(Function_Call_AST, &ctx->ast_pool, 1);
                call_ast->type = AST_Type::function_call_ast;
                call_ast->flags = 0;
                call_ast->line_number = lhs->line_number;
                call_ast->line_offset = lhs->line_offset;
                
                call_ast->function = lhs;
                
                call_ast->args.count = args.count;
                call_ast->args.data = pool_alloc(AST*, &ctx->ast_pool, args.count);
                
                for(u64 i = 0; i < args.count; ++i)
                {
                    call_ast->args[i] = args[i];
                }
                
                // TODO memory leak
                lhs = call_ast;
                
                continue;
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
            return lhs;
        }
    }
}

internal AST *parse_base_expr(Parsing_Context *ctx, Token **current_ptr)
{
    Token *current = *current_ptr;
    Token *start_section = current;
    AST *result = nullptr;
    defer {
        *current_ptr = current;
    };
    
    if(current->type == Token_Type::ident)
    {
        Ident_AST *result_ident = pool_alloc(Ident_AST, &ctx->ast_pool, 1);
        *result_ident = make_ident_ast(*current);
        result = result_ident;
        ++current;
    }
    else if(current->type == Token_Type::key_enum)
    {
        u32 line_number = current->line_number;
        u32 line_offset = current->line_offset;
        
        ++current;
        Dynamic_Array<Decl_AST*> values = {0};
        
        if(current->type != Token_Type::open_brace)
        {
            report_error(ctx, start_section, current, "Expected '{'");
            return nullptr;
        }
        ++current;
        
        while(current->type != Token_Type::eof && current->type != Token_Type::close_brace)
        {
            Decl_AST *decl = parse_decl(ctx, &current, Decl_Type::Enum);
            if(!decl)
            {
                return nullptr;
            }
            array_add(&values, decl);
        }
        
        if(current->type != Token_Type::close_brace)
        {
            report_error(ctx, start_section, current, "Expected '}'");
            return nullptr;
        }
        ++current;
        
        Enum_AST *enum_ast = pool_alloc(Enum_AST, &ctx->ast_pool, 1);
        enum_ast->type = AST_Type::enum_ast;
        enum_ast->flags = 0;
        enum_ast->line_number = line_number;
        enum_ast->line_offset = line_offset;
        enum_ast->values.count = values.count;
        enum_ast->values.data = pool_alloc(Decl_AST*, &ctx->ast_pool, values.count);
        for(u64 i = 0; i < values.count; ++i)
        {
            enum_ast->values[i] = values[i];
        }
        result = enum_ast;
    }
    else if(current->type == Token_Type::key_struct)
    {
        u32 line_number = current->line_number;
        u32 line_offset = current->line_offset;
        
        ++current;
        Dynamic_Array<Decl_AST*> decls = {0};
        
        if(current->type != Token_Type::open_brace)
        {
            report_error(ctx, start_section, current, "Expected '{'");
            return nullptr;
        }
        ++current;
        
        u64 var_count = 0;
        u64 const_count = 0;
        
        while(current->type != Token_Type::eof && current->type != Token_Type::close_brace)
        {
            Decl_AST *decl = parse_decl(ctx, &current, Decl_Type::Struct);
            if(!decl)
            {
                return nullptr;
            }
            if(decl->flags & DECL_FLAG_CONSTANT)
            {
                ++const_count;
            }
            else
            {
                ++var_count;
            }
            array_add(&decls, decl);
        }
        
        if(current->type != Token_Type::close_brace)
        {
            report_error(ctx, start_section, current, "Expected '}'");
            return nullptr;
        }
        ++current;
        
        Struct_AST *struct_ast = pool_alloc(Struct_AST, &ctx->ast_pool, 1);
        struct_ast->type = AST_Type::struct_ast;
        struct_ast->flags = 0;
        struct_ast->line_number = line_number;
        struct_ast->line_offset = line_offset;
        
        struct_ast->constants.count = const_count;
        struct_ast->fields.count = var_count;
        struct_ast->constants.data = pool_alloc(Decl_AST*, &ctx->ast_pool, const_count);
        struct_ast->fields.data = pool_alloc(Decl_AST*, &ctx->ast_pool, var_count);
        
        u64 const_i = 0;
        u64 var_i = 0;
        
        for(u64 i = 0; i < decls.count; ++i)
        {
            Decl_AST *decl = decls[i];
            if(decl->flags & DECL_FLAG_CONSTANT)
            {
                struct_ast->constants[const_i++] = decl;
            }
            else
            {
                struct_ast->fields[var_i++] = decl;
            }
        }
        result = struct_ast;
    }
    else if(current->type == Token_Type::open_paren)
    {
        // This could be a parenthesized expression, a function type, or a function literal
        
        u32 line_number = current->line_number;
        u32 line_offset = current->line_offset;
        
        ++current;
        
        Dynamic_Array<Parameter_AST> parameters = {0};
        
        bool expect_more = (current->type != Token_Type::eof && current->type != Token_Type::close_paren);
        bool must_be_func = false;
        bool must_have_body = false;
        
        while(expect_more)
        {
            Ident_AST *ident = nullptr;
            AST *type = nullptr;
            AST *default_value = nullptr;
            
            AST *expr = parse_expr(ctx, &current);
            if(!expr)
            {
                return nullptr;
            }
            // expr could be parameter name, parameter type, or maybe an expr, depending on must_be_func
            
            if(expr->type == AST_Type::ident_ast)
            {
                // If there is a ':' or ':=' the identifier must be a parameter name
                // Otherwise, it could be a Type, or just a parenthesized expression
                
                bool expect_default = false;
                if(current->type == Token_Type::colon)
                {
                    ++current;
                    must_be_func = true;
                    
                    ident = static_cast<Ident_AST*>(expr);
                    type = parse_expr(ctx, &current);
                    
                    if(!type)
                    {
                        return nullptr;
                    }
                    if(current->type == Token_Type::equal)
                    {
                        ++current;
                        expect_default = true;
                    }
                }
                else if(current->type == Token_Type::colon_eq)
                {
                    ++current;
                    must_be_func = true;
                    
                    ident = static_cast<Ident_AST*>(expr);
                    // type needs to be inferred
                    expect_default = true;
                }
                else
                {
                    type = expr;
                }
                
                if(expect_default)
                {
                    must_have_body = true;
                    default_value = parse_expr(ctx, &current);
                    if(!default_value)
                    {
                        return nullptr;
                    }
                }
            }
            else
            {
                // Expr is not a parameter name, it could be unnamed parameter, or just a parethesized expression
                type = expr;
            }
            
            array_add(&parameters, {ident,type,default_value});
            
            if(current->type == Token_Type::comma)
            {
                ++current;
                must_be_func = true;
                expect_more = true;
            }
            else
            {
                expect_more = false;
            }
        }
        
        if(current->type != Token_Type::close_paren)
        {
            report_error(ctx, start_section, current, "Expected ')'");
            return nullptr;
        }
        
        ++current;
        
        if(current->type == Token_Type::arrow)
        {
            ++current;
        }
        else if(must_be_func)
        {
            report_error(ctx, start_section, current, "Expected '->'");
            return nullptr;
        }
        else
        {
            // It was just a parenthesized expression
            // TODO: fix memory leak
            assert(parameters.count == 1);
            assert(parameters[0].name == nullptr);
            assert(parameters[0].type != nullptr);
            assert(parameters[0].default_value == nullptr);
            
            return parameters[0].type;
        }
        
        // Now parse return type
        AST *return_type = parse_expr(ctx, &current);
        if(!return_type)
        {
            return nullptr;
        }
        
        Block_AST *block = nullptr;
        
        if(current->type == Token_Type::open_brace)
        {
            block = parse_statement_block(ctx, &current);
            if(!block)
            {
                return nullptr;
            }
        }
        else if(must_have_body)
        {
            report_error(ctx, start_section, current, "Expected '{'. Only function literals can have default types, not function types");
            return nullptr;
        }
        
        Function_Type_AST *func_type = pool_alloc(Function_Type_AST, &ctx->ast_pool, 1);
        func_type->type = AST_Type::function_type_ast;
        func_type->flags = 0;
        func_type->line_number = line_number;
        func_type->line_offset = line_offset;
        
        func_type->parameter_types.count = parameters.count;
        func_type->parameter_types.data = pool_alloc(AST*, &ctx->ast_pool, parameters.count);
        
        func_type->return_types.count = 1;
        func_type->return_types.data = pool_alloc(AST*, &ctx->ast_pool, 1);
        func_type->return_types[0] = return_type;
        
        
        if(block)
        {
            Function_AST *result_func = pool_alloc(Function_AST, &ctx->ast_pool, 1);
            result_func->type = AST_Type::function_ast;
            result_func->flags = 0;
            result_func->line_number = line_number;
            result_func->line_offset = line_offset;
            
            result_func->prototype = func_type;
            result_func->block = block;
            
            result_func->param_names.count = parameters.count;
            result_func->param_names.data = pool_alloc(Ident_AST*, &ctx->ast_pool, parameters.count);
            result_func->default_values.count = parameters.count;
            result_func->default_values.data = pool_alloc(AST*, &ctx->ast_pool, parameters.count);
            
            for(u64 i = 0; i < parameters.count; ++i)
            {
                func_type->parameter_types[i] = parameters[i].type;
                result_func->param_names[i] = parameters[i].name;
                result_func->default_values[i] = parameters[i].default_value;
            }
            
            result = result_func;
        }
        else
        {
            for(u64 i = 0; i < parameters.count; ++i)
            {
                func_type->parameter_types[i] = parameters[i].type;
            }
            result = func_type;
        }
    }
    else if(current->type == Token_Type::number)
    {
        Number_AST *result_number = pool_alloc(Number_AST, &ctx->ast_pool, 1);
        *result_number = make_number_ast(*current);
        result = result_number;
        ++current;
    }
    else if(current->type == Token_Type::key_void)
    {
        Primitive_AST *prim_ast = pool_alloc(Primitive_AST, &ctx->ast_pool, 1);
        prim_ast->type = AST_Type::primitive_ast;
        prim_ast->flags = 0;
        prim_ast->line_number = current->line_number;
        prim_ast->line_offset = current->line_offset;
        
        prim_ast->primitive = Primitive_Type::void_t;
        
        ++current;
        result = prim_ast;
    }
    else if(current->type == Token_Type::add ||
            current->type == Token_Type::sub ||
            current->type == Token_Type::mul ||
            current->type == Token_Type::ref)
    {
        u32 line_number = current->line_number;
        u32 line_offset = current->line_offset;
        Unary_Operator op;
        switch(current->type)
        {
            case Token_Type::add: {
                op = Unary_Operator::plus;
            } break;
            case Token_Type::sub: {
                op = Unary_Operator::minus;
            } break;
            case Token_Type::mul: {
                op = Unary_Operator::deref;
            } break;
            case Token_Type::ref: {
                op = Unary_Operator::ref;
            } break;
            default: {
                assert(false);
            } break;
        }
        
        ++current;
        AST *operand = parse_expr(ctx, &current);
        if(!operand)
        {
            return nullptr;
        }
        
        Unary_Operator_AST *unary_ast = pool_alloc(Unary_Operator_AST, &ctx->ast_pool, 1);
        
        unary_ast->type = AST_Type::unary_ast;
        unary_ast->flags = 0;
        unary_ast->line_number = line_number;
        unary_ast->line_offset = line_offset;
        unary_ast->op = op;
        unary_ast->operand = operand;
        
        result = unary_ast;
    }
    
    if(!result)
    {
        report_error(ctx, start_section, current, "Expected expression");
    }
    return result;
}

internal AST *parse_statement(Parsing_Context *ctx, Token **current_ptr)
{
    Token *current = *current_ptr;
    Token *start_section = current;
    AST *result = nullptr;
    defer {
        *current_ptr = current;
    };
    
    bool require_semicolon = false;
    
    u32 line_number = current->line_number;
    u32 line_offset = current->line_offset;
    
    if(current->type == Token_Type::key_for)
    {
        ++current;
        // TODO
        report_error(ctx, start_section, current, "for statements not yet implemented");
    }
    else if(current->type == Token_Type::key_if)
    {
        ++current;
        AST *expr = parse_expr(ctx, &current);
        if(!expr)
        {
            return nullptr;
        }
        Block_AST *then_block = parse_statement_block(ctx, &current);
        if(!then_block)
        {
            return nullptr;
        }
        
        Block_AST *else_block = nullptr;
        
        if(current->type == Token_Type::key_else)
        {
            ++current;
            else_block = parse_statement_block(ctx, &current);
            if(!else_block)
            {
                return nullptr;
            }
        }
        
        If_AST *if_ast = pool_alloc(If_AST, &ctx->ast_pool, 1);
        if_ast->type = AST_Type::if_ast;
        if_ast->flags = 0;
        if_ast->line_number = line_number;
        if_ast->line_offset = line_offset;
        
        if_ast->guard = expr;
        if_ast->then_block = then_block;
        if_ast->else_block = else_block;
        
        result = if_ast;
    }
    else if(current->type == Token_Type::key_while)
    {
        ++current;
        AST *expr = parse_expr(ctx, &current);
        if(!expr)
        {
            return nullptr;
        }
        Block_AST *body = parse_statement_block(ctx, &current);
        
        While_AST *while_ast = pool_alloc(While_AST, &ctx->ast_pool, 1);
        while_ast->type = AST_Type::while_ast;
        while_ast->flags = 0;
        while_ast->line_number = line_number;
        while_ast->line_offset = line_offset;
        
        while_ast->guard = expr;
        while_ast->body = body;
        
        result = while_ast;
    }
    else if(current->type == Token_Type::key_return)
    {
        require_semicolon = true;
        ++current;
        AST *expr = parse_expr(ctx, &current);
        if(!expr)
        {
            return nullptr;
        }
        
        Return_AST *return_ast = pool_alloc(Return_AST, &ctx->ast_pool, 1);
        return_ast->type = AST_Type::return_ast;
        return_ast->flags = 0;
        return_ast->line_number = line_number;
        return_ast->line_offset = line_offset;
        
        return_ast->expr = expr;
        
        result = return_ast;
    }
    else if(current->type == Token_Type::open_brace)
    {
        result = parse_statement_block(ctx, &current);
    }
    else
    {
        // Expect an expression, declaration, or operation (e.g. +=)
        
        if(current->type == Token_Type::ident)
        {
            Token *at_ident = current;
            ++current;
            if(current->type == Token_Type::colon ||
               current->type == Token_Type::colon_eq ||
               current->type == Token_Type::double_colon)
            {
                // Expect a declaration
                current = at_ident;
                Decl_AST *decl = parse_decl(ctx, &current, Decl_Type::Statement);
                if(!decl)
                {
                    return nullptr;
                }
                result = decl;
            }
            else if(current->type == Token_Type::equal ||
                    current->type == Token_Type::mul_eq ||
                    current->type == Token_Type::add_eq ||
                    current->type == Token_Type::sub_eq ||
                    current->type == Token_Type::div_eq)
            {
                Assign_Operator assign_type;
                switch(current->type)
                {
                    case Token_Type::equal: {
                        assign_type = Assign_Operator::equal;
                    } break;
                    case Token_Type::mul_eq: {
                        assign_type = Assign_Operator::mul_eq;
                    } break;
                    case Token_Type::add_eq: {
                        assign_type = Assign_Operator::add_eq;
                    } break;
                    case Token_Type::sub_eq: {
                        assign_type = Assign_Operator::sub_eq;
                    } break;
                    case Token_Type::div_eq: {
                        assign_type = Assign_Operator::div_eq;
                    } break;
                    default: {
                        assert(false);
                    } break;
                }
                
                ++current;
                AST *expr = parse_expr(ctx, &current);
                if(!expr)
                {
                    return nullptr;
                }
                require_semicolon = true;
                
                Assign_AST *assign = pool_alloc(Assign_AST, &ctx->ast_pool, 1);
                assign->type = AST_Type::assign_ast;
                assign->flags = 0;
                assign->line_number = line_number;
                assign->line_offset = line_offset;
                
                assign->ident = make_ident_ast(*at_ident);
                assign->assign_type = assign_type;
                assign->rhs = expr;
                
                result = assign;
            }
            else
            {
                // Expect an expression;
                current = at_ident;
                AST *expr = parse_expr(ctx, &current);
                if(!expr)
                {
                    return nullptr;
                }
                result = expr;
                require_semicolon = true;
            }
        }
        else
        {
            // Expect an expression;
            AST *expr = parse_expr(ctx, &current);
            if(!expr)
            {
                return nullptr;
            }
            result = expr;
            require_semicolon = true;
        }
    }
    
    if(require_semicolon)
    {
        if(current->type == Token_Type::semicolon)
        {
            ++current;
        }
        else
        {
            report_error(ctx, start_section, current, "Expected ';'");
        }
    }
    
    return result;
}

internal Block_AST *parse_statement_block(Parsing_Context *ctx, Token **current_ptr)
{
    Token *current = *current_ptr;
    Token *start_section = current;
    Block_AST *result = nullptr;
    defer {
        *current_ptr = current;
    };
    
    if(current->type == Token_Type::open_brace)
    {
        result = pool_alloc(Block_AST, &ctx->ast_pool, 1);
        
        result->type = AST_Type::block_ast;
        result->flags = 0;
        result->line_number = current->line_number;
        result->line_offset = current->line_offset;
        
        ++current;
        
        // TODO: memory leak
        Dynamic_Array<AST*> statements = {0};
        
        while(true)
        {
            if(current->type == Token_Type::close_brace)
            {
                ++current;
                
                result->statements.count = statements.count;
                result->statements.data = pool_alloc(AST*, &ctx->ast_pool, statements.count);
                
                for(u64 i = 0; i < statements.count; ++i)
                {
                    result->statements[i] = statements[i];
                }
                break;
            }
            else
            {
                AST *stmt = parse_statement(ctx, &current);
                if(stmt)
                {
                    array_add(&statements, stmt);
                }
                else
                {
                    return nullptr;
                }
            }
        }
    }
    else
    {
        report_error(ctx, start_section, current, "Expected '{' to begin statement block");
    }
    
    return result;
}

Decl_AST *parse_decl(Parsing_Context *ctx, Token **current_ptr, Decl_Type decl_type)
{
    Token *current = *current_ptr;
    Token *start_section = current;
    Decl_AST *result = nullptr;
    defer {
        *current_ptr = current;
    };
    
    u32 line_number = current->line_number;
    u32 line_offset = current->line_offset;
    
    if(current->type == Token_Type::ident)
    {
        Token *ident_tok = current;
        ++current;
        AST *type = nullptr;
        
        bool not_enum = (decl_type != Decl_Type::Enum);
        bool require_value = (decl_type == Decl_Type::Statement);
        bool expect_expr;
        bool is_constant;
        
        if(current->type == Token_Type::colon && not_enum)
        {
            ++current;
            type = parse_expr(ctx, &current);
            if(!type)
            {
                return nullptr;
            }
            
            if(current->type == Token_Type::equal)
            {
                ++current;
                expect_expr = true;
                is_constant = false;
            }
            else if(current->type == Token_Type::colon)
            {
                ++current;
                expect_expr = true;
                is_constant = true;
            }
            else if(require_value)
            {
                report_error(ctx, start_section, current, "Expected '=' or ':'");
                return nullptr;
            }
            else
            {
                expect_expr = false;
                is_constant = false;
            }
        }
        else if(current->type == Token_Type::colon_eq && not_enum)
        {
            ++current;
            expect_expr = true;
            is_constant = false;
        }
        else if(current->type == Token_Type::double_colon)
        {
            ++current;
            expect_expr = true;
            is_constant = true;
        }
        else if(not_enum)
        {
            report_error(ctx, start_section, current, "Expected ':', ':=', or '::'");
            return nullptr;
        }
        else
        {
            expect_expr = false;
            is_constant = true;
        }
        
        AST *expr = nullptr;
        
        if(expect_expr)
        {
            if(current->type == Token_Type::semicolon)
            {
                report_error(ctx, start_section, current, "Expected expression");
                return nullptr;
            }
            expr = parse_expr(ctx, &current);
            if(!expr)
            {
                return nullptr;
            }
        }
        
        // TODO: verify that the expression is constant if is_constant?
        
        if(current->type == Token_Type::semicolon)
        {
            ++current;
        }
        else
        {
            if(decl_type == Decl_Type::Enum)
            {
                report_error(ctx, start_section, current, "Expected ';' or '::'");
            }
            else
            {
                report_error(ctx, start_section, current, "Expected ';'");
            }
            return nullptr;
        }
        
        result = pool_alloc(Decl_AST, &ctx->ast_pool, 1);
        result->type = AST_Type::decl_ast;
        result->flags = 0;
        result->line_number = line_number;
        result->line_offset = line_offset;
        result->ident = make_ident_ast(*ident_tok);
        result->decl_type = type;
        result->expr = expr;
        
        if(is_constant)
        {
            result->flags |= DECL_FLAG_CONSTANT;
        }
    }
    else
    {
        report_error(ctx, start_section, current, "Expected identifier to begin declaration");
    }
    
    return result;
}


Dynamic_Array<Decl_AST*> parse_tokens(Parsing_Context *ctx)
{
    Dynamic_Array<Decl_AST*> result = {0};
    
    Token *current = ctx->tokens.data;
    Token *start_section = current;
    
    while(current->type != Token_Type::eof)
    {
        start_section = current;
        Decl_AST *decl = parse_decl(ctx, &current, Decl_Type::Statement);
        if(decl)
        {
            array_add(&result, decl);
        }
        else
        {
            while(true)
            {
                if(current->type == Token_Type::eof)
                {
                    break;
                }
                else if(current->type == Token_Type::semicolon)
                {
                    ++current;
                    break;
                }
                else
                {
                    ++current;
                }
            }
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
            if(decl_ast->expr)
            {
                print_dot_child(pb, decl_ast->expr, this_serial, serial);
            }
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
                if(type_ast->parameter_types[i])
                {
                    print_dot_child(pb, type_ast->parameter_types[i], this_serial, serial);
                }
            }
            for(u64 i = 0; i < type_ast->return_types.count; ++i)
            {
                if(type_ast->return_types[i])
                {
                    print_dot_child(pb, type_ast->return_types[i], this_serial, serial);
                }
            }
        } break;
        case AST_Type::function_ast: {
            Function_AST *function_ast = static_cast<Function_AST*>(ast);
            
            u64 this_serial = (*serial)++;
            print_buf(pb, "n%ld[label=\"Function\"];\n", this_serial);
            
            print_dot_child(pb, function_ast->prototype, this_serial, serial);
            for(u64 i = 0; i < function_ast->param_names.count; ++i)
            {
                if(function_ast->param_names[i])
                {
                    print_dot_child(pb, function_ast->param_names[i], this_serial, serial);
                }
            }
            for(u64 i = 0; i < function_ast->default_values.count; ++i)
            {
                if(function_ast->default_values[i])
                {
                    print_dot_child(pb, function_ast->default_values[i], this_serial, serial);
                }
            }
            print_dot_child(pb, function_ast->block, this_serial, serial);
        } break;
        case AST_Type::function_call_ast: {
            Function_Call_AST *call_ast = static_cast<Function_Call_AST*>(ast);
            
            u64 this_serial = (*serial)++;
            print_buf(pb, "n%ld[label=\"function call\"];\n", this_serial);
            print_dot_child(pb, call_ast->function, this_serial, serial);
            for(u64 i = 0; i < call_ast->args.count; ++i)
            {
                print_dot_child(pb, call_ast->args[i], this_serial, serial);
            }
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
        case AST_Type::while_ast: {
            While_AST *while_ast = static_cast<While_AST*>(ast);
            
            u64 this_serial = (*serial)++;
            
            print_buf(pb, "n%ld[label=\"while\"];\n", this_serial);
            print_dot_child(pb, while_ast->guard, this_serial, serial);
            print_dot_child(pb, while_ast->body, this_serial, serial);
        } break;
        // TODO: print for_ast
        case AST_Type::if_ast: {
            If_AST *if_ast = static_cast<If_AST*>(ast);
            
            u64 this_serial = (*serial)++;
            
            print_buf(pb, "n%ld[label=\"if\"];\n", this_serial);
            print_dot_child(pb, if_ast->guard, this_serial, serial);
            print_dot_child(pb, if_ast->then_block, this_serial, serial);
            if(if_ast->else_block)
            {
                print_dot_child(pb, if_ast->else_block, this_serial, serial);
            }
        } break;
        case AST_Type::struct_ast: {
            Struct_AST *struct_ast = static_cast<Struct_AST*>(ast);
            
            u64 this_serial = (*serial)++;
            
            print_buf(pb, "n%ld[label=\"struct\"];\n", this_serial);
            for(u64 i = 0; i < struct_ast->constants.count; ++i)
            {
                print_dot_child(pb, struct_ast->constants[i], this_serial, serial);
            }
            for(u64 i = 0; i < struct_ast->fields.count; ++i)
            {
                print_dot_child(pb, struct_ast->fields[i], this_serial, serial);
            }
        } break;
        case AST_Type::enum_ast: {
            Enum_AST *enum_ast = static_cast<Enum_AST*>(ast);
            
            u64 this_serial = (*serial)++;
            
            print_buf(pb, "n%ld[label=\"enum\"];\n", this_serial);
            for(u64 i = 0; i < enum_ast->values.count; ++i)
            {
                print_dot_child(pb, enum_ast->values[i], this_serial, serial);
            }
        } break;
        case AST_Type::assign_ast: {
            Assign_AST *assign_ast = static_cast<Assign_AST*>(ast);
            
            u64 this_serial = (*serial)++;
            
            print_buf(pb, "n%ld[label=\"%s\"];\n", this_serial, assign_names[(u64)assign_ast->assign_type]);
            print_dot_child(pb, &assign_ast->ident, this_serial, serial);
            print_dot_child(pb, assign_ast->rhs, this_serial, serial);
        } break;
        case AST_Type::unary_ast: {
            Unary_Operator_AST *unary_ast = static_cast<Unary_Operator_AST*>(ast);
            
            u64 this_serial = (*serial)++;
            
            print_buf(pb, "n%ld[label=\"%s\"];\n", this_serial, unary_operator_names[(u64)unary_ast->op]);
            print_dot_child(pb, unary_ast->operand, this_serial, serial);
        } break;
        case AST_Type::return_ast: {
            Return_AST *return_ast = static_cast<Return_AST*>(ast);
            
            u64 this_serial = (*serial)++;
            
            print_buf(pb, "n%ld[label=\"return\"];\n", this_serial);
            print_dot_child(pb, return_ast->expr, this_serial, serial);
        } break;
        case AST_Type::primitive_ast: {
            Primitive_AST *primitive_ast = static_cast<Primitive_AST*>(ast);
            
            u64 this_serial = (*serial)++;
            
            print_buf(pb, "n%ld[label=\"%s\"];\n", this_serial, primitive_names[(u64)primitive_ast->primitive]);
        } break;
        default: {
            print_err("Unknown AST type in dot printer\n");
        } break;
    }
}

void print_dot(Print_Buffer *pb, Array<Decl_AST*> decls)
{
    print_buf(pb, "digraph decls {\n");
    
    u64 s = 0;
    for(u64 i = 0; i < decls.count; ++i)
    {
        print_dot_rec(pb, decls[i], &s);
    }
    
    print_buf(pb, "}\n");
    flush_buffer(pb);
}
