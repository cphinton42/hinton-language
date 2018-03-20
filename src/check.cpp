
void report_error(const byte *msg, AST *ast)
{
    print_err("Error: %d:%d: %s\n", ast->line_number, ast->line_offset, msg);
}

void report_error(const byte *msg, AST *t1, AST *t2)
{
    print_err("Type Error: %d:%d vs %d:%d: %s\n", t1->line_number, t1->line_offset, t2->line_number, t2->line_offset, msg);
}

Job make_typecheck_job(Expr_AST *type, AST *ast, u32 stage)
{
    Job result;
    result.type = Job_Type::typecheck;
    result.stage = stage;
    result.typecheck.type = type;
    result.typecheck.ast = ast;
    return result;
}

bool do_job(Context *ctx, Job job)
{
    switch(job.type)
    {
        case Job_Type::typecheck: {
            return do_typecheck_job(ctx, job);
        } break;
    }
    assert(false);
    return false;
}


void set_resolved_type(Expr_AST *expr, Expr_AST *type)
{
    expr->types.count = 1;
    expr->resolved_type = type;
}

bool type_resolved(Expr_AST *expr)
{
    return expr->types.count == 1;
}


internal
bool typecheck_decl(Context *ctx, u32 stage, Decl_AST *ast);
internal
bool typecheck_assign(Context *ctx, u32 stage, Assign_AST *assign_ast);
internal
bool typecheck_return(Context *ctx, Return_AST *return_ast);
internal
bool typecheck_ident(Context *ctx, u32 stage, Expr_AST *type, Ident_AST *ident_ast);
internal
bool typecheck_function_type(Context *ctx, u32 stage, Expr_AST *type, Function_Type_AST *function_type_ast);
internal
bool typecheck_function(Context *ctx, u32 stage, Expr_AST *type, Function_AST *function_ast);
internal
bool typecheck_function_call(Context *ctx, u32 stage, Expr_AST *type, Function_Call_AST *function_call_ast);
internal
bool typecheck_access(Context *ctx, u32 stage, Expr_AST *type, Access_AST *access_ast);
internal
bool typecheck_binop_ast(Context *ctx, u32 stage, Expr_AST *type, Binary_Operator_AST *binop_ast);
internal
bool typecheck_number(Context *ctx, Expr_AST *type, Number_AST *number_ast);
internal
bool typecheck_enum(Context *ctx, u32 stage, Expr_AST *type, Enum_AST *enum_ast);
internal
bool typecheck_struct(Context *ctx, u32 stage, Expr_AST *type, Struct_AST *struct_ast);
internal
bool typecheck_unary(Context *ctx, u32 stage, Expr_AST *type, Unary_Operator_AST *unop_ast);
internal
bool typecheck_primitive(Context *ctx, Expr_AST *type, Primitive_AST *prim_ast);
internal 
bool typecheck_string(Context *ctx, Expr_AST *type, String_AST *str_ast);
internal
bool typecheck_bool(Context *ctx, Expr_AST *type, Bool_AST *bool_ast);

internal
Status combine_number_types(Context *ctx, Expr_AST *t1, Expr_AST *t2, Primitive_AST **out);

bool do_typecheck_job(Context *ctx, Job job)
{
    Expr_AST *type = job.typecheck.type;
    AST *ast = job.typecheck.ast;
    
    switch(ast->type)
    {
        case AST_Type::decl_ast: {
            Decl_AST *decl_ast = static_cast<Decl_AST*>(ast);
            assert(type == nullptr);
            return typecheck_decl(ctx, job.stage, decl_ast);
        } break;
        case AST_Type::block_ast: {
            Block_AST *block_ast = static_cast<Block_AST*>(ast);
            assert(type == nullptr);
            for(u64 i = 0; i < block_ast->statements.count; ++i)
            {
                auto job = make_typecheck_job(nullptr, block_ast->statements[i]);
                do_typecheck_job(ctx, job);
            }
            return true;
        } break;
        case AST_Type::while_ast: {
            While_AST *while_ast = static_cast<While_AST*>(ast);
            assert(type == nullptr);
            
            auto job = make_typecheck_job(&boollike_t_ast, while_ast->guard);
            do_typecheck_job(ctx, job);
            job = make_typecheck_job(nullptr, while_ast->body);
            do_typecheck_job(ctx, job);
            return true;
        } break;
        case AST_Type::for_ast: {
            For_AST *for_ast = static_cast<For_AST*>(ast);
            assert(type == nullptr);
            
            if(for_ast->flags & FOR_FLAG_OVER_ARRAY)
            {
                // TODO: once built-in arrays are a thing, check this
                report_error("For loop over array not yet supported at type level", for_ast);
                ctx->success = false;
            }
            else
            {
                // TODO: find best type? (change to u64 if out of range for s64?)
                // If ints can't downcast, it might be a pain for 'it' to be big
                set_resolved_type(for_ast->induction_var, &s64_t_ast);
                
                auto job = make_typecheck_job(&intlike_t_ast, for_ast->low_expr);
                do_typecheck_job(ctx, job);
                job = make_typecheck_job(&intlike_t_ast, for_ast->high_expr);
                do_typecheck_job(ctx, job);
            }
            
            auto job = make_typecheck_job(nullptr, for_ast->body);
            do_typecheck_job(ctx, job);
            return true;
        } break;
        case AST_Type::if_ast: {
            If_AST *if_ast = static_cast<If_AST*>(ast);
            assert(type == nullptr);
            
            auto job = make_typecheck_job(&boollike_t_ast, if_ast->guard);
            do_typecheck_job(ctx, job);
            job = make_typecheck_job(nullptr, if_ast->then_block);
            do_typecheck_job(ctx, job);
            if(if_ast->else_block)
            {
                job = make_typecheck_job(nullptr, if_ast->else_block);
                do_typecheck_job(ctx, job);
            }
            return true;
        } break;
        case AST_Type::assign_ast: {
            Assign_AST *assign_ast = static_cast<Assign_AST*>(ast);
            assert(type == nullptr);
            return typecheck_assign(ctx, job.stage, assign_ast);
        } break;
        case AST_Type::return_ast: {
            Return_AST *return_ast = static_cast<Return_AST*>(ast);
            assert(type == nullptr);
            assert(job.stage == 0);
            return typecheck_return(ctx, return_ast);
        } break;
        
        case AST_Type::ident_ast: {
            Ident_AST *ident_ast = static_cast<Ident_AST*>(ast);
            return typecheck_ident(ctx, job.stage, type, ident_ast);
        } break;
        case AST_Type::function_type_ast: {
            Function_Type_AST *function_type_ast = static_cast<Function_Type_AST*>(ast);
            return typecheck_function_type(ctx, job.stage, type, function_type_ast);
        } break;
        case AST_Type::function_ast: {
            Function_AST *function_ast = static_cast<Function_AST*>(ast);
            return typecheck_function(ctx, job.stage, type, function_ast);
        } break;
        case AST_Type::function_call_ast: {
            Function_Call_AST *function_call_ast = static_cast<Function_Call_AST*>(ast);
            return typecheck_function_call(ctx, job.stage, type, function_call_ast);
        } break;
        case AST_Type::access_ast: {
            Access_AST *access_ast = static_cast<Access_AST*>(ast);
            return typecheck_access(ctx, job.stage, type, access_ast);
        } break;
        case AST_Type::binary_operator_ast: {
            Binary_Operator_AST *binop_ast = static_cast<Binary_Operator_AST*>(ast);
            return typecheck_binop_ast(ctx, job.stage, type, binop_ast);
        } break;
        case AST_Type::number_ast: {
            Number_AST *number_ast = static_cast<Number_AST*>(ast);
            assert(job.stage == 0);
            return typecheck_number(ctx, type, number_ast);
        } break;
        case AST_Type::enum_ast: {
            Enum_AST *enum_ast = static_cast<Enum_AST*>(ast);
            return typecheck_enum(ctx, job.stage, type, enum_ast);
        } break;
        case AST_Type::struct_ast: {
            Struct_AST *struct_ast = static_cast<Struct_AST*>(ast);
            return typecheck_struct(ctx, job.stage, type, struct_ast);
        } break;
        case AST_Type::unary_ast: {
            Unary_Operator_AST *unop_ast = static_cast<Unary_Operator_AST*>(ast);
            return typecheck_unary(ctx, job.stage, type, unop_ast);
        } break;
        case AST_Type::primitive_ast: {
            Primitive_AST *prim_ast = static_cast<Primitive_AST*>(ast);
            assert(job.stage == 0);
            return typecheck_primitive(ctx, type, prim_ast);
        } break;
        case AST_Type::string_ast: {
            String_AST *string_ast = static_cast<String_AST*>(ast);
            assert(job.stage == 0);
            return typecheck_string(ctx, type, string_ast);
        } break;
        case AST_Type::bool_ast: {
            Bool_AST *bool_ast = static_cast<Bool_AST*>(ast);
            assert(job.stage == 0);
            return typecheck_bool(ctx, type, bool_ast);
        } break;
    }
    
    assert(false);
    return false;
}


