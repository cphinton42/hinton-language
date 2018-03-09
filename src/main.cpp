

void report_error(const byte *msg, AST *ast)
{
    print_err("Error: %d:%d: %s\n", ast->line_number, ast->line_offset, msg);
}

void report_error(const byte *msg, AST *t1, AST *t2)
{
    print_err("Type Error: %d:%d vs %d:%d: %s\n", t1->line_number, t1->line_offset, t2->line_number, t2->line_offset, msg);
}

Expr_AST *find_ident(Array<Decl_AST*> globals, String ident, Parent_Scope scope)
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
                        if(decl_ast->flags & DECL_FLAG_CONSTANT)
                        {
                            return decl_ast->expr;
                        }
                        else
                        {
                            return &decl_ast->ident;
                        }
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
                                if(decl_ast->flags & DECL_FLAG_CONSTANT)
                                {
                                    return decl_ast->expr;
                                }
                                else
                                {
                                    return &decl_ast->ident;
                                }
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
                    return &globals[i]->ident;
                }
            }
            return nullptr;
        }
    }
}

void link_ast(Array<Decl_AST*> globals, Function_AST *current_function, AST *current)
{
    switch(current->type)
    {
        case AST_Type::refer_ident_ast: {
            Refer_Ident_AST *ident_ast = static_cast<Refer_Ident_AST*>(current);
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
                link_ast(globals, current_function, decl_ast->decl_type);
            }
            if(decl_ast->expr)
            {
                link_ast(globals, current_function, decl_ast->expr);
            }
        } break;
        case AST_Type::block_ast: {
            Block_AST *block_ast = static_cast<Block_AST*>(current);
            for(u64 i = 0; i < block_ast->statements.count; ++i)
            {
                link_ast(globals, current_function, block_ast->statements[i]);
            }
        } break;
        case AST_Type::function_type_ast: {
            Function_Type_AST *function_type_ast = static_cast<Function_Type_AST*>(current);
            for(u64 i = 0; i < function_type_ast->parameter_types.count; ++i)
            {
                if(function_type_ast->parameter_types[i])
                {
                    link_ast(globals, current_function, function_type_ast->parameter_types[i]);
                }
            }
            for(u64 i = 0; i < function_type_ast->return_types.count; ++i)
            {
                if(function_type_ast->return_types[i])
                {
                    link_ast(globals, current_function, function_type_ast->return_types[i]);
                }
            }
        } break;
        case AST_Type::function_ast: {
            Function_AST *function_ast = static_cast<Function_AST*>(current);
            link_ast(globals, function_ast, function_ast->prototype);
            for(u64 i = 0; i < function_ast->default_values.count; ++i)
            {
                if(function_ast->default_values[i])
                {
                    link_ast(globals, function_ast, function_ast->default_values[i]);
                }
            }
            link_ast(globals, function_ast, function_ast->block);
        } break;
        case AST_Type::function_call_ast: {
            Function_Call_AST *function_call_ast = static_cast<Function_Call_AST*>(current);
            for(u64 i = 0; i < function_call_ast->args.count; ++i)
            {
                link_ast(globals, current_function, function_call_ast->args[i]);
            }
            link_ast(globals, current_function, function_call_ast->function);
        } break;
        case AST_Type::binary_operator_ast: {
            Binary_Operator_AST *binary_operator_ast = static_cast<Binary_Operator_AST*>(current);
            link_ast(globals, current_function, binary_operator_ast->lhs);
            if(binary_operator_ast->op != Binary_Operator::access)
            {
                link_ast(globals, current_function, binary_operator_ast->rhs);
            }
        } break;
        case AST_Type::while_ast: {
            While_AST *while_ast = static_cast<While_AST*>(current);
            link_ast(globals, current_function, while_ast->guard);
            link_ast(globals, current_function, while_ast->body);
        } break;
        case AST_Type::for_ast: {
            For_AST *for_ast = static_cast<For_AST*>(current);
            if(for_ast->flags & FOR_FLAG_OVER_ARRAY)
            {
                link_ast(globals, current_function, for_ast->array_expr);
            }
            else
            {
                link_ast(globals, current_function, for_ast->low_expr);
                link_ast(globals, current_function, for_ast->high_expr);
            }
            link_ast(globals, current_function, for_ast->body);
        } break;
        case AST_Type::if_ast: {
            If_AST *if_ast = static_cast<If_AST*>(current);
            link_ast(globals, current_function, if_ast->guard);
            link_ast(globals, current_function, if_ast->then_block);
            if(if_ast->else_block)
            {
                link_ast(globals, current_function, if_ast->else_block);
            }
        } break;
        case AST_Type::enum_ast: {
            Enum_AST *enum_ast = static_cast<Enum_AST*>(current);
            for(u64 i = 0; i < enum_ast->values.count; ++i)
            {
                link_ast(globals, current_function, enum_ast->values[i]);
            }
        } break;
        case AST_Type::struct_ast: {
            Struct_AST *struct_ast = static_cast<Struct_AST*>(current);
            for(u64 i = 0; i < struct_ast->constants.count; ++i)
            {
                link_ast(globals, current_function, struct_ast->constants[i]);
            }
            for(u64 i = 0; i < struct_ast->fields.count; ++i)
            {
                link_ast(globals, current_function, struct_ast->fields[i]);
            }
        } break;
        case AST_Type::assign_ast: {
            Assign_AST *assign_ast = static_cast<Assign_AST*>(current);
            link_ast(globals, current_function, &assign_ast->ident);
            link_ast(globals, current_function, assign_ast->rhs);
        } break;
        case AST_Type::unary_ast: {
            Unary_Operator_AST *unary_operator_ast = static_cast<Unary_Operator_AST*>(current);
            link_ast(globals, current_function, unary_operator_ast->operand);
        } break;
        case AST_Type::return_ast: {
            Return_AST *return_ast = static_cast<Return_AST*>(current);
            return_ast->function = current_function;
            link_ast(globals, current_function, return_ast->expr);
        } break;
        case AST_Type::def_ident_ast:
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
        link_ast(globals, nullptr, globals[i]);
    }
}

struct Infer_Context
{
    Pool_Allocator *ast_pool;
};

struct Typecheck_Result
{
    bool ok;
    bool waiting;
    bool progress;
};


Typecheck_Result typecheck_result(bool ok, bool waiting, bool progress)
{
    Typecheck_Result result;
    result.ok = ok;
    result.waiting = waiting;
    result.progress = progress;
    return result;
}

internal inline
Typecheck_Result combine_typecheck_result(Typecheck_Result r1, Typecheck_Result r2)
{
    Typecheck_Result result;
    result.ok = r1.ok && r2.ok;
    result.waiting = r1.waiting || r2.waiting;
    result.progress = r1.progress || r2.progress;
    return result;
}

/*
bool ast_is_expr(AST *ast)
{
    return ast_type_is_expr[(u16)ast->type];
}
*/

