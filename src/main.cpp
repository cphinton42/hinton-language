

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

struct Typecheck_Job
{
    Expr_AST *type_to_match;
    AST *ast_to_check;
};

/*
bool match_type(AST *t1, AST *t2)
{
    return false;
}

AST *resolve_to_type(AST *expr)
{
    return nullptr;
}
*/

enum class Typecheck_Result
{
    failure,
    yield,
    success,
};

internal
bool expr_is_lvalue(Expr_AST *expr)
{
    // TODO:
    return false;
}

internal inline
Typecheck_Result combine_typecheck_result(Typecheck_Result r1, Typecheck_Result r2)
{
    if(r1 == Typecheck_Result::failure || r2 == Typecheck_Result::failure)
    {
        return Typecheck_Result::failure;
    }
    if(r1 == Typecheck_Result::yield || r2 == Typecheck_Result::yield)
    {
        return Typecheck_Result::yield;
    }
    return Typecheck_Result::success;
}

Typecheck_Result types_match(Expr_AST *t1, Expr_AST *t2)
{
    return Typecheck_Result::failure;
}

Typecheck_Result infer_and_check_type(Pool_Allocator *ast_pool, Expr_AST *type_to_match, Expr_AST *expr)
{
    if(expr->resolved_type)
    {
        if(type_to_match)
        {
            return types_match(type_to_match, expr->resolved_type);
        }
        else
        {
            return Typecheck_Result::success;
        }
    }
    
    switch(expr->type)
    {
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
        case AST_Type::enum_ast: {
        } break;
        case AST_Type::struct_ast: {
            
        } break;
        case AST_Type::unary_ast: {
            Unary_Operator_AST *unary_operator_ast = static_cast<Unary_Operator_AST*>(expr);
            
            switch(unary_operator_ast->op)
            {
                case Unary_Operator::plus:
                case Unary_Operator::minus: {
                    Expr_AST *number_type;
                    if(type_to_match)
                    {
                        Typecheck_Result match = types_match(&numberlike_t_ast, type_to_match);
                        if(match == Typecheck_Result::success)
                        {
                            number_type = type_to_match;
                        }
                        else
                        {
                            return match;
                        }
                    }
                    else
                    {
                        number_type = &numberlike_t_ast;
                    }
                    
                    
                    
                    
                } break;
                case Unary_Operator::deref: {
                    Unary_Operator_AST *type_to_deref = construct_ast(ast_pool, Unary_Operator_AST, 0, 0);
                    type_to_deref->flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPECHECKED;
                    type_to_deref->resolved_type = &type_t_ast;
                    type_to_deref->op = Unary_Operator::ref;
                    
                    Typecheck_Result result;
                    
                    if(type_to_match)
                    {
                        type_to_deref->operand = type_to_match;
                        result = infer_and_check_type(ast_pool, type_to_deref, unary_operator_ast->operand);
                    }
                    else
                    {
                        result = infer_and_check_type(ast_pool, nullptr, unary_operator_ast->operand);
                        type_to_deref->operand = type_to_match;
                    }
                    // TODO: memory leak if result is yield?
                    // Note: could allocate in temporary memory, then copy to pool if confirmed
                    if(result == Typecheck_Result::success)
                    {
                        expr->resolved_type = type_to_deref;
                    }
                    return result;
                } break;
                case Unary_Operator::ref: {
                    // TODO: how to check for l-values?
                } break;
                case Unary_Operator::lnot: {
                    Expr_AST *bool_type;
                    
                    if(type_to_match)
                    {
                        Typecheck_Result match = types_match(&boollike_t_ast, type_to_match);
                        if(match == Typecheck_Result::success)
                        {
                            bool_type = type_to_match;
                        }
                        else
                        {
                            return match;
                        }
                    }
                    else
                    {
                        bool_type = &bool8_t_ast;
                    }
                    
                    Typecheck_Result result = infer_and_check_type(ast_pool, bool_type, unary_operator_ast->operand);
                    if(result == Typecheck_Result::success)
                    {
                        expr->resolved_type = bool_type;
                    }
                    return result;
                } break;
            }
        } break;
        case AST_Type::primitive_ast: {
            expr->resolved_type = &type_t_ast;
            return Typecheck_Result::success;
        } break;
        case AST_Type::string_ast: {
            // TODO:
            print_err("Strings types are currently unsupported\n");
            return Typecheck_Result::failure;
        } break;
        case AST_Type::number_ast: {
            Number_AST *number_ast = static_cast<Number_AST*>(expr);
            
            if(number_ast->flags & NUMBER_FLAG_FLOATLIKE)
            {
                // TODO: add warning when literal can't be represented well
                if(type_to_match)
                {
                    if(type_to_match->type != AST_Type::primitive_ast)
                    {
                        print_err("Type mismatch: got float\n");
                        return Typecheck_Result::failure;
                    }
                    
                    Primitive_AST *primitive_ast = static_cast<Primitive_AST*>(type_to_match);
                    u64 primitive = primitive_ast->primitive;
                    
                    if(!(primitive & PRIM_FLOATLIKE))
                    {
                        print_err("Type mismatch: got float\n");
                        return Typecheck_Result::failure;
                    }
                    
                    u64 size = primitive & PRIM_SIZE_MASK;
                    if(size == PRIM_SIZE4)
                    {
                        expr->resolved_type = &f32_t_ast;
                        return Typecheck_Result::success;
                    }
                    else if(size == PRIM_SIZE8 || size == PRIM_NO_SIZE)
                    {
                        expr->resolved_type = &f64_t_ast;
                        return Typecheck_Result::success;
                    }
                    else
                    {
                        print_err("Type mismatch: got float\n");
                        return Typecheck_Result::failure;
                    }
                }
                else
                {
                    expr->resolved_type = &f64_t_ast;
                    return Typecheck_Result::success;
                }
            }
            else
            {
                // Number is int-like
                // TODO: add errors when literal is too large
                if(type_to_match)
                {
                    if(type_to_match->type != AST_Type::primitive_ast)
                    {
                        print_err("Type mismatch: got number\n");
                        return Typecheck_Result::failure;
                    }
                    
                    Primitive_AST *primitive_ast = static_cast<Primitive_AST*>(type_to_match);
                    u64 primitive = primitive_ast->primitive;
                    
                    if(!(primitive & PRIM_NUMBERLIKE))
                    {
                        print_err("Type mismatch: got number\n");
                        return Typecheck_Result::failure;
                    }
                    
                    bool intlike = (primitive & PRIM_INTLIKE);
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
                                    expr->resolved_type = &u8_t_ast;
                                } break;
                                case PRIM_SIZE2: {
                                    expr->resolved_type = &u16_t_ast;
                                } break;
                                case PRIM_SIZE4: {
                                    expr->resolved_type = &u32_t_ast;
                                } break;
                                case PRIM_NO_SIZE:
                                case PRIM_SIZE8: {
                                    expr->resolved_type = &u64_t_ast;
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
                                    expr->resolved_type = &s8_t_ast;
                                } break;
                                case PRIM_SIZE2: {
                                    expr->resolved_type = &s16_t_ast;
                                } break;
                                case PRIM_SIZE4: {
                                    expr->resolved_type = &s32_t_ast;
                                } break;
                                case PRIM_NO_SIZE:
                                case PRIM_SIZE8: {
                                    expr->resolved_type = &s64_t_ast;
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
                                expr->resolved_type = &f32_t_ast;
                            } break;
                            case PRIM_NO_SIZE:
                            case PRIM_SIZE8: {
                                expr->resolved_type = &f64_t_ast;
                            } break;
                            default: {
                                // Note: no floatlike type should have a different size
                                assert(false);
                            } break;
                        }
                    }
                    
                    return Typecheck_Result::success;
                }
                else
                {
                    expr->resolved_type = &s64_t_ast;
                    return Typecheck_Result::success;
                }
            }
        } break;
        case AST_Type::bool_ast: {
            Bool_AST *bool_ast = static_cast<Bool_AST*>(expr);
            
            if(type_to_match)
            {
                if(type_to_match->type != AST_Type::primitive_ast)
                {
                    print_err("Type mismatch: got bool\n");
                    return Typecheck_Result::failure;
                }
                
                Primitive_AST *primitive_ast = static_cast<Primitive_AST*>(type_to_match);
                u64 primitive = primitive_ast->primitive;
                
                if(!(primitive & PRIM_BOOLLIKE))
                {
                    print_err("Type mismatch: got bool\n");
                    return Typecheck_Result::failure;
                }
                
                u64 size = primitive & PRIM_SIZE_MASK;
                
                switch(size)
                {
                    case PRIM_NO_SIZE:
                    case PRIM_SIZE1: {
                        expr->resolved_type = &bool8_t_ast;
                    } break;
                    case PRIM_SIZE2: {
                        expr->resolved_type = &bool16_t_ast;
                    } break;
                    case PRIM_SIZE4: {
                        expr->resolved_type = &bool32_t_ast;
                    } break;
                    case PRIM_SIZE8: {
                        expr->resolved_type = &bool64_t_ast;
                    } break;
                    default: {
                        assert(false);
                    } break;
                }
                return Typecheck_Result::success;
            }
            else
            {
                expr->resolved_type = &bool8_t_ast;
                return Typecheck_Result::success;
            }
        } break;
        default: {
            assert(false);
        } break;
    }
    
    return Typecheck_Result::failure;
}