internal
bool typecheck_decl(Context *ctx, u32 stage, Decl_AST *decl_ast)
{
    u32 new_stage = 0;
    
    switch(stage)
    {
        case 0: {
            if(decl_ast->decl_type)
            {
                auto job = make_typecheck_job(&type_t_ast, decl_ast->decl_type);
                do_typecheck_job(ctx, job);
            }
        } // fall through
        case 1: {
            new_stage = 1;
            if(decl_ast->decl_type && !type_resolved(decl_ast->decl_type))
            {
                break;
            }
            if(decl_ast->expr)
            {
                Expr_AST *type_to_match = nullptr;
                if(decl_ast->decl_type)
                {
                    assert(type_resolved(decl_ast->decl_type));
                    type_to_match = decl_ast->decl_type;
                }
                auto job = make_typecheck_job(type_to_match, decl_ast->expr);
                do_typecheck_job(ctx, job);
            }
        } // fall through
        case 2: {
            new_stage = 2;
            if(decl_ast->expr && !type_resolved(decl_ast->expr))
            {
                break;
            }
            if(decl_ast->expr)
            {
                ident_set_expr(&decl_ast->ident, decl_ast->expr);
            }
            if(decl_ast->decl_type)
            {
                set_resolved_type(&decl_ast->ident, decl_ast->decl_type);
            }
            else if(decl_ast->expr)
            {
                set_resolved_type(&decl_ast->ident, decl_ast->expr->resolved_type);
            }
            else
            {
                // TODO: this should be an enum value
            }
            new_stage = 3;
        } // fall through
    }
    
    if(new_stage < 3)
    {
        auto job = make_typecheck_job(nullptr, decl_ast, new_stage);
        array_add(ctx->waiting_jobs, job);
    }
    return new_stage != stage;
}

internal
bool typecheck_assign(Context *ctx, u32 stage, Assign_AST *assign_ast)
{
    u32 new_stage = 0;
    bool add_job = false;
    
    switch(stage)
    {
        case 0: {
            add_job = true;
        } // fall through
        case 1: {
            new_stage = 1;
            if(!type_resolved(assign_ast->lhs) && add_job)
            {
                auto job = make_typecheck_job(nullptr, assign_ast->lhs);
                do_typecheck_job(ctx, job);
            }
            if(!type_resolved(assign_ast->lhs))
            {
                break;
            }
            add_job = true;
        } // fall through
        case 2: {
            new_stage = 2;
            u16 lvalue_flags = assign_ast->lhs->flags & EXPR_FLAG_LVALUE_MASK;
            if(lvalue_flags == 0)
            {
                break;
            }
            else if(lvalue_flags == EXPR_FLAG_NOT_LVALUE)
            {
                report_error("LHS of assignment must be an lvalue", assign_ast);
                ctx->success = false;
                break;
            }
            else
            {
                assert(lvalue_flags == EXPR_FLAG_LVALUE);
            }
            add_job = true;
        } // fall through
        case 3: {
            new_stage = 3;
            
            switch(assign_ast->assign_type)
            {
                case Assign_Operator::equal: {
                    if(!type_resolved(assign_ast->rhs) && add_job)
                    {
                        auto job = make_typecheck_job(assign_ast->lhs->resolved_type, assign_ast->rhs);
                        do_typecheck_job(ctx, job);
                    }
                    if(!type_resolved(assign_ast->rhs))
                    {
                        goto finish;
                    }
                } break;
                
                // TODO:
                case Assign_Operator::mul_eq:
                case Assign_Operator::add_eq:
                case Assign_Operator::sub_eq:
                case Assign_Operator::div_eq: {
                    report_error("*=, +=, -=, /= currently unsupported by the typechecking system", assign_ast);
                    ctx->success = false;
                } break;
            }
            
            new_stage = 4;
        } // fall through
    }
    finish:
    
    if(new_stage < 4)
    {
        auto job = make_typecheck_job(nullptr, assign_ast, new_stage);
        array_add(ctx->waiting_jobs, job);
    }
    return new_stage != stage;
}

internal
bool typecheck_return(Context *ctx, Return_AST *return_ast)
{
    // Note: return types will be checked from the function prototype
    auto return_types = return_ast->function->prototype->return_types;
    bool all_resolved = true;
    for(u64 i = 0; i < return_types.count; ++i)
    {
        if(!type_resolved(return_types[i]))
        {
            all_resolved = false;
            break;
        }
    }
    
    if(all_resolved)
    {
        // TODO: support multi-return values
        if(return_types.count == 1)
        {
            auto job = make_typecheck_job(return_types[0], return_ast->expr);
            do_typecheck_job(ctx, job);
        }
        else
        {
            // TODO: better error reporting
            print_err("Expected only one return type\n");
            ctx->success = false;
        }
        return true;
    }
    else
    {
        auto job = make_typecheck_job(nullptr, return_ast);
        array_add(ctx->waiting_jobs, job);
        return false;
    }
}

bool typecheck_function_type(Context *ctx, u32 stage, Expr_AST *type, Function_Type_AST *function_type_ast)
{
    u32 new_stage = 0;
    bool add_job = false;
    
    switch(stage)
    {
        case 0: {
            function_type_ast->flags |= EXPR_FLAG_NOT_LVALUE;
            add_job = true;
        } // fall through
        case 1: {
            new_stage = 1;
            Status match = types_match(ctx, type, &type_t_ast);
            if(match != Status::good)
            {
                break;
            }
            set_resolved_type(function_type_ast, &type_t_ast);
            add_job = true;
        } // fall through
        case 2: {
            new_stage = 2;
            auto parameter_types = function_type_ast->parameter_types;
            for(u64 i = 0; i < parameter_types.count; ++i)
            {
                auto job = make_typecheck_job(&type_t_ast, parameter_types[i]);
                do_typecheck_job(ctx, job);
            }
            auto return_types = function_type_ast->return_types;
            for(u64 i = 0; i < return_types.count; ++i)
            {
                auto job = make_typecheck_job(&type_t_ast, return_types[i]);
                do_typecheck_job(ctx, job);
            }
            new_stage = 3;
        } // fall through
        // Note: typecheck_function has special handling for the prototype
        // If this changes, that should also change
    }
    
    if(new_stage < 3)
    {
        auto job = make_typecheck_job(type, function_type_ast, new_stage);
        array_add(ctx->waiting_jobs, job);
    }
    return new_stage != stage;
}