/*
switch()
{
    case AST_Type::def_ident_ast: {
    } break;
    case AST_Type::refer_ident_ast: {
    } break;
    case AST_Type::function_type_ast: {
    } break;
    case AST_Type::function_ast: {
    } break;
    case AST_Type::function_call_ast: {
    } break;
    case AST_Type::binary_operator_ast: {
    } break;
    case AST_Type::number_ast: {
    } break;
    case AST_Type::enum_ast: {
    } break;
    case AST_Type::struct_ast: {
    } break;
    case AST_Type::unary_ast: {
    } break;
    case AST_Type::primitive_ast: {
    } break;
    case AST_Type::string_ast: {
    } break;
    case AST_Type::bool_ast: {
    } break;
}
*/

Typecheck_Result check_expr_is_lvalue(Expr_AST *expr)
{
    Typecheck_Result result = typecheck_result(true,false,false);
    
    switch(expr->type)
    {
        case AST_Type::function_type_ast: {
            report_error("A function type is not an l-value", expr);
            result.ok = false;
        } break;
        case AST_Type::number_ast: {
            report_error("A number literal is not an l-value", expr);
            result.ok = false;
        } break;
        case AST_Type::enum_ast: {
            report_error("An enum type is not an l-value", expr);
            result.ok = false;
        } break;
        case AST_Type::struct_ast: {
            report_error("A struct type is not an l-value", expr);
            result.ok = false;
        } break;
        case AST_Type::primitive_ast: {
            report_error("A primitive type is not an l-value", expr);
            result.ok = false;
        } break;
        case AST_Type::bool_ast: {
            report_error("A boolean literal is not an l-value", expr);
            result.ok = false;
        } break;
        case AST_Type::function_call_ast: {
            report_error("The result of a function call is not an l-value (?)", expr);
            result.ok = false;
        } break;
        case AST_Type::binary_operator_ast: {
            Binary_Operator_AST *binop_ast = static_cast<Binary_Operator_AST*>(expr);
            switch(binop_ast->op)
            {
                case Binary_Operator::access:
                case Binary_Operator::subscript: {
                    // OK
                } break;
                case Binary_Operator::cmp_eq:
                case Binary_Operator::cmp_neq:
                case Binary_Operator::cmp_lt:
                case Binary_Operator::cmp_le:
                case Binary_Operator::cmp_gt:
                case Binary_Operator::cmp_ge: {
                    report_error("The result of a comparison is not an l-value", expr);
                    result.ok = false;
                } break;
                case Binary_Operator::add:
                case Binary_Operator::sub:
                case Binary_Operator::mul:
                case Binary_Operator::div: {
                    report_error("The result of arithmetic is not an l-value", expr);
                    result.ok = false;
                } break;
                case Binary_Operator::lor:
                case Binary_Operator::land: {
                    report_error("The result of logical operators is not an l-value", expr);
                    result.ok = false;
                } break;
            }
        } break;
        case AST_Type::unary_ast: {
            report_error("The result of a unary operator is not an l-value", expr);
            result.ok = false;
        } break;
        case AST_Type::def_ident_ast:
        case AST_Type::refer_ident_ast:
        case AST_Type::function_ast:
        case AST_Type::string_ast: {
        } break;
        default: {
            assert(false);
        } break;
    }
    
    return result;
}


Typecheck_Result eval_type(Expr_AST *expr, Expr_AST **eval_out)
{
    Typecheck_Result result = typecheck_result(true,false,false);
    
    switch(expr->type)
    {
        case AST_Type::refer_ident_ast: {
            Refer_Ident_AST *ident_ast = static_cast<Refer_Ident_AST*>(expr);
            if(ident_ast->referred_to)
            {
                return eval_type(ident_ast->referred_to, eval_out);
            }
            else
            {
                result.waiting = true;
            }
        } break;
        case AST_Type::def_ident_ast: {
            Def_Ident_AST *ident_ast = static_cast<Def_Ident_AST*>(expr);
            if(ident_ast->resolved_type)
            {
                if(ident_ast->resolved_type == &type_t_ast)
                {
                    *eval_out = ident_ast;
                }
                else
                {
                    report_error("Expected type", ident_ast);
                    result.ok = false;
                }
            }
            else
            {
                result.waiting = true;
            }
        } break;
        case AST_Type::function_call_ast:
        case AST_Type::binary_operator_ast: {
            // TODO: these could probably return a pointer type in the future
            // specifically, if return type is 'type', function marked as pure, and arguments known at compile time
            if(expr->resolved_type)
            {
                if(expr->resolved_type == &type_t_ast)
                {
                    report_error("Type from function call or operator not supported yet", expr);
                    result.ok = false;
                }
                else
                {
                    report_error("Expected type", expr);
                    result.ok = false;
                }
            }
            else
            {
                result.waiting = true;
            }
        } break;
        case AST_Type::unary_ast: {
            Unary_Operator_AST *unop_ast = static_cast<Unary_Operator_AST*>(expr);
            if(unop_ast->resolved_type)
            {
                if(unop_ast->resolved_type == &type_t_ast)
                {
                    // TODO: in the future, other operators could yield a type (with operator overloading, a pure operator, and arguments are compile time constants
                    
                    if(unop_ast->op != Unary_Operator::ref)
                    {
                        report_error("Expected pointer type", expr);
                        result.ok = false;
                    }
                }
                else
                {
                    report_error("Expected type", unop_ast);
                    result.ok = false;
                }
            }
            else
            {
                result.waiting = true;
            }
        } break;
        case AST_Type::function_type_ast:
        case AST_Type::enum_ast:
        case AST_Type::struct_ast:
        case AST_Type::primitive_ast: {
            *eval_out = expr;
        } break;
        
        case AST_Type::function_ast:
        case AST_Type::number_ast:
        case AST_Type::string_ast:
        case AST_Type::bool_ast: {
            report_error("Expected type", expr);
            result.ok = false;
        } break;
        default: {
            assert(false);
        } break;
    }
    
    return result;
}


Typecheck_Result get_as_pointer_type(Expr_AST *expr, Unary_Operator_AST **type_ast)
{
    Typecheck_Result result = typecheck_result(true,false,false);
    
    switch(expr->type)
    {
        case AST_Type::refer_ident_ast: {
            Refer_Ident_AST *ident_ast = static_cast<Refer_Ident_AST*>(expr);
            if(ident_ast->referred_to)
            {
                return get_as_pointer_type(ident_ast->referred_to, type_ast);
            }
            else
            {
                result.waiting = true;
            }
        } break;
        
        case AST_Type::def_ident_ast:
        case AST_Type::function_type_ast:
        case AST_Type::function_call_ast:
        case AST_Type::binary_operator_ast: {
            // TODO: these could probably return a pointer type in the future
            report_error("Expected pointer type", expr);
            result.ok = false;
        } break;
        
        case AST_Type::unary_ast: {
            Unary_Operator_AST *unop_ast = static_cast<Unary_Operator_AST*>(expr);
            if(unop_ast->op != Unary_Operator::ref)
            {
                // TODO: in the future, other operators could yield a pointer type?
                report_error("Expected pointer type", expr);
                result.ok = false;
            }
            if(unop_ast->resolved_type)
            {
                if(unop_ast->resolved_type != &type_t_ast)
                {
                    report_error("Expected pointer type", expr);
                    result.ok = false;
                }
                else
                {
                    *type_ast = unop_ast;
                }
            }
            else
            {
                result.waiting = true;
            }
        } break;
        case AST_Type::function_ast:
        case AST_Type::number_ast:
        case AST_Type::enum_ast:
        case AST_Type::struct_ast:
        case AST_Type::primitive_ast:
        case AST_Type::string_ast:
        case AST_Type::bool_ast: {
            report_error("Expected pointer type", expr);
            result.ok = false;
        } break;
        default: {
            assert(false);
        } break;
    }
    
    return result;
}

