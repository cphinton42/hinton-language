
void init_parsing_context(Parsing_Context *ctx, String program_text, Array<Token> tokens, Pool_Allocator *ast_pool)
{
    ctx->program_text = program_text;
    ctx->tokens = tokens;
    ctx->ast_pool = ast_pool;
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
    result.s = next_serial++;
    result.line_number = ident.line_number;
    result.line_offset = ident.line_offset;
    result.resolved_type = nullptr;
    result.canonical_form = nullptr;
    result.ident = ident.contents;
    result.scope_index = 0;
    result.scope = nullptr;
    return result;
}

internal Number_AST make_number_ast(Token number)
{
    Number_AST result;
    result.type = AST_Type::number_ast;
    result.flags = 0;
    result.s = next_serial++;
    result.line_number = number.line_number;
    result.line_offset = number.line_offset;
    result.literal = number.contents;
    result.resolved_type = nullptr;
    result.canonical_form = nullptr;
    String literal = result.literal;
    u64 i_value = 0;
    
    // TODO: cleanup: better precision
    
    u64 i = 0;
    while(i < literal.count && '0' <= literal[i] && literal[i] <= '9')
    {
        i_value *= 10;
        i_value += literal[i] - '0';
        ++i;
    }
    
    if(i < literal.count)
    {
        assert(literal[i] == '.');
        ++i;
        
        f64 factor = 1.0;
        u64 fractional = 0;
        
        while(i < literal.count && '0' <= literal[i] && literal[i] <= '9')
        {
            fractional *= 10;
            fractional += literal[i] - '0';
            factor *= 0.1;
            ++i;
        }
        
        if(i < literal.count)
        {
            assert(literal[i] == 'e' || literal[i] == 'E');
            ++i;
            assert(i < literal.count);
            bool exponent_positive = true;
            if(literal[i] == '+')
            {
                exponent_positive = true;
            }
            else if(literal[i] == '-')
            {
                exponent_positive = false;
            }
            
            u64 exponent = 0;
            while(i < literal.count && '0' <= literal[i] && literal[i] <= '9')
            {
                exponent *= 10;
                exponent += literal[i] - '0';
                ++i;
            }
            
            if(exponent_positive)
            {
                for(u64 j = 0; j < exponent; ++j)
                {
                    factor *= 10.0;
                }
            }
            else
            {
                for(u64 j = 0; j < exponent; ++j)
                {
                    factor *= 0.1;
                }
            }
        }
        
        result.float_value = ((f64)i_value) + ((f64)fractional * factor);
        result.flags |= NUMBER_FLAG_FLOATLIKE;
    }
    else
    {
        result.int_value = i_value;
    }
    
    return result;
}