internal
bool typecheck_function(Context *ctx, u32 stage, Expr_AST *type, Function_AST *function_ast)
{
    u32 new_stage = 0;
    bool add_job = false;
    
    // TODO: test if you can use a function recursively in a default value
    // I think this will work, though it isn't self-evident
    
    switch(stage)
    {
        case 0: {
            function_ast->flags |= EXPR_FLAG_NOT_LVALUE;
            set_resolved_type(function_ast->prototype, &type_t_ast);
            set_resolved_type(function_ast, function_ast->prototype);
            auto parameter_types = function_ast->prototype->parameter_types;
            for(u64 i = 0; i < parameter_types.count; ++i)
            {
                if(parameter_types[i])
                {
                    auto job = make_typecheck_job(&type_t_ast, parameter_types[i]);
                    do_typecheck_job(ctx, job);
                }
            }
            auto return_types = function_ast->prototype->return_types;
            for(u64 i = 0; i < return_types.count; ++i)
            {
                assert(return_types[i]);
                auto job = make_typecheck_job(&type_t_ast, return_types[i]);
                do_typecheck_job(ctx, job);
            }
            add_job = true;
        } // fall through
        case 1: {
            new_stage = 1;
            auto parameter_types = function_ast->prototype->parameter_types;
            auto default_values = function_ast->default_values;
            for(u64 i = 0; i < parameter_types.count; ++i)
            {
                auto param_type = parameter_types[i];
                if(default_values[i] && param_type && !type_resolved(param_type))
                {
                    goto finish;
                }
            }
            for(u64 i = 0; i < default_values.count; ++i)
            {
                if(default_values[i])
                {
                    auto job = make_typecheck_job(parameter_types[i], default_values[i]);
                    do_typecheck_job(ctx, job);
                }
            }
            add_job = true;
        } // fall through
        case 2: {
            new_stage = 2;
            auto default_values = function_ast->default_values;
            auto parameter_types = function_ast->prototype->parameter_types;
            for(u64 i = 0; i < parameter_types.count; ++i)
            {
                if(!parameter_types[i])
                {
                    auto val = default_values[i];
                    assert(val);
                    if(type_resolved(val))
                    {
                        parameter_types[i] = val->resolved_type;
                    }
                    else
                    {
                        goto finish;
                    }
                }
            }
            auto param_names = function_ast->param_names;
            for(u64 i = 0; i < param_names.count; ++i)
            {
                assert(parameter_types[i]);
                if(param_names[i])
                {
                    set_resolved_type(param_names[i], parameter_types[i]);
                }
            }
            auto job = make_typecheck_job(nullptr, function_ast->block);
            do_typecheck_job(ctx, job);
            add_job = true;
        } // fall through
        case 3: {
            new_stage = 3;
            Status match = types_match(ctx, type, function_ast->resolved_type);
            if(match != Status::good)
            {
                break;
            }
            new_stage = 4;
        } // fall through
    }
    finish:
    
    if(new_stage < 4)
    {
        auto job = make_typecheck_job(type, function_ast, new_stage);
        array_add(ctx->waiting_jobs, job);
    }
    return new_stage != stage;
}

internal
bool typecheck_function_call(Context *ctx, u32 stage, Expr_AST *type, Function_Call_AST *function_call_ast)
{
    u32 new_stage = 0;
    bool add_job = false;
    
    switch(stage)
    {
        case 0: {
            function_call_ast->flags |= EXPR_FLAG_NOT_LVALUE;
            add_job = true;
        } // fall through
        case 1: {
            new_stage = 1;
            auto function = function_call_ast->function;
            if(!type_resolved(function) && add_job)
            {
                // TODO: this will need to be souped-up for overloading
                // Note: no overloading based on return type, hence the nullptr
                auto job = make_typecheck_job(nullptr, function);
                do_typecheck_job(ctx, job);
            }
            if(!type_resolved(function))
            {
                break;
            }
            add_job = true;
        } // fall through
        case 2: {
            new_stage = 2;
            Expr_AST *func_type = function_call_ast->function->resolved_type;
            Expr_AST *reduced_type;
            Status status = reduce_type(ctx, func_type, &reduced_type);
            if(status != Status::good)
            {
                break;
            }
            if(reduced_type->type != AST_Type::function_type_ast)
            {
                report_error("Expected function\n", function_call_ast->function);
                ctx->success = false;
                break;
            }
            Function_Type_AST *function_type = static_cast<Function_Type_AST*>(reduced_type);
            auto return_types = function_type->return_types;
            for(u64 i = 0; i < return_types.count; ++i)
            {
                if(!type_resolved(return_types[i]))
                {
                    goto finish;
                }
            }
            // TODO support multiple return types
            set_resolved_type(function_call_ast, return_types[0]);
            
            auto param_types = function_type->parameter_types;
            for(u64 i = 0; i < param_types.count; ++i)
            {
                if(!type_resolved(param_types[i]))
                {
                    goto finish;
                }
            }
            // TODO: varargs and named arguments
            auto args = function_call_ast->args;
            if(args.count != param_types.count)
            {
                report_error("Bad number of arguments", function_call_ast);
                ctx->success = false;
                break;
            }
            
            for(u64 i = 0; i < param_types.count; ++i)
            {
                auto job = make_typecheck_job(param_types[i], args[i]);
                do_typecheck_job(ctx, job);
            }
            new_stage = 3;
        } // fall through
    }
    finish:
    
    if(new_stage < 3)
    {
        auto job = make_typecheck_job(type, function_call_ast, new_stage);
        array_add(ctx->waiting_jobs, job);
    }
    return new_stage != stage;
}

internal
bool typecheck_access(Context *ctx, u32 stage, Expr_AST *type, Access_AST *access_ast)
{
    u32 new_stage = 0;
    bool add_job = false;
    Expr_AST *lhs = access_ast->lhs;
    
    switch(stage)
    {
        case 0: {
            add_job = true;
        } // fall through
        case 1: {
            new_stage = 1;
            if(!type_resolved(lhs) && add_job)
            {
                auto job = make_typecheck_job(nullptr, lhs);
                do_typecheck_job(ctx, job);
            }
            if(!type_resolved(lhs))
            {
                break;
            }
            add_job = true;
        } // fall through
        case 2: {
            new_stage = 2;
            if((access_ast->lhs->flags & EXPR_FLAG_LVALUE_MASK) == 0)
            {
                break;
            }
            Expr_AST *lookup_type;
            Status status = reduce_type(ctx, lhs->resolved_type, &lookup_type);
            if(status != Status::good)
            {
                break;
            }
            bool pointer = false;
            if(lookup_type->type == AST_Type::unary_ast)
            {
                Unary_Operator_AST *unop_ast = static_cast<Unary_Operator_AST*>(lookup_type);
                assert(unop_ast->op == Unary_Operator::ref);
                pointer = true;
                status = reduce_type(ctx, unop_ast->operand, &lookup_type);
                if(status != Status::good)
                {
                    break;
                }
            }
            Expr_AST *expr;
            
            if(lookup_type->type == AST_Type::struct_ast)
            {
                Struct_AST *struct_ast = static_cast<Struct_AST*>(lookup_type);
                expr = scope_find(struct_ast->constant_scope, access_ast->atom, 0, false);
                if(!expr)
                {
                    expr = scope_find(struct_ast->field_scope, access_ast->atom, 0, false);
                }
            }
            else if(lookup_type->type == AST_Type::enum_ast && !pointer)
            {
                Enum_AST *enum_ast = static_cast<Enum_AST*>(lookup_type);
                expr = scope_find(enum_ast->scope, access_ast->atom, 0, false);
            }
            else
            {
                report_error("Expected enum, struct, or pointer to struct", lhs);
                ctx->success = false;
                break;
            }
            if(expr)
            {
                access_ast->expr = expr;
            }
            else
            {
                report_error("No such member or field", access_ast);
                ctx->success = false;
                break;
            }
            if(pointer)
            {
                access_ast->flags |= EXPR_FLAG_LVALUE;
            }
            else
            {
                access_ast->flags |= (access_ast->lhs->flags & EXPR_FLAG_LVALUE_MASK);
            }
            add_job = true;
        } // fall through
        case 3: {
            new_stage = 3;
            if(!type_resolved(access_ast->expr))
            {
                break;
            }
            set_resolved_type(access_ast, access_ast->expr->resolved_type);
            new_stage = 4;
        } // fall through
    }
    
    if(new_stage < 4)
    {
        auto job = make_typecheck_job(type, access_ast, new_stage);
        array_add(ctx->waiting_jobs, job);
    }
    return new_stage != stage;
}