Typecheck_Result get_as_function_type(Expr_AST *expr, Function_Type_AST **type_ast)
{
    Typecheck_Result result = typecheck_result(true,false,false);
    
    switch(expr->type)
    {
        case AST_Type::refer_ident_ast: {
            Refer_Ident_AST *ident_ast = static_cast<Refer_Ident_AST*>(expr);
            if(ident_ast->referred_to)
            {
                return get_as_function_type(ident_ast->referred_to, type_ast);
            }
            else
            {
                result.waiting = true;
            }
        } break;
        case AST_Type::function_type_ast: {
            *type_ast = static_cast<Function_Type_AST*>(expr);
        } break;
        case AST_Type::def_ident_ast:
        case AST_Type::function_call_ast:
        case AST_Type::binary_operator_ast:
        case AST_Type::unary_ast: {
            // TODO: these could probably return a function type in the future
            report_error("Expected function type", expr);
            result.ok = false;
        } break;
        case AST_Type::function_ast:
        case AST_Type::number_ast:
        case AST_Type::enum_ast:
        case AST_Type::struct_ast:
        case AST_Type::primitive_ast:
        case AST_Type::string_ast:
        case AST_Type::bool_ast: {
            report_error("Expected function type", expr);
            result.ok = false;
        } break;
        default: {
            assert(false);
        } break;
    }
    
    return result;
}

Typecheck_Result get_as_primitive_type(Expr_AST *expr, Primitive_AST **type_ast)
{
    Typecheck_Result result = typecheck_result(true,false,false);
    
    switch(expr->type)
    {
        case AST_Type::refer_ident_ast: {
            Refer_Ident_AST *ident_ast = static_cast<Refer_Ident_AST*>(expr);
            if(ident_ast->referred_to)
            {
                return get_as_primitive_type(ident_ast->referred_to, type_ast);
            }
            else
            {
                result.waiting = true;
            }
        } break;
        case AST_Type::def_ident_ast:
        case AST_Type::function_call_ast:
        case AST_Type::binary_operator_ast:
        case AST_Type::unary_ast: {
            // TODO: these could probably return a primitive type in the future
            report_error("Expected primitive type", expr);
            result.ok = false;
        } break;
        case AST_Type::function_ast:
        case AST_Type::function_type_ast:
        case AST_Type::number_ast:
        case AST_Type::enum_ast:
        case AST_Type::struct_ast:
        case AST_Type::string_ast:
        case AST_Type::bool_ast: {
            report_error("Expected primitive type", expr);
            result.ok = false;
        } break;
        case AST_Type::primitive_ast: {
            *type_ast = static_cast<Primitive_AST*>(expr);
        } break;
        default: {
            assert(false);
        } break;
    }
    
    return result;
}

Typecheck_Result combine_number_types(Expr_AST *t1, Expr_AST *t2, Primitive_AST **prim_out)
{
    Primitive_AST *p1 = nullptr, *p2 = nullptr;
    Typecheck_Result result = get_as_primitive_type(t1, &p1);
    if(!result.ok || result.waiting)
    {
        return result;
    }
    result = get_as_primitive_type(t2, &p2);
    if(!result.ok || result.waiting)
    {
        return result;
    }
    
    assert(p1 && p2);
    
    u64 prim1 = p1->primitive;
    u64 prim2 = p2->primitive;
    
    u64 size1 = prim1 & PRIM_SIZE_MASK;
    u64 size2 = prim2 & PRIM_SIZE_MASK;
    u64 max_size = max(size1, size2);
    
    assert(prim1 & PRIM_NUMBERLIKE);
    assert(prim2 & PRIM_NUMBERLIKE);
    
    if((prim1 & PRIM_FLOATLIKE) || (prim2 & PRIM_FLOATLIKE))
    {
        if(max_size == PRIM_NO_SIZE)
        {
            *prim_out = &floatlike_t_ast;
        }
        else if(max_size == PRIM_SIZE4)
        {
            *prim_out = &f32_t_ast;
        }
        else
        {
            assert(max_size == PRIM_SIZE8);
            *prim_out = &f64_t_ast;
        }
    }
    else
    {
        // TODO: maybe a warning or error if mixing signed and unsigned?
        if((prim1 & PRIM_SIGN_MASK) == PRIM_UNSIGNED_INT && (prim1 & PRIM_SIGN_MASK) == PRIM_UNSIGNED_INT)
        {
            switch(max_size)
            {
                case PRIM_NO_SIZE: {
                    // TODO: maybe make uintlike_t_ast ?
                    *prim_out = &intlike_t_ast;
                } break;
                case PRIM_SIZE1: {
                    *prim_out = &u8_t_ast;
                } break;
                case PRIM_SIZE2: {
                    *prim_out = &u16_t_ast;
                } break;
                case PRIM_SIZE4: {
                    *prim_out = &u32_t_ast;
                } break;
                case PRIM_SIZE8: {
                    *prim_out = &u64_t_ast;
                } break;
                default: {
                    assert(false);
                } break;
            }
        }
        else
        {
            switch(max_size)
            {
                case PRIM_NO_SIZE: {
                    *prim_out = &intlike_t_ast;
                } break;
                case PRIM_SIZE1: {
                    *prim_out = &s8_t_ast;
                } break;
                case PRIM_SIZE2: {
                    *prim_out = &s16_t_ast;
                } break;
                case PRIM_SIZE4: {
                    *prim_out = &s32_t_ast;
                } break;
                case PRIM_SIZE8: {
                    *prim_out = &s64_t_ast;
                } break;
                default: {
                    assert(false);
                } break;
            }
        }
    }
    
    return result;
}

