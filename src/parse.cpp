
u32 next_serial = 0;

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
    result.s = next_serial++;
    result.line_number = ident.line_number;
    result.line_offset = ident.line_offset;
    result.ident = ident.contents;
    result.referred_to = nullptr;
    result.parent = {0};
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

internal AST* construct_ast_(AST *new_ast, AST_Type type, u64 line_number, u64 line_offset)
{
    new_ast->type = type;
    new_ast->flags = 0;
    new_ast->s = next_serial++;
    new_ast->line_number = line_number;
    new_ast->line_offset = line_offset;
    return new_ast;
}

#define construct_ast(pool, type, line_number, line_offset) \
(static_cast<type*>(construct_ast_(pool_alloc(type,(pool),1),type::type_value, (line_number), (line_offset))))


internal void set_parent_ast(AST *root, AST *parent_ast)
{
    switch(root->type)
    {
        case AST_Type::ident_ast: {
            Ident_AST *ident_ast = static_cast<Ident_AST*>(root);
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
            assign_ast->ident.parent.ast = parent_ast;
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

enum class Decl_Type
{
    Statement,
    Struct,
    Enum,
};

internal Decl_AST *parse_decl(Parsing_Context *ctx, Token **current_ptr, Parent_Scope parent, Decl_Type type);
internal AST *parse_statement(Parsing_Context *ctx, Token **current_ptr, Parent_Scope parent);
internal Block_AST *parse_statement_block(Parsing_Context *ctx, Token **current_ptr, Parent_Scope parent);
internal AST *parse_base_expr(Parsing_Context *ctx, Token **current_ptr, Parent_Scope parent, u32 precedence);

internal AST* parse_expr(Parsing_Context *ctx, Token **current_ptr, Parent_Scope parent, u32 precedence = 5)
{
    Token *current = *current_ptr;
    Token *start_section = current;
    defer {
        *current_ptr = current;
    };
    
    AST *lhs = parse_base_expr(ctx, &current, parent, precedence);
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
            case 5:
            if(current->type == Token_Type::lor)
            {
                op = Binary_Operator::lor;
                found_op = true;
                rhs_precedence = 4;
                break;
            }
            case 4:
            if(current->type == Token_Type::land)
            {
                op = Binary_Operator::land;
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
                
                Dynamic_Array<AST*> args = {0};
                
                if(current->type != Token_Type::eof && current->type != Token_Type::close_paren)
                {
                    AST *first_expr = parse_expr(ctx, &current, parent);
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
                    
                    AST *expr = parse_expr(ctx, &current, parent);
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
                
                Function_Call_AST *call_ast = construct_ast(&ctx->ast_pool, Function_Call_AST, lhs->line_number, lhs->line_offset);
                
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
                AST *rhs = parse_expr(ctx, &current, parent);
                if(rhs)
                {
                    if(current->type == Token_Type::close_sqr)
                    {
                        ++current;
                        
                        Binary_Operator_AST *result = construct_ast(&ctx->ast_pool, Binary_Operator_AST, lhs->line_number, lhs->line_offset);
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
                    // Note: not sure if this one strictly needs the parent set, it can hardly hurt
                    rhs->parent = parent; 
                    ++current;
                    
                    Binary_Operator_AST *result = construct_ast(&ctx->ast_pool, Binary_Operator_AST, lhs->line_number, lhs->line_offset);
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
            AST *rhs = parse_expr(ctx, &current, parent, rhs_precedence);
            if(rhs)
            {
                Binary_Operator_AST *result = construct_ast(&ctx->ast_pool, Binary_Operator_AST, lhs->line_number, lhs->line_offset);
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


internal AST *parse_base_expr(Parsing_Context *ctx, Token **current_ptr, Parent_Scope parent, u32 precedence)
{
    Token *current = *current_ptr;
    Token *start_section = current;
    AST *result = nullptr;
    defer {
        *current_ptr = current;
    };
    
    switch(current->type)
    {
        case Token_Type::ident: {
            Ident_AST *result_ident = pool_alloc(Ident_AST, &ctx->ast_pool, 1);
            *result_ident = make_ident_ast(*current);
            result_ident->parent = parent;
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
                Decl_AST *decl = parse_decl(ctx, &current, parent, Decl_Type::Enum);
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
            
            Enum_AST *enum_ast = construct_ast(&ctx->ast_pool, Enum_AST, line_number, line_offset);
            enum_ast->values.count = values.count;
            enum_ast->values.data = pool_alloc(Decl_AST*, &ctx->ast_pool, values.count);
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
                Decl_AST *decl = parse_decl(ctx, &current, parent, Decl_Type::Struct);
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
            
            Struct_AST *struct_ast = construct_ast(&ctx->ast_pool, Struct_AST, line_number, line_offset);
            
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
        } break;
        case Token_Type::open_paren: {
            // This could be a parenthesized expression, a function type, or a function literal
            
            u32 line_number = current->line_number;
            u32 line_offset = current->line_offset;
            
            ++current;
            
            Dynamic_Array<Parameter_AST> parameters = {0};
            
            bool expect_more = (current->type != Token_Type::eof && current->type != Token_Type::close_paren);
            bool must_be_func = false;
            bool must_have_body = false;
            
            Parent_Scope blank_parent;
            blank_parent.ast = nullptr;
            blank_parent.index = 0; // TODO: does this matter?
            
            while(expect_more)
            {
                Ident_AST *ident = nullptr;
                AST *type = nullptr;
                AST *default_value = nullptr;
                
                AST *expr = parse_expr(ctx, &current, blank_parent);
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
                        type = parse_expr(ctx, &current, blank_parent);
                        
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
                        default_value = parse_expr(ctx, &current, blank_parent);
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
                ++blank_parent.index;
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
                
                AST *result = parameters[0].type;
                if(parent.ast)
                {
                    set_parent_ast(result, parent.ast);
                }
                return result;
            }
            
            // Now parse return type
            AST *return_type = parse_expr(ctx, &current, blank_parent);
            if(!return_type)
            {
                return nullptr;
            }
            
            Block_AST *block = nullptr;
            
            if(current->type == Token_Type::open_brace)
            {
                Parent_Scope block_parent;
                block_parent.ast = nullptr; // Will be filled in later
                block_parent.index = parameters.count;
                
                block = parse_statement_block(ctx, &current, block_parent);
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
            
            
            Function_Type_AST *func_type = construct_ast(&ctx->ast_pool, Function_Type_AST, line_number, line_offset);
            
            func_type->parameter_types.count = parameters.count;
            func_type->parameter_types.data = pool_alloc(AST*, &ctx->ast_pool, parameters.count);
            
            func_type->return_types.count = 1;
            func_type->return_types.data = pool_alloc(AST*, &ctx->ast_pool, 1);
            func_type->return_types[0] = return_type;
            
            if(block)
            {
                Function_AST *result_func = construct_ast(&ctx->ast_pool, Function_AST, line_number, line_offset);
                
                result_func->prototype = func_type;
                result_func->block = block;
                
                result_func->param_names.count = parameters.count;
                result_func->param_names.data = pool_alloc(Ident_AST*, &ctx->ast_pool, parameters.count);
                result_func->default_values.count = parameters.count;
                result_func->default_values.data = pool_alloc(AST*, &ctx->ast_pool, parameters.count);
                result_func->parent = parent;
                
                for(u64 i = 0; i < parameters.count; ++i)
                {
                    func_type->parameter_types[i] = parameters[i].type;
                    result_func->param_names[i] = parameters[i].name;
                    result_func->default_values[i] = parameters[i].default_value;
                    if(result_func->default_values[i])
                    {
                        set_parent_ast(result_func->default_values[i], result_func);
                    }
                }
                
                if(parent.ast)
                {
                    set_parent_ast(result_func, parent.ast);
                }
                set_parent_ast(block, result_func);
                result = result_func;
            }
            else
            {
                for(u64 i = 0; i < parameters.count; ++i)
                {
                    func_type->parameter_types[i] = parameters[i].type;
                }
                if(parent.ast)
                {
                    set_parent_ast(func_type, parent.ast);
                }
                result = func_type;
            }
        } break;
        case Token_Type::number: {
            Number_AST *result_number = pool_alloc(Number_AST, &ctx->ast_pool, 1);
            *result_number = make_number_ast(*current);
            result = result_number;
            ++current;
        } break;
        case Token_Type::string: {
            String_AST *result_string = construct_ast(&ctx->ast_pool, String_AST, current->line_number, current->line_offset);
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
                value.data = pool_alloc(byte, &ctx->ast_pool, new_size);
                
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
            Primitive_Type prim_type;
            
            case Token_Type::key_bool: {
                prim_type = Primitive_Type::bool_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_bool8: {
                prim_type = Primitive_Type::bool8_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_bool16: {
                prim_type = Primitive_Type::bool16_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_bool32: {
                prim_type = Primitive_Type::bool32_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_bool64: {
                prim_type = Primitive_Type::bool64_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_s8: {
                prim_type = Primitive_Type::s8_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_s16: {
                prim_type = Primitive_Type::s16_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_s32: {
                prim_type = Primitive_Type::s32_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_s64: {
                prim_type = Primitive_Type::s64_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_int: {
                prim_type = Primitive_Type::int_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_u8: {
                prim_type = Primitive_Type::u8_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_u16: {
                prim_type = Primitive_Type::u16_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_u32: {
                prim_type = Primitive_Type::u32_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_u64: {
                prim_type = Primitive_Type::u64_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_uint: {
                prim_type = Primitive_Type::uint_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_f32: {
                prim_type = Primitive_Type::f32_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_f64: {
                prim_type = Primitive_Type::f64_t;
                goto make_primitive_type_ast;
            } break;
            case Token_Type::key_void: {
                prim_type = Primitive_Type::void_t;
                goto make_primitive_type_ast;
            } break;
            
            // TODO: these can be 'interned'
            
            make_primitive_type_ast:
            
            Primitive_AST *prim_ast = construct_ast(&ctx->ast_pool, Primitive_AST, current->line_number, current->line_offset);
            prim_ast->primitive = prim_type;
            
            ++current;
            result = prim_ast;
        } break;
        
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
            
            Bool_AST *bool_ast = construct_ast(&ctx->ast_pool, Bool_AST, current->line_number, current->line_offset);
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
            AST *operand = parse_expr(ctx, &current, parent, precedence);
            if(!operand)
            {
                return nullptr;
            }
            
            Unary_Operator_AST *unary_ast = construct_ast(&ctx->ast_pool, Unary_Operator_AST, line_number, line_offset);
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

internal AST *parse_statement(Parsing_Context *ctx, Token **current_ptr, Parent_Scope parent)
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
                induction_var = pool_alloc(Ident_AST, &ctx->ast_pool, 1);
                *induction_var = make_ident_ast(*first_ident);
                
                if(second_ident)
                {
                    index_var = pool_alloc(Ident_AST, &ctx->ast_pool, 1);
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
        
        AST *low_range_expr = nullptr;
        AST *high_range_expr = nullptr;
        
        low_range_expr = parse_expr(ctx, &current, parent);
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
            high_range_expr = parse_expr(ctx, &current, parent);
            if(!high_range_expr)
            {
                return nullptr;
            }
        }
        
        Parent_Scope block_scope = {0};
        
        Block_AST *body = parse_statement_block(ctx, &current, block_scope);
        if(!body)
        {
            return nullptr;
        }
        
        For_AST *for_ast = construct_ast(&ctx->ast_pool, For_AST, line_number, line_offset);
        if(by_pointer)
        {
            for_ast->flags |= FOR_FLAG_BY_POINTER;
        }
        
        if(!induction_var)
        {
            induction_var = construct_ast(&ctx->ast_pool, Ident_AST, 0, 0);
            induction_var->ident = str_lit("it");
            induction_var->referred_to = nullptr;
            induction_var->parent = {0};
            induction_var->flags |= AST_FLAG_SYNTHETIC;
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
                index_var = construct_ast(&ctx->ast_pool, Ident_AST, 0, 0);
                index_var->ident = str_lit("it_index");
                index_var->referred_to = nullptr;
                index_var->parent = {0};
                index_var->flags |= AST_FLAG_SYNTHETIC;
            }
            
            for_ast->flags |= FOR_FLAG_OVER_ARRAY;
            for_ast->index_var = index_var;
            for_ast->array_expr = low_range_expr;
        }
        
        for_ast->parent = parent;
        for_ast->body->parent.ast = for_ast;
        
        result = for_ast;
    }
    else if(current->type == Token_Type::key_if)
    {
        ++current;
        AST *expr = parse_expr(ctx, &current, parent);
        if(!expr)
        {
            return nullptr;
        }
        Block_AST *then_block = parse_statement_block(ctx, &current, parent);
        if(!then_block)
        {
            return nullptr;
        }
        
        Block_AST *else_block = nullptr;
        
        if(current->type == Token_Type::key_else)
        {
            ++current;
            else_block = parse_statement_block(ctx, &current, parent);
            if(!else_block)
            {
                return nullptr;
            }
        }
        
        If_AST *if_ast = construct_ast(&ctx->ast_pool, If_AST, line_number, line_offset);
        if_ast->guard = expr;
        if_ast->then_block = then_block;
        if_ast->else_block = else_block;
        
        result = if_ast;
    }
    else if(current->type == Token_Type::key_while)
    {
        ++current;
        AST *expr = parse_expr(ctx, &current, parent);
        if(!expr)
        {
            return nullptr;
        }
        Block_AST *body = parse_statement_block(ctx, &current, parent);
        
        While_AST *while_ast = construct_ast(&ctx->ast_pool, While_AST, line_number, line_offset);
        while_ast->guard = expr;
        while_ast->body = body;
        
        result = while_ast;
    }
    else if(current->type == Token_Type::key_return)
    {
        require_semicolon = true;
        ++current;
        AST *expr = parse_expr(ctx, &current, parent);
        if(!expr)
        {
            return nullptr;
        }
        
        Return_AST *return_ast = construct_ast(&ctx->ast_pool, Return_AST, line_number, line_offset);
        return_ast->expr = expr;
        
        result = return_ast;
    }
    else if(current->type == Token_Type::open_brace)
    {
        result = parse_statement_block(ctx, &current, parent);
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
                Decl_AST *decl = parse_decl(ctx, &current, parent, Decl_Type::Statement);
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
                AST *expr = parse_expr(ctx, &current, parent);
                if(!expr)
                {
                    return nullptr;
                }
                require_semicolon = true;
                
                Assign_AST *assign = construct_ast(&ctx->ast_pool, Assign_AST, line_number, line_offset);
                
                assign->ident = make_ident_ast(*at_ident);
                assign->ident.parent = parent;
                assign->assign_type = assign_type;
                assign->rhs = expr;
                
                result = assign;
            }
            else
            {
                // Expect an expression;
                current = at_ident;
                AST *expr = parse_expr(ctx, &current, parent);
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
            AST *expr = parse_expr(ctx, &current, parent);
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

internal Block_AST *parse_statement_block(Parsing_Context *ctx, Token **current_ptr, Parent_Scope parent)
{
    Token *current = *current_ptr;
    Token *start_section = current;
    Block_AST *result = nullptr;
    defer {
        *current_ptr = current;
    };
    
    if(current->type == Token_Type::open_brace)
    {
        result = construct_ast(&ctx->ast_pool, Block_AST, current->line_number, current->line_offset);
        result->parent = parent;
        ++current;
        
        Parent_Scope new_parent;
        new_parent.ast = result;
        new_parent.index = 0;
        
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
                AST *stmt = parse_statement(ctx, &current, new_parent);
                if(stmt)
                {
                    array_add(&statements, stmt);
                    ++new_parent.index;
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

Decl_AST *parse_decl(Parsing_Context *ctx, Token **current_ptr, Parent_Scope parent, Decl_Type decl_type)
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
        
        Parent_Scope blank_parent = {0};
        
        bool not_enum = (decl_type != Decl_Type::Enum);
        // TODO: remove require_value, variables should be able to take on default values (e.g. zero initialized)
        bool require_value = (decl_type == Decl_Type::Statement);
        bool expect_expr;
        bool is_constant;
        
        if(current->type == Token_Type::colon && not_enum)
        {
            ++current;
            type = parse_expr(ctx, &current, blank_parent);
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
            expr = parse_expr(ctx, &current, blank_parent);
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
        
        result = construct_ast(&ctx->ast_pool, Decl_AST, line_number, line_offset);
        result->ident = make_ident_ast(*ident_tok);
        result->decl_type = type;
        result->expr = expr;
        
        result->parent = parent;
        if(result->decl_type)
        {
            set_parent_ast(result->decl_type, result);
        }
        if(result->expr)
        {
            set_parent_ast(result->expr, result);
        }
        
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
    
    Parent_Scope global_scope = {0};
    
    while(current->type != Token_Type::eof)
    {
        start_section = current;
        Decl_AST *decl = parse_decl(ctx, &current, global_scope, Decl_Type::Statement);
        if(decl)
        {
            array_add(&result, decl);
            ++global_scope.index;
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

bool draw_parents = true;

internal void print_dot_rec(Print_Buffer *pb, AST *ast);
internal inline void print_dot_child(Print_Buffer *pb, AST *child, u64 parent_serial)
{
    u64 child_serial = child->s;
    print_buf(pb, "n%ld->n%ld;\n", parent_serial, child_serial);
    print_dot_rec(pb, child);
}

internal void print_dot_rec(Print_Buffer *pb, AST *ast)
{
    u32 s = ast->s;
    switch(ast->type)
    {
        case AST_Type::ident_ast: {
            Ident_AST *ident_ast = static_cast<Ident_AST*>(ast);
            print_buf(pb, "n%ld[label=\"%.*s\"];\n", s, (u32)ident_ast->ident.count, ident_ast->ident.data);
            if(draw_parents && ident_ast->parent.ast)
            {
                print_buf(pb, "n%ld->n%ld[style=dotted];\n", s, ident_ast->parent.ast->s);
            }
        } break;
        case AST_Type::decl_ast: {
            Decl_AST *decl_ast = static_cast<Decl_AST*>(ast);
            
            print_buf(pb, "n%ld[label=\"Declare %.*s\"];\nn%ld[shape=box];\n", s, (u32)decl_ast->ident.ident.count, decl_ast->ident.ident.data, s);
            if(decl_ast->decl_type)
            {
                print_dot_child(pb, decl_ast->decl_type, s);
            }
            if(decl_ast->expr)
            {
                print_dot_child(pb, decl_ast->expr, s);
            }
            if(draw_parents && decl_ast->parent.ast)
            {
                print_buf(pb, "n%ld->n%ld[style=dotted];\n", s, decl_ast->parent.ast->s);
            }
        } break;
        case AST_Type::block_ast: {
            Block_AST *block_ast = static_cast<Block_AST*>(ast);
            
            print_buf(pb, "n%ld[label=\"Block\"];\nn%ld[shape=box];\n", s, s);
            
            for(u64 i = 0; i < block_ast->statements.count; ++i)
            {
                print_dot_child(pb, block_ast->statements[i], s);
            }
            if(draw_parents && block_ast->parent.ast)
            {
                print_buf(pb, "n%ld->n%ld[style=dotted];\n", s, block_ast->parent.ast->s);
            }
        } break;
        case AST_Type::function_type_ast: {
            Function_Type_AST *type_ast = static_cast<Function_Type_AST*>(ast);
            print_buf(pb, "n%ld[label=\"Function Type\"];\n", s);
            
            for(u64 i = 0; i < type_ast->parameter_types.count; ++i)
            {
                if(type_ast->parameter_types[i])
                {
                    print_dot_child(pb, type_ast->parameter_types[i], s);
                }
            }
            for(u64 i = 0; i < type_ast->return_types.count; ++i)
            {
                if(type_ast->return_types[i])
                {
                    print_dot_child(pb, type_ast->return_types[i], s);
                }
            }
        } break;
        case AST_Type::function_ast: {
            Function_AST *function_ast = static_cast<Function_AST*>(ast);
            
            print_buf(pb, "n%ld[label=\"Function\"];\nn%ld[shape=box];\n", s, s);
            
            print_dot_child(pb, function_ast->prototype, s);
            for(u64 i = 0; i < function_ast->param_names.count; ++i)
            {
                if(function_ast->param_names[i])
                {
                    print_dot_child(pb, function_ast->param_names[i], s);
                }
            }
            for(u64 i = 0; i < function_ast->default_values.count; ++i)
            {
                if(function_ast->default_values[i])
                {
                    print_dot_child(pb, function_ast->default_values[i], s);
                }
            }
            if(draw_parents && function_ast->parent.ast)
            {
                print_buf(pb, "n%ld->n%ld[style=dotted];\n", s, function_ast->parent.ast->s);
            }
            print_dot_child(pb, function_ast->block, s);
        } break;
        case AST_Type::function_call_ast: {
            Function_Call_AST *call_ast = static_cast<Function_Call_AST*>(ast);
            
            print_buf(pb, "n%ld[label=\"function call\"];\n", s);
            print_dot_child(pb, call_ast->function, s);
            for(u64 i = 0; i < call_ast->args.count; ++i)
            {
                print_dot_child(pb, call_ast->args[i], s);
            }
        } break;
        case AST_Type::binary_operator_ast: {
            Binary_Operator_AST *bin_ast = static_cast<Binary_Operator_AST*>(ast);
            
            print_buf(pb, "n%ld[label=\"%s\"];\n", s, binary_operator_names[(u64)bin_ast->op]);
            print_dot_child(pb, bin_ast->lhs, s);
            print_dot_child(pb, bin_ast->rhs, s);
        } break;
        case AST_Type::number_ast: {
            Number_AST *number_ast = static_cast<Number_AST*>(ast);
            print_buf(pb, "n%ld[label=\"%.*s\"];\n", s, (u32)number_ast->literal.count, number_ast->literal.data);
        } break;
        case AST_Type::while_ast: {
            While_AST *while_ast = static_cast<While_AST*>(ast);
            
            print_buf(pb, "n%ld[label=\"while\"];\n", s);
            print_dot_child(pb, while_ast->guard, s);
            print_dot_child(pb, while_ast->body, s);
        } break;
        case AST_Type::for_ast: {
            For_AST *for_ast = static_cast<For_AST*>(ast);
            
            if(for_ast->flags & FOR_FLAG_OVER_ARRAY)
            {
                if(for_ast->flags & FOR_FLAG_BY_POINTER)
                {
                    print_buf(pb, "n%ld[label=\"for &(array)\"];\n", s);
                }
                else
                {
                    print_buf(pb, "n%ld[label=\"for (array)\"];\n", s);
                }
            }
            else
            {
                assert(!(for_ast->flags & FOR_FLAG_BY_POINTER));
                print_buf(pb, "n%ld[label=\"for (range)\"];\n", s);
            }
            print_buf(pb, "n%ld[shape=box];\n", s);
            if(draw_parents && for_ast->parent.ast)
            {
                print_buf(pb, "n%ld->n%ld[style=dotted];\n", s, for_ast->parent.ast->s);
            }
            
            if(for_ast->induction_var)
            {
                print_dot_child(pb, for_ast->induction_var, s);
            }
            if(for_ast->flags & FOR_FLAG_OVER_ARRAY)
            {
                if(for_ast->index_var)
                {
                    print_dot_child(pb, for_ast->index_var, s);
                }
                print_dot_child(pb, for_ast->array_expr, s);
            }
            else
            {
                print_dot_child(pb, for_ast->low_expr, s);
                print_dot_child(pb, for_ast->high_expr, s);
            }
            
            print_dot_child(pb, for_ast->body, s);
        } break;
        case AST_Type::if_ast: {
            If_AST *if_ast = static_cast<If_AST*>(ast);
            
            print_buf(pb, "n%ld[label=\"if\"];\n", s);
            print_dot_child(pb, if_ast->guard, s);
            print_dot_child(pb, if_ast->then_block, s);
            if(if_ast->else_block)
            {
                print_dot_child(pb, if_ast->else_block, s);
            }
        } break;
        case AST_Type::struct_ast: {
            Struct_AST *struct_ast = static_cast<Struct_AST*>(ast);
            
            print_buf(pb, "n%ld[label=\"struct\"];\n", s);
            for(u64 i = 0; i < struct_ast->constants.count; ++i)
            {
                print_dot_child(pb, struct_ast->constants[i], s);
            }
            for(u64 i = 0; i < struct_ast->fields.count; ++i)
            {
                print_dot_child(pb, struct_ast->fields[i], s);
            }
        } break;
        case AST_Type::enum_ast: {
            Enum_AST *enum_ast = static_cast<Enum_AST*>(ast);
            
            print_buf(pb, "n%ld[label=\"enum\"];\n", s);
            for(u64 i = 0; i < enum_ast->values.count; ++i)
            {
                print_dot_child(pb, enum_ast->values[i], s);
            }
        } break;
        case AST_Type::assign_ast: {
            Assign_AST *assign_ast = static_cast<Assign_AST*>(ast);
            
            print_buf(pb, "n%ld[label=\"%s\"];\n", s, assign_names[(u64)assign_ast->assign_type]);
            print_dot_child(pb, &assign_ast->ident, s);
            print_dot_child(pb, assign_ast->rhs, s);
        } break;
        case AST_Type::unary_ast: {
            Unary_Operator_AST *unary_ast = static_cast<Unary_Operator_AST*>(ast);
            
            print_buf(pb, "n%ld[label=\"%s\"];\n", s, unary_operator_names[(u64)unary_ast->op]);
            print_dot_child(pb, unary_ast->operand, s);
        } break;
        case AST_Type::return_ast: {
            Return_AST *return_ast = static_cast<Return_AST*>(ast);
            
            print_buf(pb, "n%ld[label=\"return\"];\n", s);
            print_dot_child(pb, return_ast->expr, s);
        } break;
        case AST_Type::primitive_ast: {
            Primitive_AST *primitive_ast = static_cast<Primitive_AST*>(ast);
            
            print_buf(pb, "n%ld[label=\"%s\"];\n", s, primitive_names[(u64)primitive_ast->primitive]);
        } break;
        case AST_Type::string_ast: {
            String_AST *string_ast = static_cast<String_AST*>(ast);
            
            print_buf(pb, "n%ld[label=\"\\\"%.*s\\\"\"];\n", s, string_ast->literal.count, string_ast->literal.data);
        } break;
        case AST_Type::bool_ast: {
            Bool_AST *bool_ast = static_cast<Bool_AST*>(ast);
            if(bool_ast->value)
            {
                print_buf(pb, "n%ld[label=\"true\"];\n", s);
            }
            else
            {
                print_buf(pb, "n%ld[label=\"false\"];\n", s);
            }
        } break;
        default: {
            print_err("Unknown AST type in dot printer\n");
        } break;
    }
}

void print_dot(Print_Buffer *pb, Array<Decl_AST*> decls)
{
    print_buf(pb, "digraph decls {\n");
    
    for(u64 i = 0; i < decls.count; ++i)
    {
        print_dot_rec(pb, decls[i]);
    }
    
    print_buf(pb, "}\n");
    flush_buffer(pb);
}