/*
Typecheck_Result typecheck_ast(Expr_AST *match_type, AST *ast)
{
    if(ast->flags & AST_FLAG_TYPECHECKED)
    {
        return Typecheck_Result::success;
    }
    Typecheck_Result result = Typecheck_Result::success;
    defer {
        if(result == Typecheck_Result::success)
        {
            ast->flags |= AST_FLAG_TYPECHECKED;
        }
    };
    
    switch(ast->type)
    {
        // Statement cases first:
        //
        case AST_Type::decl_ast: {
            Decl_AST *decl_ast = static_cast<Decl_AST*>(ast);
            assert(match_type == nullptr);
            
            Expr_AST *type_to_match = nullptr;
            
            if(decl_ast->decl_type)
            {
                result = typecheck_ast(&type_t_ast, decl_ast->decl_type);
                switch(result)
                {
                    case Typecheck_Result::yield:
                    case Typecheck_Result::failure: {
                        return result;
                    } break;
                    case Typecheck_Result::success: {
                        type_to_match = decl_ast->decl_type->resolved_type;
                    } break;
                }
            }
            if(decl_ast->expr)
            {
                result = typecheck_ast(type_to_match, decl_ast->expr, to_check);
            }
        } break;
        case AST_Type::block_ast: {
            Block_AST *block_ast = static_cast<Block_AST*>(ast);
            assert(match_type == nullptr);
            
            for(u64 i = 0; i < block_ast->statements.count; ++i)
            {
                Typecheck_Result sub_result = typecheck_ast(nullptr, block_ast->statements[i]);
                if(sub_result == Typecheck_Result::failure)
                {
                    result = sub_result;
                    return result;
                }
                else if(sub_result == Typecheck_Result::yield)
                {
                    result = sub_result;
                }
            }
        } break;
        case AST_Type::while_ast: {
            While_AST *while_ast = static_cast<While_AST*>(ast);
            assert(match_type == nullptr);
            
            result = typecheck_ast(&bool8_t_ast, while_ast->guard);
            if(result == Typecheck_Result::failure)
            {
                return result;
            }
            Typecheck_Result sub_result = typecheck_ast(nullptr, while_ast->body);
            result = combine_typecheck_result(result, sub_result);
        } break;
        case AST_Type::for_ast: {
            For_AST *for_ast = static_cast<For_AST*>(ast);
            assert(match_type == nullptr);
            
            if(for_ast->flags & FOR_FLAG_OVER_ARRAY)
            {
                // TODO: once built-in arrays are a thing, check this
                print_err("For loop over array not yet supported at type level\n");
                result = Typecheck_Result::failure;
                return result;
            }
            else
            {
                result = typecheck_ast(&intlike_t_ast, for_ast->low_expr);
                if(result == Typecheck_Result::failure)
                {
                    return result;
                }
                Typecheck_Result result2 = typecheck_ast(&intlike_t_ast, for_ast->high_expr);
                result = combine_typecheck_result(result, result2);
            }
            if(result == Typecheck_Result::failure)
            {
                return result;
            }
            
            Typecheck_Result result2 = typecheck_ast(nullptr, for_ast->body);
            result = combine_typecheck_result(result, result2);
        } break;
        case AST_Type::if_ast: {
            If_AST *if_ast = static_cast<If_AST*>(ast);
            assert(match_type == nullptr);
            
            
            result = typecheck_ast(&bool8_t_ast, if_ast->guard);
            if(result == Typecheck_Result::failure)
            {
                return result;
            }
            Typecheck_Result result2 = typecheck_ast(nullptr, if_ast->then_block);
            result = combine_typecheck_result(result,result2);
            if(result == Typecheck_Result::failure)
            {
                return result;
            }
            
            if(if_ast->else_block)
            {
                result2 = typecheck_ast(nullptr, if_ast->else_block);
                result = combine_typecheck_result(result,result2);
            }
        } break;
        case AST_Type::assign_ast: {
            Assign_AST *assign_ast = static_cast<Assign_AST*>(ast);
            assert(match_type == nullptr);
            
            Expr_AST *ident_type;
            
            result = typecheck_ast(nullptr, &assign_ast->ident);
            switch(result)
            {
                case Typecheck_Result::yield:
                case Typecheck_Result::failure: {
                    return result;
                } break;
                case Typecheck_Result::success: {
                    ident_type = assign_ast->ident.resolved_type;
                } break;
            }
            
            switch(assign_ast->assign_type)
            {
                case Assign_Operator::equal: {
                    result = typecheck_ast(ident_type, assign_ast->rhs);
                    return result;
                } break;
                
                // TODO:
                case Assign_Operator::mul_eq:
                case Assign_Operator::add_eq:
                case Assign_Operator::sub_eq:
                case Assign_Operator::div_eq: {
                    print_err("*=, +=, -=, /= currently unsupported by the typechecking system\n");
                    result = Typecheck_Result::failure;
                    return result;
                } break;
            }
        } break;
        case AST_Type::return_ast: {
            Return_AST *return_ast = static_cast<Return_AST*>(ast);
            assert(match_type == nullptr);
            
            // TODO: typechecking will probably get into loops at this rate.
            // Either need to just check AST_FLAG_TYPECHECKED in some cases, or use a visited flag
            
            Expr_AST *return_type = return_ast->function->prototype->return_types[0];
            result = typecheck_ast(&type_t_ast, return_type);
            switch(result)
            {
                case Typecheck_Result::yield: {
                    array_add(to_check, {nullptr, return_ast});
                } // fall through
                case Typecheck_Result::failure: {
                    return result;
                } break;
                case Typecheck_Result::success: {
                    return typecheck_ast(return_type, return_ast->expr, to_check);
                } break;
            }
        } break;
        // Expression cases:
        //
        case AST_Type::function_type_ast: {
            Function_Type_AST *function_type_ast = static_cast<Function_Type_AST*>(ast);
            
            if(match_type && match_type != &type_t_ast)
            {
                // TODO: better error message
                print_err("Expected type\n");
                return Typecheck_Result::failure;
            }
            
            Typecheck_Result result = Typecheck_Result::success;
            for(u64 i = 0; i < function_type_ast->parameter_types.count; ++i)
            {
                Typecheck_Result sub_result = typecheck_ast(&type_t_ast, function_type_ast->parameter_types[i], to_check);
                result = combine_typecheck_results(result, sub_result);
                if(result == Typecheck_Result::failure)
                {
                    return result;
                }
            }
            for(u64 i = 0; i < function_type_ast->return_types.count; ++i)
            {
                Typecheck_Result sub_result = typecheck_ast(&type_t_ast, function_type_ast->return_types[i], to_check);
                result = combine_typecheck_results(result, sub_result);
                if(result == Typecheck_Result::failure)
                {
                    return result;
                }
            }
            // TODO: should this be set if result is yield? Maybe need extra flags ?
            function_type_ast->resolved_type = &type_t_ast;
            return result;
        } break;
        case AST_Type::function_ast: {
            Function_AST *function_ast = static_cast<Function_AST*>(ast);
            
            result = typecheck_ast(&type_t_ast, function_ast->prototype);
            
            switch(result)
            {
                case Typecheck_Result::failure:
                case Typecheck_Result::yield:{
                    return result;
                } break;
                case Typecheck_Result::success: {
                } break;
            }
            
            if(!types_match(match_type, function_ast->prototype))
            {
                print_err("Type mismatch\n");
                return Typecheck_Result::failure;
            }
            
            for(u64 i = 0; i < function_ast->default_values.count; ++i)
            {
                Typecheck_Result sub_result = typecheck_ast(function_ast->prototype->parameter_types[i], function_ast->default_values[i]);
                result = combine_typecheck_results(result, sub_result);
                if(result == Typecheck_Result::failure)
                {
                    return result;
                }
            }
            
            Typecheck_Result sub_result = typecheck_ast(nullptr, function_ast->block);
            
            result = combine_typecheck_results(result, sub_result);
            return result;
        } break;
        case AST_Type::function_call_ast: {
            Function_Call_AST *function_call_ast = static_cast<Function_Call_AST*>(ast);
            
            // TODO: this would cause a loop in recursive functions. Whats going on!?
            // result = typecheck_ast();
            
        } break;
        case AST_Type::binary_operator_ast:
        case AST_Type::number_ast:
        case AST_Type::enum_ast:
        case AST_Type::struct_ast:
        case AST_Type::unary_ast:
        case AST_Type::refer_ident_ast:
        case AST_Type::bool_ast: {
        
            return Typecheck_Result::failure;
        } break;
        
        case AST_Type::def_ident_ast:
        case AST_Type::primitive_ast: {
            return Typecheck_Result::success;
        } break;
        
        // TODO: for now, the string type is not built-in
        case AST_Type::string_ast:
        default: {
            assert(false);
        } break;
    }
    
    return result;
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
    
    print_dot(&stdout_buf, decls.array);
    
    return 0;
}
