
void init_primitive_types()
{
    zero_memory(&u8_t_ast, 1);
    zero_memory(&u16_t_ast, 1);
    zero_memory(&u32_t_ast, 1);
    zero_memory(&u64_t_ast, 1);
    zero_memory(&s8_t_ast, 1);
    zero_memory(&s16_t_ast, 1);
    zero_memory(&s32_t_ast, 1);
    zero_memory(&s64_t_ast, 1);
    zero_memory(&bool8_t_ast, 1);
    zero_memory(&bool16_t_ast, 1);
    zero_memory(&bool32_t_ast, 1);
    zero_memory(&bool64_t_ast, 1);
    zero_memory(&f32_t_ast, 1);
    zero_memory(&f64_t_ast, 1);
    zero_memory(&void_t_ast, 1);
    zero_memory(&type_t_ast, 1);
    zero_memory(&intlike_t_ast, 1);
    zero_memory(&floatlike_t_ast, 1);
    zero_memory(&numberlike_t_ast, 1);
    zero_memory(&boollike_t_ast, 1);
    
    u8_t_ast.type = AST_Type::primitive_ast;
    u16_t_ast.type = AST_Type::primitive_ast;
    u32_t_ast.type = AST_Type::primitive_ast;
    u64_t_ast.type = AST_Type::primitive_ast;
    s8_t_ast.type = AST_Type::primitive_ast;
    s16_t_ast.type = AST_Type::primitive_ast;
    s32_t_ast.type = AST_Type::primitive_ast;
    s64_t_ast.type = AST_Type::primitive_ast;
    bool8_t_ast.type = AST_Type::primitive_ast;
    bool16_t_ast.type = AST_Type::primitive_ast;
    bool32_t_ast.type = AST_Type::primitive_ast;
    bool64_t_ast.type = AST_Type::primitive_ast;
    f32_t_ast.type = AST_Type::primitive_ast;
    f64_t_ast.type = AST_Type::primitive_ast;
    void_t_ast.type = AST_Type::primitive_ast;
    type_t_ast.type = AST_Type::primitive_ast;
    intlike_t_ast.type = AST_Type::primitive_ast;
    floatlike_t_ast.type = AST_Type::primitive_ast;
    numberlike_t_ast.type = AST_Type::primitive_ast;
    boollike_t_ast.type = AST_Type::primitive_ast;
    
    u8_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    u16_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    u32_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    u64_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    s8_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    s16_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    s32_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    s64_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    bool8_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    bool16_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    bool32_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    bool64_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    f32_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    f64_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    void_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    type_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    intlike_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    floatlike_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    numberlike_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    boollike_t_ast.flags |= AST_FLAG_SYNTHETIC | AST_FLAG_TYPE_INFERRED_TRANSITIVE;
    
    u8_t_ast.s = next_serial++;
    u16_t_ast.s = next_serial++;
    u32_t_ast.s = next_serial++;
    u64_t_ast.s = next_serial++;
    s8_t_ast.s = next_serial++;
    s16_t_ast.s = next_serial++;
    s32_t_ast.s = next_serial++;
    s64_t_ast.s = next_serial++;
    bool8_t_ast.s = next_serial++;
    bool16_t_ast.s = next_serial++;
    bool32_t_ast.s = next_serial++;
    bool64_t_ast.s = next_serial++;
    f32_t_ast.s = next_serial++;
    f64_t_ast.s = next_serial++;
    void_t_ast.s = next_serial++;
    type_t_ast.s = next_serial++;
    intlike_t_ast.s = next_serial++;
    floatlike_t_ast.s = next_serial++;
    numberlike_t_ast.s = next_serial++;
    boollike_t_ast.s = next_serial++;
    
    u8_t_ast.resolved_type = &type_t_ast;
    u16_t_ast.resolved_type = &type_t_ast;
    u32_t_ast.resolved_type = &type_t_ast;
    u64_t_ast.resolved_type = &type_t_ast;
    s8_t_ast.resolved_type = &type_t_ast;
    s16_t_ast.resolved_type = &type_t_ast;
    s32_t_ast.resolved_type = &type_t_ast;
    s64_t_ast.resolved_type = &type_t_ast;
    bool8_t_ast.resolved_type = &type_t_ast;
    bool16_t_ast.resolved_type = &type_t_ast;
    bool32_t_ast.resolved_type = &type_t_ast;
    bool64_t_ast.resolved_type = &type_t_ast;
    f32_t_ast.resolved_type = &type_t_ast;
    f64_t_ast.resolved_type = &type_t_ast;
    void_t_ast.resolved_type = &type_t_ast;
    type_t_ast.resolved_type = &type_t_ast;
    intlike_t_ast.resolved_type = &type_t_ast;
    floatlike_t_ast.resolved_type = &type_t_ast;
    numberlike_t_ast.resolved_type = &type_t_ast;
    boollike_t_ast.resolved_type = &type_t_ast;
    
    u8_t_ast.canonical_form = &u8_t_ast;
    u16_t_ast.canonical_form = &u16_t_ast;
    u32_t_ast.canonical_form = &u32_t_ast;
    u64_t_ast.canonical_form = &u64_t_ast;
    s8_t_ast.canonical_form = &s8_t_ast;
    s16_t_ast.canonical_form = &s16_t_ast;
    s32_t_ast.canonical_form = &s32_t_ast;
    s64_t_ast.canonical_form = &s64_t_ast;
    bool8_t_ast.canonical_form = &bool8_t_ast;
    bool16_t_ast.canonical_form = &bool16_t_ast;
    bool32_t_ast.canonical_form = &bool32_t_ast;
    bool64_t_ast.canonical_form = &bool64_t_ast;
    f32_t_ast.canonical_form = &f32_t_ast;
    f64_t_ast.canonical_form = &f64_t_ast;
    void_t_ast.canonical_form = &void_t_ast;
    type_t_ast.canonical_form = &type_t_ast;
    intlike_t_ast.canonical_form = &intlike_t_ast;
    floatlike_t_ast.canonical_form = &floatlike_t_ast;
    numberlike_t_ast.canonical_form = &numberlike_t_ast;
    boollike_t_ast.canonical_form = &boollike_t_ast;
    
    u8_t_ast.primitive = PRIM_U8;
    u16_t_ast.primitive = PRIM_U16;
    u32_t_ast.primitive = PRIM_U32;
    u64_t_ast.primitive = PRIM_U64;
    s8_t_ast.primitive = PRIM_S8;
    s16_t_ast.primitive = PRIM_S16;
    s32_t_ast.primitive = PRIM_S32;
    s64_t_ast.primitive = PRIM_S64;
    bool8_t_ast.primitive = PRIM_BOOL8;
    bool16_t_ast.primitive = PRIM_BOOL16;
    bool32_t_ast.primitive = PRIM_BOOL32;
    bool64_t_ast.primitive = PRIM_BOOL64;
    f32_t_ast.primitive = PRIM_F32;
    f64_t_ast.primitive = PRIM_F64;
    void_t_ast.primitive = PRIM_VOID;
    type_t_ast.primitive = PRIM_TYPE;
    intlike_t_ast.primitive = PRIM_INTLIKE;
    floatlike_t_ast.primitive = PRIM_FLOATLIKE;
    numberlike_t_ast.primitive = PRIM_NUMBERLIKE;
    boollike_t_ast.primitive = PRIM_BOOLLIKE;
}