/*
internal void set_parent_ast(AST *root, AST *parent_ast)
{
    switch(root->type)
    {
        case AST_Type::refer_ident_ast: {
            Refer_Ident_AST *ident_ast = static_cast<Refer_Ident_AST*>(root);
            ident_ast->parent.ast = parent_ast;
        } break;
        case AST_Type::decl_ast: {
            Decl_AST *decl_ast = static_cast<Decl_AST*>(root);
            decl_ast->parent.ast = parent_ast;
        } break;
        case AST_Type::block_ast: {
            Block_AST *block_ast = static_cast<Block_AST*>(root);
            block_ast->parent.ast = parent_ast;
        } break;
        case AST_Type::function_type_ast: {
            Function_Type_AST *function_type_ast = static_cast<Function_Type_AST*>(root);
            for(u64 i = 0; i < function_type_ast->parameter_types.count; ++i)
            {
                if(function_type_ast->parameter_types[i])
                {
                    set_parent_ast(function_type_ast->parameter_types[i], parent_ast);
                }
            }
            for(u64 i = 0; i < function_type_ast->return_types.count; ++i)
            {
                set_parent_ast(function_type_ast->return_types[i], parent_ast);
            }
        } break;
        case AST_Type::function_ast: {
            Function_AST *function_ast = static_cast<Function_AST*>(root);
            function_ast->parent.ast = parent_ast;
            set_parent_ast(function_ast->prototype, parent_ast);
        } break;
        case AST_Type::function_call_ast: {
            Function_Call_AST *function_call_ast = static_cast<Function_Call_AST*>(root);
            for(u64 i = 0; i < function_call_ast->args.count; ++i)
            {
                set_parent_ast(function_call_ast->args[i], parent_ast);
            }
            set_parent_ast(function_call_ast->function, parent_ast);
        } break;
        case AST_Type::access_ast: {
            Access_AST *access_ast = static_cast<Access_AST*>(root);
            set_parent_ast(access_ast->lhs, parent_ast);
        } break;
        case AST_Type::binary_operator_ast: {
            Binary_Operator_AST *binary_operator_ast = static_cast<Binary_Operator_AST*>(root);
            set_parent_ast(binary_operator_ast->lhs, parent_ast);
            set_parent_ast(binary_operator_ast->rhs, parent_ast);
        } break;
        case AST_Type::while_ast: {
            While_AST *while_ast = static_cast<While_AST*>(root);
            while_ast->body->parent.ast = parent_ast;
            set_parent_ast(while_ast->guard, parent_ast);
        } break;
        case AST_Type::for_ast: {
            For_AST *for_ast = static_cast<For_AST*>(root);
            for_ast->parent.ast = parent_ast;
            if(for_ast->flags & FOR_FLAG_OVER_ARRAY)
            {
                set_parent_ast(for_ast->array_expr, parent_ast);
            }
            else
            {
                set_parent_ast(for_ast->low_expr, parent_ast);
                set_parent_ast(for_ast->high_expr, parent_ast);
            }
        } break;
        case AST_Type::if_ast: {
            If_AST *if_ast = static_cast<If_AST*>(root);
            if_ast->then_block->parent.ast = parent_ast;
            if(if_ast->else_block)
            {
                if_ast->else_block->parent.ast = parent_ast;
            }
            set_parent_ast(if_ast->guard, parent_ast);
        } break;
        case AST_Type::enum_ast: {
            Enum_AST *enum_ast = static_cast<Enum_AST*>(root);
            for(u64 i = 0; i < enum_ast->values.count; ++i)
            {
                set_parent_ast(enum_ast->values[i], parent_ast);
            }
        } break;
        case AST_Type::struct_ast: {
            Struct_AST *struct_ast = static_cast<Struct_AST*>(root);
            for(u64 i = 0; i < struct_ast->constants.count; ++i)
            {
                set_parent_ast(struct_ast->constants[i], parent_ast);
            }
            for(u64 i = 0; i < struct_ast->fields.count; ++i)
            {
                set_parent_ast(struct_ast->fields[i], parent_ast);
            }
        } break;
        case AST_Type::assign_ast: {
            Assign_AST *assign_ast = static_cast<Assign_AST*>(root);
            set_parent_ast(assign_ast->lhs, parent_ast);
            set_parent_ast(assign_ast->rhs, parent_ast);
        } break;
        case AST_Type::unary_ast: {
            Unary_Operator_AST *unary_operator_ast = static_cast<Unary_Operator_AST*>(root);
            set_parent_ast(unary_operator_ast->operand, parent_ast);
        } break;
        case AST_Type::return_ast: {
            Return_AST *return_ast = static_cast<Return_AST*>(root);
            set_parent_ast(return_ast->expr, parent_ast);
        } break;
        case AST_Type::def_ident_ast:
        case AST_Type::number_ast:
        case AST_Type::primitive_ast:
        case AST_Type::string_ast:
        case AST_Type::bool_ast: {
        } break;
    }
}
*/

enum class Decl_Type
{
    Statement,
    Struct,
    Enum,
};

internal Decl_AST *parse_decl(Parsing_Context *ctx, Token **current_ptr, Decl_Type type);
internal AST *parse_statement(Parsing_Context *ctx, Token **current_ptr);
internal Block_AST *parse_statement_block(Parsing_Context *ctx, Token **current_ptr);
internal Expr_AST *parse_base_expr(Parsing_Context *ctx, Token **current_ptr, u32 precedence);