Typecheck_Result types_match(Expr_AST *lhs_type, Expr_AST *rhs_type)
{
    assert(lhs_type && rhs_type);
    
    Typecheck_Result result = typecheck_result(true, false, false);
    
    if(lhs_type->type == AST_Type::refer_ident_ast)
    {
        Refer_Ident_AST *lhs_ident = static_cast<Refer_Ident_AST*>(lhs_type);
        // TODO: is this the right guard?
        if(lhs_ident->referred_to->resolved_type)
        {
            return types_match(lhs_ident->referred_to, rhs_type);
        }
        else
        {
            result.waiting = true;
        }
    }
    if(rhs_type->type == AST_Type::refer_ident_ast)
    {
        Refer_Ident_AST *rhs_ident = static_cast<Refer_Ident_AST*>(rhs_type);
        // TODO: is this the right guard?
        if(rhs_ident->referred_to->resolved_type)
        {
            return types_match(lhs_type, rhs_ident->referred_to);
        }
        else
        {
            result.waiting = true;
        }
    }
    
    if(!lhs_type->resolved_type || !rhs_type->resolved_type)
    {
        result.waiting = true;
        return result;
    }
    if(lhs_type->resolved_type != &type_t_ast || rhs_type->resolved_type != &type_t_ast)
    {
        report_error("Expected types", lhs_type, rhs_type);
        result.ok = false;
        return result;
    }
    
    switch(lhs_type->type)
    {
        case AST_Type::def_ident_ast: {
            // TODO: iron out def_ident_ast
            assert(false);
        } break;
        case AST_Type::function_type_ast: {
            if(rhs_type->type != AST_Type::function_type_ast)
            {
                report_error("Function type vs non-function type", lhs_type, rhs_type);
                result.ok = false;
                return result;
            }
            
            Function_Type_AST *lhs_func = static_cast<Function_Type_AST*>(lhs_type);
            Function_Type_AST *rhs_func = static_cast<Function_Type_AST*>(rhs_type);
            
            if(lhs_func->parameter_types.count != rhs_func->parameter_types.count)
            {
                report_error("Function types do not have same number of parameters", lhs_type, rhs_type);
                result.ok = false;
                return result;
            }
            if(lhs_func->return_types.count != rhs_func->return_types.count)
            {
                report_error("Function types do not have same number of return values", lhs_type, rhs_type);
                result.ok = false;
                return result;
            }
            
            Typecheck_Result r2;
            
            for(u64 i = 0; i < lhs_func->parameter_types.count; ++i)
            {
                r2 = types_match(lhs_func->parameter_types[i], rhs_func->parameter_types[i]);
                result = combine_typecheck_result(result, r2);
                if(!result.ok || result.waiting)
                {
                    return result;
                }
            }
            for(u64 i = 0; i < lhs_func->return_types.count; ++i)
            {
                r2 = types_match(lhs_func->return_types[i], rhs_func->return_types[i]);
                result = combine_typecheck_result(result, r2);
                if(!result.ok || result.waiting)
                {
                    return result;
                }
            }
        } break;
        case AST_Type::function_ast: {
            result.ok = false;
            return result;
        } break;
        case AST_Type::function_call_ast:
        case AST_Type::binary_operator_ast: {
            // TODO: these could be ok in the future ?
            result.ok = false;
            return result;
        } break;
        case AST_Type::enum_ast: {
            // Note: enums are unique (could change?)
            if(lhs_type != rhs_type)
            {
                report_error("Enums not equal", lhs_type, rhs_type);
                result.ok = false;
                return result;
            }
        } break;
        case AST_Type::struct_ast: {
            // Note: structs are unique
            if(lhs_type != rhs_type)
            {
                report_error("Structs not equal", lhs_type, rhs_type);
                result.ok = false;
                return result;
            }
        } break;
        case AST_Type::unary_ast: {
            Unary_Operator_AST *lhs_unary = static_cast<Unary_Operator_AST*>(lhs_type);
            if(rhs_type->type != AST_Type::unary_ast)
            {
                report_error("Pointer vs non-pointer type", lhs_type, rhs_type);
                result.ok = false;
                return result;
            }
            Unary_Operator_AST *rhs_unary = static_cast<Unary_Operator_AST*>(rhs_type);
            
            // Assert since resolved_type is type_t_ast
            assert(lhs_unary->op == Unary_Operator::ref);
            assert(rhs_unary->op == Unary_Operator::ref);
            
            return types_match(lhs_unary->operand, rhs_unary->operand);
        } break;
        case AST_Type::primitive_ast: {
            Primitive_AST *lhs_prim = static_cast<Primitive_AST*>(lhs_type);
            Primitive_AST *rhs_prim = nullptr;
            result = get_as_primitive_type(rhs_type, &rhs_prim);
            if(!result.ok || result.waiting)
            {
                return result;
            }
            
            u64 lhs_prim_flags = lhs_prim->primitive;
            u64 rhs_prim_flags = rhs_prim->primitive;
            
            u64 lhs_size = lhs_prim_flags & PRIM_SIZE_MASK;
            u64 rhs_size = rhs_prim_flags & PRIM_SIZE_MASK;
            
            if((lhs_prim_flags & PRIM_FLOATLIKE) && (rhs_prim_flags & PRIM_FLOATLIKE))
            {
                if(lhs_size != rhs_size && lhs_size != 0 && rhs_size != 0)
                {
                    if(rhs_size > lhs_size)
                    {
                        report_error("Float size mismatch", lhs_type, rhs_type);
                        result.ok = false;
                    }
                }
            }
            else if((lhs_prim_flags & PRIM_INTLIKE) && (lhs_prim_flags & PRIM_INTLIKE))
            {
                if(lhs_size != rhs_size && lhs_size != 0 && rhs_size != 0)
                {
                    if(rhs_size > lhs_size)
                    {
                        report_error("Integer size mismatch", lhs_type, rhs_type);
                        result.ok = false;
                    }
                }
            }
            else if((lhs_prim_flags & PRIM_BOOLLIKE) &&
                    (rhs_prim_flags & PRIM_BOOLLIKE))
            {
                // OK
            }
            else if(lhs_prim_flags == PRIM_VOID && rhs_prim_flags == PRIM_VOID)
            {
                // OK
            }
            else if(lhs_prim_flags == PRIM_TYPE && rhs_prim_flags == PRIM_TYPE)
            {
                // OK
            }
            else
            {
                report_error("Primitive type mismatch", lhs_type, rhs_type);
                result.ok = false;
            }
        } break;
        default: {
            assert(false);
        } break;
    }
    
    return result;
}


// Note: success means 'ast' and all children have resolved types
// Must check resolved_type to check if just 'ast' does, but not the children