internal
bool typecheck_binop_ast(Context *ctx, u32 stage, Expr_AST *type, Binary_Operator_AST *binop_ast)
{
    u32 new_stage = 0;
    u32 done_stage = 0;
    bool add_job = false;
    
    switch(binop_ast->op)
    {
        case Binary_Operator::cmp_eq:
        case Binary_Operator::cmp_neq:
        case Binary_Operator::cmp_lt:
        case Binary_Operator::cmp_le:
        case Binary_Operator::cmp_gt:
        case Binary_Operator::cmp_ge: {
            done_stage = 4;
            switch(stage)
            {
                case 0: {
                    binop_ast->flags |= EXPR_FLAG_NOT_LVALUE;
                    if(type)
                    {
                        Status status = types_match(ctx, &boollike_t_ast, type);
                        if(status != Status::good)
                        {
                            goto finish;
                        }
                    }
                    add_job = true;
                } // fall through
                case 1: {
                    new_stage = 1;
                    Expr_AST *binop_type;
                    if(type)
                    {
                        binop_type = type;
                    }
                    else
                    {
                        binop_type = &bool8_t_ast;
                    }
                    set_resolved_type(binop_ast, binop_type);
                    add_job = true;
                } // fall through
                case 2: {
                    new_stage = 2;
                    // TODO: change with operator overloading
                    auto job = make_typecheck_job(&numberlike_t_ast, binop_ast->lhs);
                    do_typecheck_job(ctx, job);
                    job = make_typecheck_job(&numberlike_t_ast, binop_ast->rhs);
                    do_typecheck_job(ctx, job);
                    add_job = true;
                } // fall through
                case 3: {
                    new_stage = 3;
                    if(!type_resolved(binop_ast->lhs))
                    {
                        goto finish;
                    }
                    if(!type_resolved(binop_ast->rhs))
                    {
                        goto finish;
                    }
                    Status status = types_match(ctx, binop_ast->lhs->resolved_type, binop_ast->rhs->resolved_type);
                    if(status != Status::good)
                    {
                        goto finish;
                    }
                    new_stage = 4;
                } // fall through
                
            }
        } break;
        case Binary_Operator::add:
        case Binary_Operator::sub:
        case Binary_Operator::mul:
        case Binary_Operator::div: {
            done_stage = 3;
            switch(stage)
            {
                case 0: {
                    binop_ast->flags |= EXPR_FLAG_NOT_LVALUE;
                    if(type)
                    {
                        Status status = types_match(ctx, &numberlike_t_ast, type);
                        if(status != Status::good)
                        {
                            goto finish;
                        }
                    }
                    add_job = true;
                } // fall through
                case 1: {
                    new_stage = 1;
                    Expr_AST *number_type;
                    if(type)
                    {
                        number_type = type;
                    }
                    else
                    {
                        number_type = &numberlike_t_ast;
                    }
                    auto job = make_typecheck_job(number_type, binop_ast->lhs);
                    do_typecheck_job(ctx, job);
                    job = make_typecheck_job(number_type, binop_ast->rhs);
                    do_typecheck_job(ctx, job);
                    add_job = true;
                } // fall through
                case 2: {
                    new_stage = 2;
                    if(!type_resolved(binop_ast->lhs))
                    {
                        goto finish;
                    }
                    if(!type_resolved(binop_ast->rhs))
                    {
                        goto finish;
                    }
                    Primitive_AST *combined;
                    Status status = combine_number_types(ctx, binop_ast->lhs->resolved_type, binop_ast->rhs->resolved_type, &combined);
                    if(status != Status::good)
                    {
                        goto finish;
                    }
                    if(type)
                    {
                        status = types_match(ctx, type, combined);
                        if(status != Status::good)
                        {
                            goto finish;
                        }
                    }
                    set_resolved_type(binop_ast, combined);
                    new_stage = 3;
                } // fall through
            }
        } break;
        case Binary_Operator::subscript: {
            // TODO: change once operator overloading is a thing
            // Polymorphism may also change it (e.g. operator[](ptr : &$T) -> &T
            done_stage = 2;
            switch(stage)
            {
                case 0: {
                    // TODO: once fixed size arrays are a thing, this may not be an lvalue
                    binop_ast->flags |= EXPR_FLAG_LVALUE;
                    Expr_AST *type_to_check = nullptr;
                    if(type)
                    {
                        // TODO: maybe make a canonical type table, so not every & makes a new type in memory
                        Unary_Operator_AST *type_to_deref = construct_ast(ctx->ast_pool, Unary_Operator_AST, 0, 0);
                        type_to_deref->flags |= AST_FLAG_SYNTHETIC;
                        set_resolved_type(type_to_deref, &type_t_ast);
                        type_to_deref->op = Unary_Operator::ref;
                        type_to_deref->operand = type;
                        type_to_check = type_to_deref;
                    }
                    auto job = make_typecheck_job(type_to_check, binop_ast->lhs);
                    do_typecheck_job(ctx, job);
                    job = make_typecheck_job(&numberlike_t_ast, binop_ast->rhs);
                    do_typecheck_job(ctx, job);
                } // fall through
                case 1: {
                    new_stage = 1;
                    if(!type_resolved(binop_ast->lhs))
                    {
                        goto finish;
                    }
                    Expr_AST *reduced;
                    Status status = reduce_type(ctx, binop_ast->lhs->resolved_type, &reduced);
                    if(status != Status::good)
                    {
                        goto finish;
                    }
                    if(reduced->type != AST_Type::unary_ast)
                    {
                        report_error("Expected expr of pointer type", binop_ast->lhs);
                        ctx->success = false;
                        goto finish;
                    }
                    Unary_Operator_AST *lhs_type = static_cast<Unary_Operator_AST*>(reduced);
                    assert(lhs_type->resolved_type == &type_t_ast);
                    assert(lhs_type->op == Unary_Operator::ref);
                    status = types_match(ctx, type, lhs_type->operand);
                    if(status != Status::good)
                    {
                        goto finish;
                    }
                    set_resolved_type(binop_ast, lhs_type->operand);
                    new_stage = 2;
                } // fall through
            }
        } break;
        case Binary_Operator::lor:
        case Binary_Operator::land: {
            done_stage = 1;
            Expr_AST *bool_type;
            if(type)
            {
                Status status = types_match(ctx, &boollike_t_ast, type);
                if(status != Status::good)
                {
                    goto finish;
                }
                bool_type = type;
            }
            else
            {
                bool_type = &bool8_t_ast;
            }
            binop_ast->flags |= EXPR_FLAG_NOT_LVALUE;
            auto job = make_typecheck_job(bool_type, binop_ast->lhs);
            do_typecheck_job(ctx, job);
            job = make_typecheck_job(bool_type, binop_ast->rhs);
            do_typecheck_job(ctx, job);
            
            // TODO: make sure this is a concrete bool type, not boollike
            set_resolved_type(binop_ast, bool_type);
            new_stage = 1;
        } break;
    }
    finish:
    
    if(new_stage < done_stage)
    {
        auto job = make_typecheck_job(type, binop_ast, new_stage);
        array_add(ctx->waiting_jobs, job);
    }
    return new_stage != stage;
}