internal Expr_AST* parse_expr(Parsing_Context *ctx, Token **current_ptr, u32 precedence = 7)
{
    Token *current = *current_ptr;
    Token *start_section = current;
    defer {
        *current_ptr = current;
    };
    
    Expr_AST *lhs = parse_base_expr(ctx, &current, precedence);
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
            case 7:
            if(current->type == Token_Type::lor)
            {
                op = Binary_Operator::lor;
                found_op = true;
                rhs_precedence = 6;
                break;
            }
            case 6:
            if(current->type == Token_Type::land)
            {
                op = Binary_Operator::land;
                found_op = true;
                rhs_precedence = 5;
                break;
            }
            case 5:
            if(current->type == Token_Type::double_equal)
            {
                op = Binary_Operator::cmp_eq;
                found_op = true;
                rhs_precedence = 4;
                break;
            }
            if(current->type == Token_Type::not_equal)
            {
                op = Binary_Operator::cmp_neq;
                found_op = true;
                rhs_precedence = 4;
                break;
            }
            case 4:
            if(current->type == Token_Type::lt)
            {
                op = Binary_Operator::cmp_lt;
                found_op = true;
                rhs_precedence = 3;
                break;
            }
            if(current->type == Token_Type::le)
            {
                op = Binary_Operator::cmp_le;
                found_op = true;
                rhs_precedence = 3;
                break;
            }
            if(current->type == Token_Type::gt)
            {
                op = Binary_Operator::cmp_gt;
                found_op = true;
                rhs_precedence = 3;
                break;
            }
            if(current->type == Token_Type::ge)
            {
                op = Binary_Operator::cmp_ge;
                found_op = true;
                rhs_precedence = 3;
                break;
            }
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
                ++current;
                
                Dynamic_Array<Expr_AST*> args = {0};
                
                if(current->type != Token_Type::eof && current->type != Token_Type::close_paren)
                {
                    Expr_AST *first_expr = parse_expr(ctx, &current);
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
                    
                    Expr_AST *expr = parse_expr(ctx, &current);
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
                
                Function_Call_AST *call_ast = construct_ast(ctx->ast_pool, Function_Call_AST, lhs->line_number, lhs->line_offset);
                
                call_ast->resolved_type = nullptr;
                call_ast->canonical_form = nullptr;
                call_ast->function = lhs;
                call_ast->args.count = args.count;
                call_ast->args.data = pool_alloc(Expr_AST*, ctx->ast_pool, args.count);
                
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
                Expr_AST *rhs = parse_expr(ctx, &current);
                if(rhs)
                {
                    if(current->type == Token_Type::close_sqr)
                    {
                        ++current;
                        
                        Binary_Operator_AST *result = construct_ast(ctx->ast_pool, Binary_Operator_AST, lhs->line_number, lhs->line_offset);
                        result->resolved_type = nullptr;
                        result->canonical_form = nullptr;
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
                ++current;
                if(current->type == Token_Type::ident)
                {
                    Access_AST *result = construct_ast(ctx->ast_pool, Access_AST, lhs->line_number, lhs->line_offset);
                    result->resolved_type = nullptr;
                    result->canonical_form = nullptr;
                    result->lhs = lhs;
                    result->ident = current->contents;
                    ++current;
                    
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
            Expr_AST *rhs = parse_expr(ctx, &current, rhs_precedence);
            if(rhs)
            {
                Binary_Operator_AST *result = construct_ast(ctx->ast_pool, Binary_Operator_AST, lhs->line_number, lhs->line_offset);
                result->resolved_type = nullptr;
                result->canonical_form = nullptr;
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


internal Expr_AST *parse_base_expr(Parsing_Context *ctx, Token **current_ptr, u32 precedence)
{
    Token *current = *current_ptr;
    Token *start_section = current;
    Expr_AST *result = nullptr;
    defer {
        *current_ptr = current;
    };
    
    switch(current->type)
    {
        case Token_Type::ident: {
            Ident_AST *result_ident = pool_alloc(Ident_AST, ctx->ast_pool, 1);
            *result_ident = make_ident_ast(*current);
            result = result_ident;
            ++current;
        } break;
        case Token_Type::key_enum: {
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
            
            Enum_AST *enum_ast = construct_ast(ctx->ast_pool, Enum_AST, line_number, line_offset);
            enum_ast->resolved_type = nullptr; // TODO: could just make this Type
            enum_ast->canonical_form = nullptr;
            enum_ast->values.count = values.count;
            enum_ast->values.data = pool_alloc(Decl_AST*, ctx->ast_pool, values.count);
            for(u64 i = 0; i < values.count; ++i)
            {
                enum_ast->values[i] = values[i];
            }
            result = enum_ast;
        } break;
        case Token_Type::key_struct: {
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
            
            Struct_AST *struct_ast = construct_ast(ctx->ast_pool, Struct_AST, line_number, line_offset);
            
            struct_ast->resolved_type = nullptr;
            struct_ast->canonical_form = nullptr;
            struct_ast->constants.count = const_count;
            struct_ast->fields.count = var_count;
            struct_ast->constants.data = pool_alloc(Decl_AST*, ctx->ast_pool, const_count);
            struct_ast->fields.data = pool_alloc(Decl_AST*, ctx->ast_pool, var_count);
            
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
        } break;
        case Token_Type::open_paren: {
            // This could be a parenthesized expression, a function type, or a function literal
            
            u32 line_number = current->line_number;
            u32 line_offset = current->line_offset;
            
            ++current;
            
            Dynamic_Array<Parameter_AST> parameters = {0};
            
            bool expect_more = (current->type != Token_Type::eof && current->type != Token_Type::close_paren);
            bool must_be_func = current->type == Token_Type::close_paren;
            bool must_have_body = false;
            
            while(expect_more)
            {
                Ident_AST *ident = nullptr;
                Expr_AST *type = nullptr;
                Expr_AST *default_value = nullptr;
                
                Token *previous = current;
                bool expect_default = false;
                
                if(current->type == Token_Type::ident)
                {
                    ++current;
                    bool got_param_name;
                    
                    if(current->type == Token_Type::colon)
                    {
                        got_param_name = true;
                        expect_default = false;
                        must_be_func = true;
                        ++current;
                    }
                    else if(current->type == Token_Type::colon_eq)
                    {
                        got_param_name = true;
                        expect_default = true;
                        must_be_func = true;
                        must_have_body = true;
                        ++current;
                    }
                    else
                    {
                        got_param_name = false;
                        current = previous;
                    }
                    
                    if(got_param_name)
                    {
                        ident = pool_alloc(Ident_AST, ctx->ast_pool, 1);
                        *ident = make_ident_ast(*previous);
                    }
                }
                
                Expr_AST *expr = parse_expr(ctx, &current);
                if(!expr)
                {
                    return nullptr;
                }
                if(expect_default)
                {
                    default_value = expr;
                }
                else
                {
                    type = expr;
                }
                // TODO: explicit type and default value at the same time
                
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
                
                Expr_AST *result = parameters[0].type;
                return result;
            }
            
            // Now parse return type
            Expr_AST *return_type = parse_expr(ctx, &current);
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
            
            
            Function_Type_AST *func_type = construct_ast(ctx->ast_pool, Function_Type_AST, line_number, line_offset);
            
            func_type->resolved_type = nullptr;
            func_type->canonical_form = nullptr;
            func_type->parameter_types.count = parameters.count;
            func_type->parameter_types.data = pool_alloc(Expr_AST*, ctx->ast_pool, parameters.count);
            
            func_type->return_types.count = 1;
            func_type->return_types.data = pool_alloc(Expr_AST*, ctx->ast_pool, 1);
            func_type->return_types[0] = return_type;
            
            if(block)
            {
                Function_AST *result_func = construct_ast(ctx->ast_pool, Function_AST, line_number, line_offset);
                
                result_func->resolved_type = nullptr;
                result_func->canonical_form = nullptr;
                result_func->prototype = func_type;
                result_func->block = block;
                
                result_func->param_names.count = parameters.count;
                result_func->param_names.data = pool_alloc(Ident_AST*, ctx->ast_pool, parameters.count);
                result_func->default_values.count = parameters.count;
                result_func->default_values.data = pool_alloc(Expr_AST*, ctx->ast_pool, parameters.count);
                
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
        } break;
        case Token_Type::number: {
            Number_AST *result_number = pool_alloc(Number_AST, ctx->ast_pool, 1);
            *result_number = make_number_ast(*current);
            result = result_number;
            ++current;
        } break;
        case Token_Type::string: {
            String_AST *result_string = construct_ast(ctx->ast_pool, String_AST, current->line_number, current->line_offset);
            
            result_string->resolved_type = nullptr;
            result_string->canonical_form = nullptr;
            result_string->literal = current->contents;
            
            String literal = result_string->literal;
            u64 new_size = 0;
            bool needs_interpretation = false;
            for(u64 i = 0; i < literal.count; ++i)
            {
                if(literal[i] == '\\')
                {
                    ++i;
                    needs_interpretation = true;
                }
                ++new_size;
            }
            if(needs_interpretation)
            {
                String value;
                value.count = new_size;
                value.data = pool_alloc(byte, ctx->ast_pool, new_size);
                
                u64 value_i = 0;
                for(u64 i = 0; i < literal.count; ++i)
                {
                    if(literal[i] == '\\')
                    {
                        ++i;
                        if(literal[i] == 'n')
                        {
                            value[value_i++] = '\n';
                        }
                        else if(literal[i] == 't')
                        {
                            value[value_i++] = '\t';
                        }
                        else
                        {
                            value[value_i++] = literal[i];
                        }
                    }
                    else
                    {
                        value[value_i++] = literal[i];
                    }
                }
            }
            else
            {
                result_string->value = result_string->literal;
            }
            
            result = result_string;
            ++current;
        } break;
        
        {
            u64 primitive;
            
            case Token_Type::key_bool: {
                primitive = PRIM_BOOL;
                goto make_primitive_ast;
            } break;
            case Token_Type::key_bool8: {
                primitive = PRIM_BOOL8;
                goto make_primitive_ast;
            } break;
            case Token_Type::key_bool16: {
                primitive = PRIM_BOOL16;
                goto make_primitive_ast;
            } break;
            case Token_Type::key_bool32: {
                primitive = PRIM_BOOL32;
                goto make_primitive_ast;
            } break;
            case Token_Type::key_bool64: {
                primitive = PRIM_BOOL64;
                goto make_primitive_ast;
            } break;
            case Token_Type::key_s8: {
                primitive = PRIM_S8;
                goto make_primitive_ast;
            } break;
            case Token_Type::key_s16: {
                primitive = PRIM_S16;
                goto make_primitive_ast;
            } break;
            case Token_Type::key_s32: {
                primitive = PRIM_S32;
                goto make_primitive_ast;
            } break;
            case Token_Type::key_int:
            case Token_Type::key_s64: {
                primitive = PRIM_S64;
                goto make_primitive_ast;
            } break;
            case Token_Type::key_type: {
                primitive = PRIM_TYPE;
                goto make_primitive_ast;
            } break;
            case Token_Type::key_u8: {
                primitive = PRIM_U8;
                goto make_primitive_ast;
            } break;
            case Token_Type::key_u16: {
                primitive = PRIM_U16;
                goto make_primitive_ast;
            } break;
            case Token_Type::key_u32: {
                primitive = PRIM_U32;
                goto make_primitive_ast;
            } break;
            case Token_Type::key_uint:
            case Token_Type::key_u64: {
                primitive = PRIM_U64;
                goto make_primitive_ast;
            } break;
            case Token_Type::key_f32: {
                primitive = PRIM_F32;
                goto make_primitive_ast;
            } break;
            case Token_Type::key_f64: {
                primitive = PRIM_F64;
                goto make_primitive_ast;
            } break;
            case Token_Type::key_void: {
                primitive = PRIM_VOID;
                goto make_primitive_ast;
            } break;
            
            make_primitive_ast:
            
            Primitive_AST *primitive_ast = construct_ast(ctx->ast_pool, Primitive_AST, current->line_number, current->line_offset);
            primitive_ast->resolved_type = &type_t_ast;
            primitive_ast->canonical_form = nullptr;
            primitive_ast->primitive = primitive;
            ++current;
            return primitive_ast;
        }
        
        {
            bool bool_value;
            
            case Token_Type::key_true: {
                bool_value = true;
                goto make_bool_ast;
            }
            case Token_Type::key_false: {
                bool_value = false;
                goto make_bool_ast;
            }
            make_bool_ast:
            
            Bool_AST *bool_ast = construct_ast(ctx->ast_pool, Bool_AST, current->line_number, current->line_offset);
            bool_ast->resolved_type = nullptr;
            bool_ast->canonical_form = nullptr;
            bool_ast->value = bool_value;
            ++current;
            result = bool_ast;
        } break;
        
        {
            Unary_Operator unary_op;
            
            case Token_Type::add: {
                unary_op = Unary_Operator::plus;
                goto make_unary_operator_ast;
            }
            case Token_Type::sub: {
                unary_op = Unary_Operator::minus;
                goto make_unary_operator_ast;
            }
            case Token_Type::mul: {
                unary_op = Unary_Operator::deref;
                goto make_unary_operator_ast;
            }
            case Token_Type::ref: {
                unary_op = Unary_Operator::ref;
                goto make_unary_operator_ast;
            }
            case Token_Type::lnot: {
                unary_op = Unary_Operator::lnot;
                goto make_unary_operator_ast;
            }
            make_unary_operator_ast:
            
            u32 line_number = current->line_number;
            u32 line_offset = current->line_offset;
            
            ++current;
            Expr_AST *operand = parse_expr(ctx, &current, precedence);
            if(!operand)
            {
                return nullptr;
            }
            
            Unary_Operator_AST *unary_ast = construct_ast(ctx->ast_pool, Unary_Operator_AST, line_number, line_offset);
            unary_ast->resolved_type = nullptr;
            unary_ast->canonical_form = nullptr;
            unary_ast->op = unary_op;
            unary_ast->operand = operand;
            
            result = unary_ast;
        } break;
        
        default: {} break;
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
        Token *start = current;
        
        Ident_AST *induction_var = nullptr;
        Ident_AST *index_var = nullptr;
        bool by_pointer = false;
        
        if(current->type == Token_Type::ref)
        {
            // expr or named identifier by pointer
            by_pointer = true;
            ++current;
        }
        if(current->type == Token_Type::ident)
        {
            bool must_use_names = false;
            Token *first_ident = current;
            Token *second_ident = nullptr;
            ++current;
            
            if(current->type == Token_Type::comma)
            {
                must_use_names = true;
                ++current;
                if(current->type == Token_Type::ident)
                {
                    second_ident = current;
                    ++current;
                }
                else
                {
                    report_error(ctx, start_section, current, "Expected identifier to name index variable");
                    return nullptr;
                }
            }
            
            if(current->type == Token_Type::colon)
            {
                ++current;
                
                assert(first_ident);
                
                // TODO: move allocation to prevent memory leak
                induction_var = pool_alloc(Ident_AST, ctx->ast_pool, 1);
                *induction_var = make_ident_ast(*first_ident);
                
                if(second_ident)
                {
                    index_var = pool_alloc(Ident_AST, ctx->ast_pool, 1);
                    *index_var = make_ident_ast(*second_ident);
                }
            }
            else if(must_use_names)
            {
                report_error(ctx, start_section, current, "Expected ':' to separate variable names from range expression");
                return nullptr;
            }
            else
            {
                // Not names, just a range expr.
                current = start;
            }
        }
        
        Expr_AST *low_range_expr = nullptr;
        Expr_AST *high_range_expr = nullptr;
        
        low_range_expr = parse_expr(ctx, &current);
        if(!low_range_expr)
        {
            return nullptr;
        }
        if(current->type == Token_Type::double_dot)
        {
            if(index_var)
            {
                report_error(ctx, start_section, current, "There is only one variable to be named when iterating over a numerical range");
                return nullptr;
            }
            if(by_pointer)
            {
                report_error(ctx, start_section, current, "Cannot iterate over numerical range by pointer");
                return nullptr;
            }
            
            ++current;
            high_range_expr = parse_expr(ctx, &current);
            if(!high_range_expr)
            {
                return nullptr;
            }
        }
        
        Block_AST *body = parse_statement_block(ctx, &current);
        if(!body)
        {
            return nullptr;
        }
        
        For_AST *for_ast = construct_ast(ctx->ast_pool, For_AST, line_number, line_offset);
        if(by_pointer)
        {
            for_ast->flags |= FOR_FLAG_BY_POINTER;
        }
        
        if(!induction_var)
        {
            induction_var = construct_ast(ctx->ast_pool, Ident_AST, 0, 0);
            induction_var->flags |= AST_FLAG_SYNTHETIC;
            induction_var->ident = str_lit("it");
            induction_var->scope_index = 0;
            induction_var->scope = nullptr;
        }
        
        for_ast->induction_var = induction_var;
        for_ast->body = body;
        if(high_range_expr)
        {
            for_ast->low_expr = low_range_expr;
            for_ast->high_expr = high_range_expr;
        }
        else
        {
            if(!index_var)
            {
                index_var = construct_ast(ctx->ast_pool, Ident_AST, 0, 0);
                index_var->flags |= AST_FLAG_SYNTHETIC;
                index_var->ident = str_lit("it_index");
                index_var->scope_index = 0;
                index_var->scope = nullptr;
            }
            
            for_ast->flags |= FOR_FLAG_OVER_ARRAY;
            for_ast->index_var = index_var;
            for_ast->array_expr = low_range_expr;
        }
        
        result = for_ast;
    }
    else if(current->type == Token_Type::key_if)
    {
        ++current;
        Expr_AST *expr = parse_expr(ctx, &current);
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
        
        If_AST *if_ast = construct_ast(ctx->ast_pool, If_AST, line_number, line_offset);
        if_ast->guard = expr;
        if_ast->then_block = then_block;
        if_ast->else_block = else_block;
        
        result = if_ast;
    }
    else if(current->type == Token_Type::key_while)
    {
        ++current;
        Expr_AST *expr = parse_expr(ctx, &current);
        if(!expr)
        {
            return nullptr;
        }
        Block_AST *body = parse_statement_block(ctx, &current);
        
        While_AST *while_ast = construct_ast(ctx->ast_pool, While_AST, line_number, line_offset);
        while_ast->guard = expr;
        while_ast->body = body;
        
        result = while_ast;
    }
    else if(current->type == Token_Type::key_return)
    {
        require_semicolon = true;
        ++current;
        Expr_AST *expr = parse_expr(ctx, &current);
        if(!expr)
        {
            return nullptr;
        }
        
        Return_AST *return_ast = construct_ast(ctx->ast_pool, Return_AST, line_number, line_offset);
        return_ast->function = nullptr;
        return_ast->expr = expr;
        
        result = return_ast;
    }
    else if(current->type == Token_Type::open_brace)
    {
        result = parse_statement_block(ctx, &current);
    }
    else
    {
        // Expect an expression, declaration, or assignment (e.g. =,+=)
        
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
            else
            {
                // Expect an expression or assignment
                current = at_ident;
                
                Expr_AST *expr = parse_expr(ctx, &current);
                if(!expr)
                {
                    return nullptr;
                }
                
                if(current->type == Token_Type::equal ||
                   current->type == Token_Type::mul_eq ||
                   current->type == Token_Type::add_eq ||
                   current->type == Token_Type::sub_eq ||
                   current->type == Token_Type::div_eq)
                {
                    // expect an assignment, 'expr' should be the l-value (checked later)
                    
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
                            // The guard above prevents these
                            assert(false);
                        } break;
                    }
                    
                    ++current;
                    Expr_AST *rhs = parse_expr(ctx, &current);
                    if(!rhs)
                    {
                        return nullptr;
                    }
                    
                    Assign_AST *assign = construct_ast(ctx->ast_pool, Assign_AST, line_number, line_offset);
                    
                    assign->assign_type = assign_type;
                    assign->lhs = expr;
                    assign->rhs = rhs;
                    
                    result = assign;
                }
                else
                {
                    result = expr;
                }
                
                require_semicolon = true;
            }
        }
        else
        {
            // Expect an expression;
            Expr_AST *expr = parse_expr(ctx, &current);
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
        result = construct_ast(ctx->ast_pool, Block_AST, current->line_number, current->line_offset);
        ++current;
        
        // TODO: memory leak
        Dynamic_Array<AST*> statements = {0};
        
        while(true)
        {
            if(current->type == Token_Type::close_brace)
            {
                ++current;
                
                result->statements.count = statements.count;
                result->statements.data = pool_alloc(AST*, ctx->ast_pool, statements.count);
                
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
        Expr_AST *type = nullptr;
        
        bool not_enum = (decl_type != Decl_Type::Enum);
        // TODO: remove require_value, variables should be able to take on default values (e.g. zero initialized)
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
        
        Expr_AST *expr = nullptr;
        
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
                // TODO: this error is probably a bit off
                // sometimes :: was already seen ?
                report_error(ctx, start_section, current, "Expected ';' or '::'");
            }
            else
            {
                report_error(ctx, start_section, current, "Expected ';'");
            }
            return nullptr;
        }
        
        result = construct_ast(ctx->ast_pool, Decl_AST, line_number, line_offset);
        result->ident = make_ident_ast(*ident_tok);
        result->decl_type = type;
        result->expr = expr;
        
        if(is_constant)
        {
            // TODO: is this the place to do this?
            result->flags |= DECL_FLAG_CONSTANT;
            result->ident.flags |= EXPR_FLAG_CONSTANT;
            
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