AST* construct_ast_(AST *new_ast, AST_Type type, u64 line_number, u64 line_offset)
{
    new_ast->type = type;
    new_ast->flags = 0;
    new_ast->s = next_serial++;
    new_ast->line_number = line_number;
    new_ast->line_offset = line_offset;
    return new_ast;
}


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
            String str = *ident_ast->atom.str;
            print_buf(pb, "n%ld[label=\"%.*s\"];\n", s, str.count, str.data);
        } break;
        case AST_Type::decl_ast: {
            Decl_AST *decl_ast = static_cast<Decl_AST*>(ast);
            
            String str = *decl_ast->ident.atom.str;
            print_buf(pb, "n%ld[label=\"Declare %.*s\"];\nn%ld[shape=box];\n", s, str.count, str.data, s);
            if(decl_ast->decl_type)
            {
                print_dot_child(pb, decl_ast->decl_type, s);
            }
            if(decl_ast->expr)
            {
                print_dot_child(pb, decl_ast->expr, s);
            }
        } break;
        case AST_Type::block_ast: {
            Block_AST *block_ast = static_cast<Block_AST*>(ast);
            
            print_buf(pb, "n%ld[label=\"Block\"];\nn%ld[shape=box];\n", s, s);
            
            for(u64 i = 0; i < block_ast->statements.count; ++i)
            {
                print_dot_child(pb, block_ast->statements[i], s);
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
        case AST_Type::access_ast: {
            Access_AST *access_ast = static_cast<Access_AST*>(ast);
            print_buf(pb, "n%ld[label=\".\"];\n", s);
            print_dot_child(pb, access_ast->lhs, s);
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
            print_dot_child(pb, assign_ast->lhs, s);
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