internal
bool typecheck_number(Context *ctx, Expr_AST *type, Number_AST *number_ast)
{
    number_ast->flags |= EXPR_FLAG_NOT_LVALUE;
    bool number_float_like = (number_ast->flags & NUMBER_FLAG_FLOATLIKE);
    Primitive_AST *number_type;
    if(type)
    {
        Expr_AST *reduced;
        Status status = reduce_type(ctx, type, &reduced);
        if(status != Status::good)
        {
            goto finish;
        }
        if(reduced->type != AST_Type::primitive_ast)
        {
            report_error("Expected primitive type", type);
            ctx->success = false;
            goto finish;
        }
        number_type = static_cast<Primitive_AST*>(reduced);
        u64 prim = number_type->primitive;
        if(!(prim & PRIM_NUMBERLIKE))
        {
            report_error("Expected number type", type);
            ctx->success = false;
            goto finish;
        }
    }
    else if(number_float_like)
    {
        number_type = &f64_t_ast;
    }
    else
    {
        number_type = &s64_t_ast;
    }
    
    {
        u64 prim = number_type->primitive;
        u64 size = prim & PRIM_SIZE_MASK;
        if(number_float_like)
        {
            if(!(prim & PRIM_FLOATLIKE))
            {
                report_error("Got float, expected other number", number_ast);
                ctx->success = false;
                goto finish;
            }
            
            switch(size)
            {
                case PRIM_SIZE4: {
                    set_resolved_type(number_ast, &f32_t_ast);
                } break;
                case PRIM_NO_SIZE:
                case PRIM_SIZE8: {
                    set_resolved_type(number_ast, &f64_t_ast);
                } break;
                default: {
                    // Note: no floatlike type should have a different size
                    assert(false);
                } break;
            }
        }
        else
        {
            if(!(prim & PRIM_INTLIKE))
            {
                report_error("Got int, expected other number", number_ast);
                ctx->success = false;
                goto finish;
            }
            u64 sign = prim & PRIM_SIGN_MASK;
            if(sign == PRIM_UNSIGNED_INT)
            {
                switch(size)
                {
                    case PRIM_SIZE1: {
                        set_resolved_type(number_ast, &u8_t_ast);
                    } break;
                    case PRIM_SIZE2: {
                        set_resolved_type(number_ast, &u16_t_ast);
                    } break;
                    case PRIM_SIZE4: {
                        set_resolved_type(number_ast, &u32_t_ast);
                    } break;
                    case PRIM_NO_SIZE:
                    case PRIM_SIZE8: {
                        set_resolved_type(number_ast, &u64_t_ast);
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
                        set_resolved_type(number_ast, &s8_t_ast);
                    } break;
                    case PRIM_SIZE2: {
                        set_resolved_type(number_ast, &s16_t_ast);
                    } break;
                    case PRIM_SIZE4: {
                        set_resolved_type(number_ast, &s32_t_ast);
                    } break;
                    case PRIM_NO_SIZE:
                    case PRIM_SIZE8: {
                        set_resolved_type(number_ast, &s64_t_ast);
                    } break;
                    default: {
                        assert(false);
                    } break;
                }
            }
        }
    }
    finish:
    
    if(type_resolved(number_ast))
    {
        return true;
    }
    else
    {
        auto job = make_typecheck_job(type, number_ast);
        array_add(ctx->waiting_jobs, job);
        return false;
    }
}

internal
bool typecheck_enum(Context *ctx, u32 stage, Expr_AST *type, Enum_AST *enum_ast)
{
    u32 new_stage = 0;
    switch(stage)
    {
        case 0: {
            if(type)
            {
                Status status = types_match(ctx, type, &type_t_ast);
                if(status != Status::good)
                {
                    break;
                }
            }
        } // fall through
        case 1: {
            new_stage = 1;
            set_resolved_type(enum_ast, &type_t_ast);
            for(u64 i = 0; i < enum_ast->values.count; ++i)
            {
                // TODO: enum values should all have the same type
                auto job = make_typecheck_job(nullptr, enum_ast->values[i]);
                do_typecheck_job(ctx, job);
            }
            new_stage = 2;
        } // fall through
    }
    
    if(new_stage < 2)
    {
        auto job = make_typecheck_job(type, enum_ast, new_stage);
        array_add(ctx->waiting_jobs, job);
    }
    return new_stage != stage;
}

internal
bool typecheck_struct(Context *ctx, u32 stage, Expr_AST *type, Struct_AST *struct_ast)
{
    u32 new_stage = 0;
    switch(stage)
    {
        case 0: {
            if(type)
            {
                Status status = types_match(ctx, type, &type_t_ast);
                if(status != Status::good)
                {
                    break;
                }
            }
        } // fall through
        case 1: {
            new_stage = 1;
            set_resolved_type(struct_ast, &type_t_ast);
            
            for(u64 i = 0; i < struct_ast->constants.count; ++i)
            {
                auto job = make_typecheck_job(nullptr, struct_ast->constants[i]);
                do_typecheck_job(ctx, job);
            }
            for(u64 i = 0; i < struct_ast->fields.count; ++i)
            {
                auto job = make_typecheck_job(nullptr, struct_ast->fields[i]);
                do_typecheck_job(ctx, job);
            }
            new_stage = 2;
        } // fall through
        // TODO: size struct
    }
    
    if(new_stage < 2)
    {
        auto job = make_typecheck_job(type, struct_ast, new_stage);
        array_add(ctx->waiting_jobs, job);
    }
    return new_stage != stage;
}