// TODO: what exactly should the pre-condition and post-condition be for this function?

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
        // Statements:
        //
        case AST_Type::decl_ast: {
            Decl_AST *decl_ast = static_cast<Decl_AST*>(ast);
            assert(type_to_match == nullptr);
            
            Expr_AST *decl_type_to_match = nullptr;
            if(decl_ast->decl_type)
            {
                r1 = infer_types(ctx, &type_t_ast, decl_ast->decl_type);
                if(!r1.ok)
                {
                    return r1;
                }
                decl_type_to_match = decl_ast->decl_type;
                decl_ast->ident.resolved_type = decl_type_to_match;
            }
            
            if(decl_ast->expr)
            {
                r2 = infer_types(ctx, decl_type_to_match, decl_ast->expr);
                r1 = combine_typecheck_result(r1, r2);
                if(decl_ast->expr->resolved_type)
                {
                    decl_ast->ident.resolved_type = decl_ast->expr->resolved_type;
                }
            }
            return r1;
        } break;
        case AST_Type::block_ast: {
            Block_AST *block_ast = static_cast<Block_AST*>(ast);
            assert(type_to_match == nullptr);
            
            for(u64 i = 0; i < block_ast->statements.count; ++i)
            {
                r2 = infer_types(ctx, nullptr, block_ast->statements[i]);
                r1 = combine_typecheck_result(r1, r2);
                if(!r1.ok)
                {
                    return r1;
                }
            }
            return r1;
        } break;
        case AST_Type::while_ast: {
            While_AST *while_ast = static_cast<While_AST*>(ast);
            assert(type_to_match == nullptr);
            
            r1 = infer_types(ctx, &boollike_t_ast, while_ast->guard);
            if(!r1.ok)
            {
                return r1;
            }
            r2 = infer_types(ctx, nullptr, while_ast->body);
            r1 = combine_typecheck_result(r1, r2);
            return r1;
        } break;
        case AST_Type::for_ast: {
            For_AST *for_ast = static_cast<For_AST*>(ast);
            assert(type_to_match == nullptr);
            
            if(for_ast->flags & FOR_FLAG_OVER_ARRAY)
            {
                // TODO: once built-in arrays are a thing, check this
                report_error("For loop over array not yet supported at type level", for_ast);
                r1.ok = false;
                return r1;
            }
            else
            {
                r1 = infer_types(ctx, &intlike_t_ast, for_ast->low_expr);
                if(!r1.ok)
                {
                    return r1;
                }
                r2 = infer_types(ctx, &intlike_t_ast, for_ast->high_expr);
                r1 = combine_typecheck_result(r1, r2);
                
                // TODO: find best type? just signed/unsigned for now?
                for_ast->induction_var->resolved_type = &s64_t_ast;
            }
            if(!r1.ok)
            {
                return r1;
            }
            
            r2 = infer_types(ctx, nullptr, for_ast->body);
            r1 = combine_typecheck_result(r1, r2);
            return r1;
        } break;
        case AST_Type::if_ast: {
            If_AST *if_ast = static_cast<If_AST*>(ast);
            assert(type_to_match == nullptr);
            
            r1 = infer_types(ctx, &boollike_t_ast, if_ast->guard);
            if(!r1.ok)
            {
                return r1;
            }
            r2 = infer_types(ctx, nullptr, if_ast->then_block);
            r1 = combine_typecheck_result(r1, r2);
            if(!r1.ok)
            {
                return r1;
            }
            
            if(if_ast->else_block)
            {
                r2 = infer_types(ctx, nullptr, if_ast->else_block);
                r1 = combine_typecheck_result(r1, r2);
            }
            return r1;
        } break;
        case AST_Type::assign_ast: {
            Assign_AST *assign_ast = static_cast<Assign_AST*>(ast);
            assert(type_to_match == nullptr);
            
            bool was_resolved = assign_ast->ident.resolved_type;
            r1 = infer_types(ctx, nullptr, &assign_ast->ident);
            if(!r1.ok)
            {
                return r1;
            }
            
            Expr_AST *ident_type = assign_ast->ident.resolved_type;
            if(!ident_type)
            {
                r1.waiting = true;
                return r1;
            }
            if(!was_resolved)
            {
                r1.progress = true;
            }
            
            switch(assign_ast->assign_type)
            {
                case Assign_Operator::equal: {
                    r2 = infer_types(ctx, ident_type, assign_ast->rhs);
                    r1 = combine_typecheck_result(r1, r2);
                    return r1;
                } break;
                
                // TODO:
                case Assign_Operator::mul_eq:
                case Assign_Operator::add_eq:
                case Assign_Operator::sub_eq:
                case Assign_Operator::div_eq: {
                    report_error("*=, +=, -=, /= currently unsupported by the typechecking system", ast);
                    r1.ok = false;
                    return r1;
                } break;
            }
        } break;
        case AST_Type::return_ast: {
            Return_AST *return_ast = static_cast<Return_AST*>(ast);
            assert(type_to_match == nullptr);
            
            // Note: the return statement takes place inside the function, so don't need to recurse
            Expr_AST *return_type = return_ast->function->prototype->return_types[0];
            r1 = infer_types(ctx, return_type, return_ast->expr);
            return r1;
        } break;
        
        // Expressions:
        //
        case AST_Type::function_type_ast: {
            Function_Type_AST *function_type_ast = static_cast<Function_Type_AST*>(ast);
            
            if(type_to_match)
            {
                r1 = types_match(type_to_match, &type_t_ast);
                if(!r1.ok)
                {
                    return r1;
                }
            }
            function_type_ast->resolved_type = &type_t_ast;
            
            for(u64 i = 0; i < function_type_ast->parameter_types.count; ++i)
            {
                if(function_type_ast->parameter_types[i])
                {
                    r2 = infer_types(ctx, &type_t_ast, function_type_ast->parameter_types[i]);
                    r1 = combine_typecheck_result(r1, r2);
                    if(!r1.ok)
                    {
                        return r1;
                    }
                }
            }
            for(u64 i = 0; i < function_type_ast->return_types.count; ++i)
            {
                if(function_type_ast->return_types[i])
                {
                    r2 = infer_types(ctx, &type_t_ast, function_type_ast->return_types[i]);
                    r1 = combine_typecheck_result(r1, r2);
                    if(!r1.ok)
                    {
                        return r1;
                    }
                }
            }
            return r1;
        } break;
        case AST_Type::function_ast: {
            Function_AST *function_ast = static_cast<Function_AST*>(ast);
            
            function_ast->resolved_type = function_ast->prototype;
            r1 = infer_types(ctx, &type_t_ast, function_ast->prototype);
            if(!r1.ok)
            {
                return r1;
            }
            
            for(u64 i = 0; i < function_ast->param_names.count; ++i)
            {
                if(function_ast->prototype->parameter_types[i] && function_ast->param_names[i])
                {
                    function_ast->param_names[i]->resolved_type = function_ast->prototype->parameter_types[i];
                }
            }
            for(u64 i = 0; i < function_ast->default_values.count; ++i)
            {
                if(function_ast->default_values[i])
                {
                    r2 = infer_types(ctx, function_ast->param_names[i]->resolved_type, function_ast->default_values[i]);
                    r1 = combine_typecheck_result(r1, r2);
                    if(function_ast->default_values[i]->resolved_type)
                    {
                        if(function_ast->param_names[i])
                        {
                            function_ast->param_names[i]->resolved_type = function_ast->default_values[i]->resolved_type;
                        }
                        // Maybe two different arrays? (given, resolved parameter types)
                        function_ast->prototype->parameter_types[i] = function_ast->default_values[i]->resolved_type;
                    }
                    if(!r1.ok)
                    {
                        return r1;
                    }
                }
            }
            r2 = infer_types(ctx, nullptr, function_ast->block);
            r1 = combine_typecheck_result(r1, r2);
            return r1;
        } break;
        case AST_Type::function_call_ast: {
            Function_Call_AST *function_call_ast = static_cast<Function_Call_AST*>(ast);
            
            // Note: no overloading based on return type
            bool was_resolved = function_call_ast->function->resolved_type;
            r1 = infer_types(ctx, nullptr, function_call_ast->function);
            if(!r1.ok)
            {
                return r1;
            }
            if(!function_call_ast->function->resolved_type)
            {
                r1.waiting = true;
                return r1;
            }
            if(!was_resolved)
            {
                r1.progress = true;
            }
            
            Expr_AST *type = function_call_ast->function->resolved_type;
            Function_Type_AST *function_type = nullptr;
            r2 = get_as_function_type(type, &function_type);
            r1 = combine_typecheck_result(r1, r2);
            if(!r1.ok || r2.waiting)
            {
                return r1;
            }
            
            if(!function_call_ast->resolved_type)
            {
                r1.progress = true;
                function_call_ast->resolved_type = function_type->return_types[0];
            }
            for(u64 i = 0; i < function_call_ast->args.count; ++i)
            {
                if(function_type->parameter_types[i])
                {
                    // Note: does this need a progress check?
                    r2 = infer_types(ctx, function_type->parameter_types[i], function_call_ast->args[i]);
                    r1 = combine_typecheck_result(r1, r2);
                    if(!r1.ok)
                    {
                        return r1;
                    }
                }
                else
                {
                    r1.waiting = true;
                }
            }
            return r1;
        } break;
        case AST_Type::binary_operator_ast: {
            Binary_Operator_AST *binary_operator_ast = static_cast<Binary_Operator_AST*>(ast);
            
            switch(binary_operator_ast->op)
            {
                case Binary_Operator::access: {
                    // TODO
                    report_error("Access not yet supported", ast);
                    r1.ok = false;
                    return r1;
                } break;
                case Binary_Operator::cmp_eq:
                case Binary_Operator::cmp_neq:
                case Binary_Operator::cmp_lt:
                case Binary_Operator::cmp_le:
                case Binary_Operator::cmp_gt:
                case Binary_Operator::cmp_ge: {
                    Expr_AST *type;
                    if(type_to_match)
                    {
                        // TODO: get more specific type, if necessary
                        r1 = types_match(&boollike_t_ast, type_to_match);
                        if(!r1.ok || r1.waiting)
                        {
                            return r1;
                        }
                        type = type_to_match;
                    }
                    else
                    {
                        type = &bool8_t_ast;
                    }
                    if(!binary_operator_ast->resolved_type)
                    {
                        r1.progress = true; // Is this needed ?
                    }
                    binary_operator_ast->resolved_type = type;
                    
                    r2 = infer_types(ctx, &numberlike_t_ast, binary_operator_ast->lhs);
                    r1 = combine_typecheck_result(r1, r2);
                    if(!r1.ok)
                    {
                        return r1;
                    }
                    
                    r2 = infer_types(ctx, &numberlike_t_ast, binary_operator_ast->rhs);
                    r1 = combine_typecheck_result(r1,r2);
                    
                    if(binary_operator_ast->lhs->resolved_type && binary_operator_ast->rhs->resolved_type)
                    {
                        r2 = types_match(binary_operator_ast->lhs->resolved_type,
                                         binary_operator_ast->rhs->resolved_type);
                        r1 = combine_typecheck_result(r1, r2);
                    }
                    return r1;
                } break;
                case Binary_Operator::add:
                case Binary_Operator::sub:
                case Binary_Operator::mul:
                case Binary_Operator::div: {
                    Expr_AST *number_type;
                    
                    if(type_to_match)
                    {
                        r1 = types_match(&numberlike_t_ast, type_to_match);
                        if(!r1.ok || r1.waiting)
                        {
                            return r1;
                        }
                        number_type = type_to_match;
                    }
                    else
                    {
                        number_type = &numberlike_t_ast;
                    }
                    r2 = infer_types(ctx, number_type, binary_operator_ast->lhs);
                    r1 = combine_typecheck_result(r1, r2);
                    if(!r1.ok)
                    {
                        return r1;
                    }
                    
                    r2 = infer_types(ctx, number_type, binary_operator_ast->rhs);
                    r1 = combine_typecheck_result(r1, r2);
                    if(!r1.ok)
                    {
                        return r1;
                    }
                    
                    Expr_AST *t1 = binary_operator_ast->lhs->resolved_type;
                    Expr_AST *t2 = binary_operator_ast->rhs->resolved_type;
                    
                    if(t1 && t2)
                    {
                        Primitive_AST *combined = nullptr;
                        r2 = combine_number_types(t1, t2, &combined);
                        r1 = combine_typecheck_result(r1, r2);
                        if(!r1.ok || r2.waiting)
                        {
                            return r1;
                        }
                        
                        binary_operator_ast->resolved_type = combined;
                        if(type_to_match)
                        {
                            r2 = types_match(type_to_match, binary_operator_ast->resolved_type);
                            r1 = combine_typecheck_result(r1, r2);
                        }
                    }
                    return r1;
                } break;
                case Binary_Operator::subscript: {
                    // TODO: I'm sure this is a memory leak under most circumstances
                    Unary_Operator_AST *type_to_deref = construct_ast(ctx->ast_pool, Unary_Operator_AST, 0, 0);
                    type_to_deref->flags |= AST_FLAG_SYNTHETIC;
                    type_to_deref->resolved_type = &type_t_ast;
                    type_to_deref->op = Unary_Operator::ref;
                    
                    if(type_to_match)
                    {
                        type_to_deref->operand = type_to_match;
                        r1 = infer_types(ctx, type_to_deref, binary_operator_ast->lhs);
                    }
                    else
                    {
                        // Note: maybe change to pointer to anything instead of anything?
                        r1 = infer_types(ctx, nullptr, binary_operator_ast->lhs);
                        type_to_deref->operand = binary_operator_ast->lhs->resolved_type;
                    }
                    
                    if(r1.ok && !r1.waiting)
                    {
                        binary_operator_ast->resolved_type = type_to_deref->operand;
                    }
                    
                    r2 = infer_types(ctx, &numberlike_t_ast, binary_operator_ast->rhs);
                    r1 = combine_typecheck_result(r1, r2);
                    return r1;
                } break;
                case Binary_Operator::lor:
                case Binary_Operator::land: {
                    Expr_AST *type;
                    if(type_to_match)
                    {
                        r1 = types_match(&boollike_t_ast, type_to_match);
                        if(!r1.ok || r1.waiting)
                        {
                            return r1;
                        }
                        type = type_to_match;
                    }
                    else
                    {
                        type = &bool8_t_ast;
                    }
                    
                    r2 = infer_types(ctx, &boollike_t_ast, binary_operator_ast->lhs);
                    r1 = combine_typecheck_result(r1, r2);
                    if(!r1.ok)
                    {
                        return r1;
                    }
                    
                    r2 = infer_types(ctx, &boollike_t_ast, binary_operator_ast->rhs);
                    r1 = combine_typecheck_result(r1, r2);
                    
                    if(r1.ok && !r1.waiting)
                    {
                        binary_operator_ast->resolved_type = type;
                    }
                    return r1;
                } break;
            }
            
            r1.ok = false;
            return r1;
        } break;
        case AST_Type::number_ast: {
            Number_AST *number_ast = static_cast<Number_AST*>(ast);
            
            if(number_ast->flags & NUMBER_FLAG_FLOATLIKE)
            {
                // TODO: add warning when literal can't be represented well
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
                    
                    if(!(primitive & PRIM_FLOATLIKE))
                    {
                        report_error("Type mismatch: got float", ast);
                        r1.ok = false;
                        return r1;
                    }
                    
                    u64 size = primitive & PRIM_SIZE_MASK;
                    if(size == PRIM_SIZE4)
                    {
                        number_ast->resolved_type = &f32_t_ast;
                    }
                    else if(size == PRIM_SIZE8 || size == PRIM_NO_SIZE)
                    {
                        number_ast->resolved_type = &f64_t_ast;
                    }
                    else
                    {
                        report_error("Type mismatch: got float", ast);
                        r1.ok = false;
                    }
                }
                else
                {
                    number_ast->resolved_type = &f64_t_ast;
                }
            }
            else
            {
                // Number is int-like
                // TODO: add errors when literal is too large
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
                    
                    if(!(primitive & PRIM_NUMBERLIKE))
                    {
                        report_error("Type mismatch: got number", ast);
                        r1.ok = false;
                        return r1;
                    }
                    
                    u64 size = primitive & PRIM_SIZE_MASK;
                    
                    if(primitive & PRIM_INTLIKE)
                    {
                        // Note: PRIM_FLOATLIKE could also be set, but prefer integer
                        u64 sign = primitive & PRIM_SIGN_MASK;
                        if(sign == PRIM_UNSIGNED_INT)
                        {
                            switch(size)
                            {
                                case PRIM_SIZE1: {
                                    number_ast->resolved_type = &u8_t_ast;
                                } break;
                                case PRIM_SIZE2: {
                                    number_ast->resolved_type = &u16_t_ast;
                                } break;
                                case PRIM_SIZE4: {
                                    number_ast->resolved_type = &u32_t_ast;
                                } break;
                                case PRIM_NO_SIZE:
                                case PRIM_SIZE8: {
                                    number_ast->resolved_type = &u64_t_ast;
                                } break;
                                default: {
                                    assert(false);
                                } break;
                            }
                        }
                        else
                        {
                            // Note: if signed, or not specified
                            switch(size)
                            {
                                case PRIM_SIZE1: {
                                    number_ast->resolved_type = &s8_t_ast;
                                } break;
                                case PRIM_SIZE2: {
                                    number_ast->resolved_type = &s16_t_ast;
                                } break;
                                case PRIM_SIZE4: {
                                    number_ast->resolved_type = &s32_t_ast;
                                } break;
                                case PRIM_NO_SIZE:
                                case PRIM_SIZE8: {
                                    number_ast->resolved_type = &s64_t_ast;
                                } break;
                                default: {
                                    assert(false);
                                } break;
                            }
                        }
                    }
                    else
                    {
                        switch(size)
                        {
                            case PRIM_SIZE4: {
                                number_ast->resolved_type = &f32_t_ast;
                            } break;
                            case PRIM_NO_SIZE:
                            case PRIM_SIZE8: {
                                number_ast->resolved_type = &f64_t_ast;
                            } break;
                            default: {
                                // Note: no floatlike type should have a different size
                                assert(false);
                            } break;
                        }
                    }
                }
                else
                {
                    number_ast->resolved_type = &s64_t_ast;
                }
            }
            return r1;
        } break;
        case AST_Type::enum_ast: {
            Enum_AST *enum_ast = static_cast<Enum_AST*>(ast);
            enum_ast->resolved_type = &type_t_ast;
            
            for(u64 i = 0; i < enum_ast->values.count; ++i)
            {
                r2 = infer_types(ctx, nullptr, enum_ast->values[i]);
                r1 = combine_typecheck_result(r1, r2);
            }
            return r1;
        } break;
        case AST_Type::struct_ast: {
            Struct_AST *struct_ast = static_cast<Struct_AST*>(ast);
            struct_ast->resolved_type = &type_t_ast;
            
            for(u64 i = 0; i < struct_ast->constants.count; ++i)
            {
                r2 = infer_types(ctx, nullptr, struct_ast->constants[i]);
                r1 = combine_typecheck_result(r1, r2);
            }
            for(u64 i = 0; i < struct_ast->fields.count; ++i)
            {
                r2 = infer_types(ctx, nullptr, struct_ast->fields[i]);
                r1 = combine_typecheck_result(r1, r2);
            }
            return r1;
        } break;
        case AST_Type::unary_ast: {
            Unary_Operator_AST *unary_operator_ast = static_cast<Unary_Operator_AST*>(ast);
            
            switch(unary_operator_ast->op)
            {
                case Unary_Operator::plus:
                case Unary_Operator::minus: {
                    Expr_AST *number_type;
                    if(type_to_match)
                    {
                        r1 = types_match(&numberlike_t_ast, type_to_match);
                        if(!r1.ok || r1.waiting)
                        {
                            return r1;
                        }
                        else
                        {
                            number_type = type_to_match;
                        }
                    }
                    else
                    {
                        number_type = &numberlike_t_ast;
                    }
                    
                    r2 = infer_types(ctx, number_type, unary_operator_ast->operand);
                    r1 = combine_typecheck_result(r1, r2);
                    if(unary_operator_ast->operand->resolved_type)
                    {
                        unary_operator_ast->resolved_type = unary_operator_ast->operand->resolved_type;
                    }
                    return r1;
                } break;
                case Unary_Operator::deref: {
                    // TODO: probably a memory leak in a lot of situations
                    Unary_Operator_AST *type_to_deref = construct_ast(ctx->ast_pool, Unary_Operator_AST, 0, 0);
                    type_to_deref->flags |= AST_FLAG_SYNTHETIC;
                    type_to_deref->resolved_type = &type_t_ast;
                    type_to_deref->op = Unary_Operator::ref;
                    
                    if(type_to_match)
                    {
                        type_to_deref->operand = type_to_match;
                        r1 = infer_types(ctx, type_to_deref, unary_operator_ast->operand);
                    }
                    else
                    {
                        r1 = infer_types(ctx, nullptr, unary_operator_ast->operand);
                        type_to_deref->operand = unary_operator_ast->operand->resolved_type;
                    }
                    
                    if(r1.ok && !r1.waiting)
                    {
                        unary_operator_ast->resolved_type = type_to_deref->operand;
                    }
                    return r1;
                } break;
                case Unary_Operator::ref: {
                    // TODO: how to check for l-values?
                    Expr_AST *inner_expr = unary_operator_ast->operand;
                    
                    
                    if(type_to_match)
                    {
                        // The type of &(a type) is a type
                        if(type_to_match == &type_t_ast)
                        {
                            r2 = infer_types(ctx, &type_t_ast, inner_expr);
                            r1 = combine_typecheck_result(r1, r2);
                            if(!r1.ok)
                            {
                                return r1;
                            }
                            if(inner_expr->resolved_type)
                            {
                                assert(inner_expr->resolved_type == &type_t_ast);
                                unary_operator_ast->resolved_type = &type_t_ast;
                            }
                        }
                        else
                        {
                            r1 = check_expr_is_lvalue(inner_expr);
                            if(!r1.ok)
                            {
                                return r1;
                            }
                            
                            Unary_Operator_AST *pointer_type;
                            r2 = get_as_pointer_type(type_to_match, &pointer_type);
                            r1 = combine_typecheck_result(r1, r2);
                            if(!r1.ok || r2.waiting)
                            {
                                return r1;
                            }
                            assert(pointer_type->op == Unary_Operator::ref);
                            
                            Expr_AST *inner_type = pointer_type->operand;
                            r2 = infer_types(ctx, inner_type, inner_expr);
                            r1 = combine_typecheck_result(r1, r2);
                            
                            unary_operator_ast->resolved_type = pointer_type;
                        }
                    }
                    else
                    {
                        r2 = infer_types(ctx, nullptr, inner_expr);
                        r1 = combine_typecheck_result(r1, r2);
                        if(!r1.ok)
                        {
                            return r1;
                        }
                        
                        if(inner_expr->resolved_type)
                        {
                            if(inner_expr->resolved_type == &type_t_ast)
                            {
                                unary_operator_ast->resolved_type = &type_t_ast;
                            }
                            else if(!unary_operator_ast->resolved_type)
                            {
                                Unary_Operator_AST *pointer_type = construct_ast(ctx->ast_pool, Unary_Operator_AST, 0, 0);
                                pointer_type->flags |= AST_FLAG_SYNTHETIC;
                                pointer_type->resolved_type = &type_t_ast;
                                pointer_type->op = Unary_Operator::ref;
                                pointer_type->operand = inner_expr->resolved_type;
                                
                                unary_operator_ast->resolved_type = pointer_type;
                            }
                        }
                    }
                    return r1;
                } break;
                case Unary_Operator::lnot: {
                    Expr_AST *bool_type;
                    
                    if(type_to_match)
                    {
                        r1 = types_match(&boollike_t_ast, type_to_match);
                        if(!r1.ok || r1.waiting)
                        {
                            return r1;
                        }
                        else
                        {
                            bool_type = type_to_match;
                        }
                    }
                    else
                    {
                        bool_type = &bool8_t_ast;
                    }
                    
                    r2 = infer_types(ctx, bool_type, unary_operator_ast->operand);
                    r1 = combine_typecheck_result(r1, r2);
                    if(unary_operator_ast->operand->resolved_type)
                    {
                        unary_operator_ast->resolved_type = unary_operator_ast->operand->resolved_type;
                    }
                    return r1;
                } break;
            }
        } break;
        
        case AST_Type::refer_ident_ast: {
            Refer_Ident_AST *ident_ast = static_cast<Refer_Ident_AST*>(ast);
            assert(ident_ast->referred_to);
            
            if(ident_ast->resolved_type)
            {
                return r1;
            }
            else if(ident_ast->referred_to->resolved_type)
            {
                ident_ast->resolved_type = ident_ast->referred_to->resolved_type;
                r1.progress = true;
            }
            else
            {
                r1.waiting = true;
            }
            return r1;
        } break;
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
        
        case AST_Type::def_ident_ast: {
            assert(false);
            return r1;
        } break;
        
        case AST_Type::primitive_ast: {
            Primitive_AST *primitive_ast = static_cast<Primitive_AST*>(ast);
            primitive_ast->resolved_type = &type_t_ast;
            if(type_to_match)
            {
                r1 = types_match(type_to_match, &type_t_ast);
            }
            return r1;
        } break;
        
        // TODO: for now, the string type is not built-in
        case AST_Type::string_ast: {
            report_error("Strings types are currently unsupported", ast);
            r1.ok = false;
            return r1;
        } break;
        default: {
            assert(false);
            return r1;
        } break;
    }
    
    r1.ok = false;
    return r1;
}