internal
bool typecheck_unary(Context *ctx, u32 stage, Expr_AST *type, Unary_Operator_AST *unop_ast)
{
    u32 new_stage = 0;
    u32 done_stage;
    
    switch(unop_ast->op)
    {
        case Unary_Operator::plus:
        case Unary_Operator::minus: {
            done_stage = 3;
            switch(stage)
            {
                case 0: {
                    if(type)
                    {
                        Status status = types_match(ctx, &numberlike_t_ast, type);
                        if(status != Status::good)
                        {
                            goto finish;
                        }
                    }
                } // fall through
                case 1: {
                    new_stage = 1;
                    unop_ast->flags |= EXPR_FLAG_NOT_LVALUE;
                    Expr_AST *number_type;
                    if(type)
                    {
                        number_type = type;
                    }
                    else
                    {
                        number_type = &numberlike_t_ast;
                    }
                    auto job = make_typecheck_job(number_type, unop_ast->operand);
                    do_typecheck_job(ctx, job);
                } // fall through
                case 2: {
                    new_stage = 2;
                    if(!type_resolved(unop_ast->operand))
                    {
                        goto finish;
                    }
                    // TODO: make sure this is a concrete type, rather than numberlike_t_ast
                    // TODO: make sure it isn't negative an unsigned type
                    set_resolved_type(unop_ast, unop_ast->operand->resolved_type);
                    new_stage = 3;
                } // fall through
            }
        } break;
        case Unary_Operator::deref: {
            done_stage = 2;
            switch(stage)
            {
                
                case 0: {
                    unop_ast->flags |= EXPR_FLAG_LVALUE;
                    Expr_AST *type_to_check = nullptr;
                    if(type)
                    {
                        // TODO: maybe make a canonical type table, so not every & makes a new type in memory
                        Unary_Operator_AST *type_to_deref = construct_ast(ctx->ast_pool, Unary_Operator_AST, 0, 0);
                        type_to_deref->flags |= AST_FLAG_SYNTHETIC;
                        set_resolved_type(type_to_deref, &type_t_ast);
                        type_to_deref->op = Unary_Operator::ref;
                        type_to_deref->operand = type;
                        type_to_check = type_to_deref;
                    }
                    auto job = make_typecheck_job(type_to_check, unop_ast->operand);
                    do_typecheck_job(ctx, job);
                } // fall through
                case 1: {
                    new_stage = 1;
                    if(!type_resolved(unop_ast->operand))
                    {
                        goto finish;
                    }
                    Expr_AST *reduced;
                    Status status = reduce_type(ctx, unop_ast->operand->resolved_type, &reduced);
                    if(status != Status::good)
                    {
                        goto finish;
                    }
                    if(reduced->type != AST_Type::unary_ast)
                    {
                        report_error("Expected expr of pointer type", unop_ast->operand);
                        ctx->success = false;
                        goto finish;
                    }
                    Unary_Operator_AST *operand_type = static_cast<Unary_Operator_AST*>(reduced);
                    assert(operand_type->resolved_type == &type_t_ast);
                    assert(operand_type->op == Unary_Operator::ref);
                    status = types_match(ctx, type, operand_type->operand);
                    if(status != Status::good)
                    {
                        goto finish;
                    }
                    set_resolved_type(unop_ast, operand_type->operand);
                    new_stage = 2;
                } // fall through
            }
        } break;
        case Unary_Operator::ref: {
            done_stage = 2;
            Expr_AST *inner_type;
            if(type)
            {
                Expr_AST *reduced;
                Status status = reduce_type(ctx, type, &reduced);
                if(status != Status::good)
                {
                    goto finish;
                }
                if(reduced->type == AST_Type::unary_ast)
                {
                    Unary_Operator_AST *type_to_match = static_cast<Unary_Operator_AST*>(reduced);
                    assert(type_to_match->op == Unary_Operator::ref);
                    inner_type = type_to_match->operand;
                }
                else if(reduced == &type_t_ast)
                {
                    // TODO: probably need to match Type more generally
                    // e.g. if user includes Type explicitly in the program
                    inner_type = &type_t_ast;
                }
                else
                {
                    report_error("Got expr of pointer type", unop_ast);
                    ctx->success = false;
                    goto finish;
                }
            }
            else
            {
                inner_type = nullptr;
            }
            
            
            // TODO: probably change so that &Type and &value are distinct
            // This way types can be values, and it may simplify some code
            switch(stage)
            {
                case 0: {
                    auto job = make_typecheck_job(inner_type, unop_ast->operand);
                    do_typecheck_job(ctx, job);
                } // fall through
                case 1: {
                    new_stage = 1;
                    if(!type_resolved(unop_ast->operand))
                    {
                        goto finish;
                    }
                    if(inner_type != &type_t_ast)
                    {
                        u16 lvalue_flags = unop_ast->operand->flags & EXPR_FLAG_LVALUE_MASK;
                        if(lvalue_flags == 0)
                        {
                            goto finish;
                        }
                        else if(lvalue_flags == EXPR_FLAG_NOT_LVALUE)
                        {
                            report_error("Can only take pointers to lvalues", unop_ast);
                            ctx->success = false;
                            goto finish;
                        }
                        else
                        {
                            assert(lvalue_flags == EXPR_FLAG_LVALUE);
                        }
                    }
                    if(unop_ast->operand->resolved_type == &type_t_ast)
                    {
                        set_resolved_type(unop_ast, &type_t_ast);
                    }
                    else if(type)
                    {
                        // TODO: check if type matches?
                        // inner_type should match unop_ast->operand->resolved_type
                        // type should be a pointer type
                        set_resolved_type(unop_ast, type);
                    }
                    else
                    {
                        // TODO: type table to reduce memory usage? (among other things)
                        Unary_Operator_AST *pointer_type = construct_ast(ctx->ast_pool, Unary_Operator_AST, 0, 0);
                        pointer_type->flags |= AST_FLAG_SYNTHETIC;
                        set_resolved_type(pointer_type, &type_t_ast);
                        pointer_type->op = Unary_Operator::ref;
                        pointer_type->operand = unop_ast->operand->resolved_type;
                        
                        set_resolved_type(unop_ast, pointer_type);
                    }
                    new_stage = 2;
                } // fall through
            }
        } break;
        case Unary_Operator::lnot: {
            done_stage = 2;
            switch(stage)
            {
                case 0: {
                    Expr_AST *bool_type;
                    if(type)
                    {
                        Status status = types_match(ctx, &boollike_t_ast, type);
                        if(status != Status::good)
                        {
                            goto finish;
                        }
                        bool_type = type;
                    }
                    else
                    {
                        bool_type = &bool8_t_ast;
                    }
                    auto job = make_typecheck_job(bool_type, unop_ast->operand);
                    do_typecheck_job(ctx, job);
                } // fall through
                case 1: {
                    new_stage = 1;
                    if(!type_resolved(unop_ast->operand))
                    {
                        goto finish;
                    }
                    // TODO: make sure this is a concrete type (instead of boollike)
                    set_resolved_type(unop_ast, unop_ast->operand->resolved_type);
                    new_stage = 2;
                } // fall through
            }
        } break;
    }
    finish:
    
    if(new_stage < done_stage)
    {
        auto job = make_typecheck_job(type, unop_ast, new_stage);
        array_add(ctx->waiting_jobs, job);
    }
    return new_stage != stage;
}

bool typecheck_ident(Context *ctx, u32 stage, Expr_AST *type, Ident_AST *ident_ast)
{
    u32 new_stage = 0;
    switch(stage)
    {
        case 0: {
            ident_ast->flags |= EXPR_FLAG_LVALUE;
            Expr_AST *expr = scope_find(ident_ast->scope, ident_ast->atom, ident_ast->scope_index);
            if(expr)
            {
                ident_set_expr(ident_ast, expr);
            }
            else
            {
                String *str = ident_ast->atom.str;
                print_err("Error: %d:%d: Undeclared identifier \"%.*s\"\n", ident_ast->line_number, ident_ast->line_offset, str->count, str->data);
                ctx->success = false;
                break;
            }
        } // fall through
        case 1: {
            new_stage = 1;
            Expr_AST *expr = ident_get_expr(ident_ast);
            if(!type_resolved(expr))
            {
                break;
            }
        } // fall through
        case 2: {
            new_stage = 2;
            Expr_AST *expr = ident_get_expr(ident_ast);
            if(type)
            {
                Status status = types_match(ctx, type, expr->resolved_type);
                if(status != Status::good)
                {
                    break;
                }
            }
            set_resolved_type(ident_ast, expr->resolved_type);
            new_stage = 3;
        } // fall through
    }
    
    if(new_stage < 3)
    {
        auto job = make_typecheck_job(type, ident_ast, new_stage);
        array_add(ctx->waiting_jobs, job);
    }
    return new_stage != stage;
}

internal
bool typecheck_primitive(Context *ctx, Expr_AST *type, Primitive_AST *prim_ast)
{
    if(type)
    {
        Status status = types_match(ctx, type, &type_t_ast);
        if(status == Status::good)
        {
            set_resolved_type(prim_ast, &type_t_ast);
            return true;
        }
    }
    else
    {
        set_resolved_type(prim_ast, &type_t_ast);
        return true;
    }
    
    auto job = make_typecheck_job(type, prim_ast);
    array_add(ctx->waiting_jobs, job);
    return false;
}

internal 
bool typecheck_string(Context *ctx, Expr_AST *type, String_AST *str_ast)
{
    report_error("Strings types are currently unsupported", str_ast);
    ctx->success = false;
    return false;
}

internal
bool typecheck_bool(Context *ctx, Expr_AST *type, Bool_AST *bool_ast)
{
    if(type)
    {
        Expr_AST *reduced_ast;
        Status status = reduce_type(ctx, type, &reduced_ast);
        if(status == Status::good)
        {
            if(reduced_ast->type != AST_Type::primitive_ast)
            {
                report_error("Type mismatch: got bool", bool_ast);
                ctx->success = false;
                return false;
            }
            
            Primitive_AST *prim_ast = static_cast<Primitive_AST*>(reduced_ast);
            
            u64 prim = prim_ast->primitive;
            if(!(prim & PRIM_BOOLLIKE))
            {
                report_error("Type mismatch: got bool", bool_ast);
                ctx->success = false;
                return false;
            }
            
            u64 size = prim & PRIM_SIZE_MASK;
            
            switch(size)
            {
                case PRIM_NO_SIZE:
                case PRIM_SIZE1: {
                    set_resolved_type(bool_ast, &bool8_t_ast);
                } break;
                case PRIM_SIZE2: {
                    set_resolved_type(bool_ast, &bool16_t_ast);
                } break;
                case PRIM_SIZE4: {
                    set_resolved_type(bool_ast, &bool32_t_ast);
                } break;
                case PRIM_SIZE8: {
                    set_resolved_type(bool_ast, &bool64_t_ast);
                } break;
                default: {
                    assert(false);
                } break;
            }
            return true;
        }
    }
    else
    {
        set_resolved_type(bool_ast, &bool8_t_ast);
        return true;
    }
    
    auto job = make_typecheck_job(type, bool_ast);
    array_add(ctx->waiting_jobs, job);
    return false;
}



Status reduce_type(Context *ctx, Expr_AST *type, Expr_AST **type_out)
{
    assert(type_resolved(type));
    // assert type->resolved_type matches type_t_ast ?
    
    switch(type->type)
    {
        case AST_Type::ident_ast: {
            Ident_AST *ident_ast = static_cast<Ident_AST*>(type);
            
            u64 ident_type = ident_get_type(ident_ast);
            Expr_AST *expr = ident_get_expr(ident_ast);
            // TODO: Is this type of constancy sufficient
            bool constant = (ident_ast->flags & EXPR_FLAG_CONSTANT);
            if(ident_type == IDENT_REFERENCE || (ident_type == IDENT_DECL && constant))
            {
                if(expr)
                {
                    return reduce_type(ctx, expr, type_out);
                }
                else
                {
                    return Status::yield;
                }
            }
            else
            {
                // TODO: later, handle type parameters, etc
                report_error("Cannot reduce identifier", type);
                ctx->success = false;
                return Status::bad;
            }
        } break;
        case AST_Type::access_ast: {
            Access_AST *access_ast = static_cast<Access_AST*>(type);
            if(access_ast->expr)
            {
                return reduce_type(ctx, access_ast->expr, type_out);
            }
            else
            {
                return Status::yield;
            }
        } break;
        
        case AST_Type::unary_ast: {
            Unary_Operator_AST *unop_ast = static_cast<Unary_Operator_AST*>(type);
            assert(unop_ast->op == Unary_Operator::ref);
        } // fall through
        case AST_Type::function_type_ast:
        case AST_Type::struct_ast:
        case AST_Type::enum_ast:
        case AST_Type::primitive_ast: {
            *type_out = type;
            return Status::good;
        } break;
        
        case AST_Type::function_ast:
        case AST_Type::function_call_ast:
        case AST_Type::binary_operator_ast:
        case AST_Type::number_ast:
        case AST_Type::string_ast:
        case AST_Type::bool_ast: {
            // assert(false) // Should this be allowed?
            report_error("Expected type", type);
            ctx->success = false;
            return Status::bad;
        } break;
        
        case_non_exprs: {
            assert(false);
        } break;
    }
    
    assert(false);
    return Status::bad;
}


internal
Status combine_number_types(Context *ctx, Expr_AST *t1, Expr_AST *t2, Primitive_AST **out)
{
    Expr_AST *reduced_t1, *reduced_t2;
    Status status = reduce_type(ctx, t1, &reduced_t1);
    if(status != Status::good)
    {
        return status;
    }
    status = reduce_type(ctx, t2, &reduced_t2);
    if(status != Status::good)
    {
        return status;
    }
    if(reduced_t1->type != AST_Type::primitive_ast)
    {
        report_error("Expected number type", t1);
        ctx->success = false;
        return Status::bad;
    }
    if(reduced_t2->type != AST_Type::primitive_ast)
    {
        report_error("Expected number type", t2);
        ctx->success = false;
        return Status::bad;
    }
    Primitive_AST *prim_ast1 = static_cast<Primitive_AST*>(reduced_t1);
    Primitive_AST *prim_ast2 = static_cast<Primitive_AST*>(reduced_t2);
    
    u64 prim1 = prim_ast1->primitive;
    u64 prim2 = prim_ast2->primitive;
    u64 size1 = prim1 & PRIM_SIZE_MASK;
    u64 size2 = prim2 & PRIM_SIZE_MASK;
    u64 max_size = max(size1, size2);
    
    if((prim1 & PRIM_FLOATLIKE) && (prim2 & PRIM_FLOATLIKE))
    {
        if(max_size == PRIM_NO_SIZE)
        {
            // TODO: should this be a concrete type, rather than floatlike?
            *out = &floatlike_t_ast;
        }
        else if(max_size == PRIM_SIZE4)
        {
            *out = &f32_t_ast;
        }
        else
        {
            assert(max_size == PRIM_SIZE8);
            *out = &f64_t_ast;
        }
    }
    else if((prim1 & PRIM_INTLIKE) && (prim2 & PRIM_INTLIKE))
    {
        // TODO: maybe a warning or error if mixing signed and unsigned?
        // Maybe only if u64 and s64, since u64 doesn't necessarily fit in s64
        if((prim1 & PRIM_SIGN_MASK) == PRIM_UNSIGNED_INT && (prim2 & PRIM_SIGN_MASK) == PRIM_UNSIGNED_INT)
        {
            switch(max_size)
            {
                case PRIM_NO_SIZE: {
                    // TODO: maybe make uintlike_t_ast ?
                    *out = &intlike_t_ast;
                } break;
                case PRIM_SIZE1: {
                    *out = &u8_t_ast;
                } break;
                case PRIM_SIZE2: {
                    *out = &u16_t_ast;
                } break;
                case PRIM_SIZE4: {
                    *out = &u32_t_ast;
                } break;
                case PRIM_SIZE8: {
                    *out = &u64_t_ast;
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
                    *out = &intlike_t_ast;
                } break;
                case PRIM_SIZE1: {
                    *out = &s8_t_ast;
                } break;
                case PRIM_SIZE2: {
                    *out = &s16_t_ast;
                } break;
                case PRIM_SIZE4: {
                    *out = &s32_t_ast;
                } break;
                case PRIM_SIZE8: {
                    *out = &s64_t_ast;
                } break;
                default: {
                    assert(false);
                } break;
            }
        }
    }
    else
    {
        // TODO: better error reporting
        report_error("Number types don't match", t1, t2);
        ctx->success = false;
        return Status::bad;
    }
    
    return Status::good;
}