void infer_all(Pool_Allocator *ast_pool, Array<Decl_AST*> decls)
{
    Infer_Context ctx;
    ctx.ast_pool = ast_pool;
    
    Typecheck_Result r1 = typecheck_result(true,false,false);
    Typecheck_Result r2 = r1;
    
    for(u64 i = 0; i < decls.count; ++i)
    {
        r2 = infer_types(&ctx, nullptr, decls[i]);
        r1 = combine_typecheck_result(r1, r2);
        if(!r1.ok)
        {
            return;
        }
    }
    
    r1.progress = true;
    while(r1.progress)
    {
        r1.waiting = false;
        r1.progress = false;
        
        for(u64 i = 0; i < decls.count; ++i)
        {
            r2 = infer_types(&ctx, nullptr, decls[i]);
            r1 = combine_typecheck_result(r1, r2);
            if(!r1.ok)
            {
                return;
            }
        }
    }
    
    if(r1.waiting)
    {
#if 1
        r1.waiting = false;
        r1.progress = false;
        // Just for debugging typechecking for now
        for(u64 i = 0; i < decls.count; ++i)
        {
            r2 = infer_types(&ctx, nullptr, decls[i]);
            r1 = combine_typecheck_result(r1, r2);
            assert(r1.ok);
            assert(!r1.waiting);
        }
#endif
        
        print_err("Still waiting on types\n");
        return;
    }
}