Status types_match(Context *ctx, Expr_AST *t1, Expr_AST *t2, bool report)
{
    if(!t1 || !t2)
    {
        return Status::good;
    }
    
    Expr_AST *lhs, *rhs;
    Status status = reduce_type(ctx, t1, &lhs);
    if(status != Status::good)
    {
        return status;
    }
    status = reduce_type(ctx, t2, &rhs);
    if(status != Status::good)
    {
        return status;
    }
    
    switch(lhs->type)
    {
        case AST_Type::function_type_ast: {
            if(rhs->type != AST_Type::function_type_ast)
            {
                if(report)
                {
                    report_error("Function type vs non-function type", lhs, rhs);
                    ctx->success = false;
                }
                return Status::bad;
            }
            Function_Type_AST *lhs_func = static_cast<Function_Type_AST*>(lhs);
            Function_Type_AST *rhs_func = static_cast<Function_Type_AST*>(rhs);
            
            if(lhs_func->parameter_types.count != rhs_func->parameter_types.count)
            {
                if(report)
                {
                    report_error("Function types do not have same number of parameters", lhs, rhs);
                    ctx->success = false;
                }
                return Status::bad;
            }
            if(lhs_func->return_types.count != rhs_func->return_types.count)
            {
                if(report)
                {
                    report_error("Function types do not have same number of return values", lhs, rhs);
                    ctx->success = false;
                }
                return Status::bad;
            }
            
            for(u64 i = 0; i < lhs_func->parameter_types.count; ++i)
            {
                status = types_match(ctx, lhs_func->parameter_types[i], rhs_func->parameter_types[i], report);
                if(status != Status::good)
                {
                    return status;
                }
            }
            for(u64 i = 0; i < lhs_func->return_types.count; ++i)
            {
                status = types_match(ctx, lhs_func->return_types[i], rhs_func->return_types[i], report);
                if(status != Status::good)
                {
                    return status;
                }
            }
        } break;
        case AST_Type::enum_ast: {
            if(lhs != rhs)
            {
                if(report)
                {
                    report_error("Mismatching enums", lhs, rhs);
                    ctx->success = false;
                }
                return Status::bad;
            }
        } break;
        case AST_Type::struct_ast: {
            if(lhs != rhs)
            {
                if(report)
                {
                    report_error("Mismatching structs", lhs, rhs);
                    ctx->success = false;
                }
                return Status::bad;
            }
        } break;
        case AST_Type::unary_ast: {
            if(rhs->type != AST_Type::unary_ast)
            {
                if(report)
                {
                    report_error("Pointer vs non-pointer type", lhs, rhs);
                    ctx->success = false;
                }
                return Status::bad;
            }
            Unary_Operator_AST *lhs_unary = static_cast<Unary_Operator_AST*>(lhs);
            Unary_Operator_AST *rhs_unary = static_cast<Unary_Operator_AST*>(rhs);
            
            // Assert since resolved_type is type_t_ast
            assert(lhs_unary->op == Unary_Operator::ref);
            assert(rhs_unary->op == Unary_Operator::ref);
            
            return types_match(ctx, lhs_unary->operand, rhs_unary->operand, report);
        } break;
        case AST_Type::primitive_ast: {
            if(rhs->type != AST_Type::primitive_ast)
            {
                if(report)
                {
                    report_error("Primitive vs non-primitive type", lhs, rhs);
                    ctx->success = false;
                }
                return Status::bad;
            }
            Primitive_AST *lhs_prim_ast = static_cast<Primitive_AST*>(lhs);
            Primitive_AST *rhs_prim_ast = static_cast<Primitive_AST*>(rhs);
            
            u64 lhs_prim = lhs_prim_ast->primitive;
            u64 rhs_prim = rhs_prim_ast->primitive;
            u64 lhs_size = lhs_prim & PRIM_SIZE_MASK;
            u64 rhs_size = rhs_prim & PRIM_SIZE_MASK;
            
            if((lhs_prim & PRIM_FLOATLIKE) && (rhs_prim & PRIM_FLOATLIKE))
            {
                if(lhs_size != rhs_size && lhs_size != 0 && rhs_size != 0)
                {
                    if(rhs_size > lhs_size)
                    {
                        if(report)
                        {
                            report_error("Float size mismatch", lhs, rhs);
                            ctx->success = false;
                        }
                        return Status::bad;
                    }
                    else
                    {
                        return Status::good;
                    }
                }
                else
                {
                    return Status::good;
                }
            }
            else if((lhs_prim & PRIM_INTLIKE) && (rhs_prim & PRIM_INTLIKE))
            {
                if(lhs_size != rhs_size && lhs_size != 0 && rhs_size != 0)
                {
                    if(rhs_size > lhs_size)
                    {
                        if(report)
                        {
                            report_error("Integer size mismatch", lhs, rhs);
                            ctx->success = false;
                        }
                        return Status::bad;
                    }
                    else
                    {
                        return Status::good;
                    }
                }
                else
                {
                    return Status::good;
                }
            }
            else if((lhs_prim & PRIM_BOOLLIKE) &&
                    (rhs_prim & PRIM_BOOLLIKE))
            {
                return Status::good;
            }
            else if(lhs_prim == PRIM_VOID && rhs_prim == PRIM_VOID)
            {
                return Status::good;
            }
            else if(lhs_prim == PRIM_TYPE && rhs_prim == PRIM_TYPE)
            {
                return Status::good;
            }
            else
            {
                if(report)
                {
                    report_error("Primitive type mismatch", lhs, rhs);
                    ctx->success = false;
                }
                return Status::bad;
            }
        } break;
        
        case AST_Type::ident_ast: // TODO: this could be a type in the future
        case AST_Type::function_ast:
        case AST_Type::function_call_ast:
        case AST_Type::access_ast: // Note: would have been reduced
        case AST_Type::binary_operator_ast:
        case AST_Type::number_ast:
        case AST_Type::string_ast:
        case AST_Type::bool_ast: {
            // Not type
            assert(false);
        } break;
        case_non_exprs: {
            assert(false);
        } break;
    }
    
    return Status::bad;
}

void print_jobs(Array<Job> jobs)
{
    for(u64 i = 0; i < jobs.count; ++i)
    {
        Job *job = &jobs[i];
        AST *ast = job->typecheck.ast;
        print_err("Waiting job %lu:\n\tStage: %lu\n\tLocation: %lu:%lu\n\tAST_Type: %s\n", i, job->stage, ast->line_number, ast->line_offset, ast_type_names[(u64)ast->type]);
    }
}

bool typecheck_all(Pool_Allocator *ast_pool, Array<Decl_AST*> decls)
{
    Dynamic_Array<Job> job_arrays[2] = {0};
    auto current = &job_arrays[0];
    auto next = &job_arrays[1];
    
    Context ctx;
    ctx.ast_pool = ast_pool;
    ctx.waiting_jobs = next;
    ctx.success = true;
    
    for(u64 i = 0; i < decls.count; ++i)
    {
        auto job = make_typecheck_job(nullptr, decls[i]);
        do_typecheck_job(&ctx, job);
    }
    
    if(!ctx.success)
    {
        print_err("Success flag was unset (1)\n");
        return false;
    }
    
    while(true)
    {
        auto tmp = current;
        current = next;
        next = tmp;
        
        if(current->count == 0)
        {
            return true;
        }
        next->count = 0;
        ctx.waiting_jobs = next;
        
        bool change = false;
        for(u64 i = 0; i < current->count; ++i)
        {
            if(do_typecheck_job(&ctx, (*current)[i]))
            {
                change = true;
            }
        }
        
        if(!ctx.success)
        {
            print_err("Success flag was unset (2)\n");
            return false;
        }
        else if(!change)
        {
            // TODO: detect error
            print_err("Error: no progress (%lu waiting jobs)\n", next->count);
            print_jobs(next->array);
            return false;
        }
    }
}