/*
struct Verify_Context
{
Pool_Allocator *ast_pool;
Dynamic_Array<AST*> *waiting_asts;
};

void verify_all(Array<Decl_AST*> decls)
{
}
*/




/*
bool typecheck_all(Array<Decl_AST*> decls)
{
Dynamic_Array<AST*> arrays[2];

for(u64 i = 0; i < decls.count; ++i)
{
typecheck_ast(decls[i], &arrays[0]);
}

Dynamic_Array<AST*> *checking = &arrays[0];
Dynamic_Array<AST*> *to_check = &arrays[1];

// Keep checking while progress is made
while(true)
{
// Clear to_check
to_check->count = 0;

// Check all in checking
for(u64 i = 0; i < checking->count; ++i)
{
typecheck_ast(*checking[i], to_check);
}

// Note: this equality check requires that elements are added to to_check in the same order they are visited in.
// This is *currently* the case if we are at a typechecking dead-end
// Of course, the condition is also fulfilled if both are empty
if(*checking == *to_check)
{
break;
}

// Swap arrays
Dynamic_Array<AST*> *tmp = checking;
checking = to_check;
to_check = tmp;
}

if(count != 0)
{
// TODO: search for the problem
print_err("Unable to finish typechecking the following:\n");
print_dot(&stderr_buf, to_check->array);
return false;
}
else
{
return true;
}
}
*/

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
    
    link_all(decls.array);
    infer_all(&ast_pool, decls.array);
    
    // TODO pass up errors
    // print_dot(&stdout_buf, decls.array);
    
    return 0;
}
