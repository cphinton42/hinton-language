#ifndef AST_H
#define AST_H

#include "basic.h"
#include "io.h"
#include "scope.h"

enum class AST_Type : u16
{
    decl_ast,
    block_ast,
    while_ast,
    for_ast,
    if_ast,
    assign_ast,
    return_ast,
    
    ident_ast,
    function_type_ast,
    function_ast,
    function_call_ast,
    access_ast,
    binary_operator_ast,
    number_ast,
    enum_ast,
    struct_ast,
    unary_ast,
    primitive_ast,
    string_ast,
    bool_ast,
};

const char *ast_type_names[] = {
    "decl",
    "block",
    "while",
    "for",
    "if",
    "assign",
    "return",
    
    "ident",
    "function_type",
    "function",
    "function_call",
    "access",
    "binary_operator",
    "number",
    "enum",
    "struct",
    "unary",
    "primitive",
    "string",
    "bool",
};


constexpr u16 AST_FLAG_SYNTHETIC = 0x1;
constexpr u16 AST_FLAG_CHECK_COMPLETE = 0x2;

constexpr u16 DECL_FLAG_CONSTANT = 0x4;

constexpr u16 FOR_FLAG_BY_POINTER = 0x4;
constexpr u16 FOR_FLAG_OVER_ARRAY = 0x8;

constexpr u16 EXPR_FLAG_CONSTANT = 0x4;
constexpr u16 EXPR_FLAG_COMPILE_TIME_CONSTANT = 0x8;
constexpr u16 EXPR_FLAG_LVALUE = 0x10;
constexpr u16 EXPR_FLAG_NOT_LVALUE = 0x20;
constexpr u16 EXPR_FLAG_LVALUE_MASK = 0x30;
constexpr u16 NUMBER_FLAG_FLOATLIKE = 0x40;
constexpr u16 TYPE_FLAG_EVALUATED = 0x40;
constexpr u16 TYPE_FLAG_CANONICAL = 0x80; // TODO: is this needed?



struct AST
{
    AST_Type type;
    u16 flags;
    u32 s;
    u32 line_number;
    u32 line_offset;
};

u32 next_serial = 0;

struct Expr_AST : AST
{
    union {
        Array<Expr_AST*> types;
        struct {
            u64 types_count;
            Expr_AST *resolved_type;
        };
    };
};

constexpr u64 PRIM_NO_SIZE = 0;
constexpr u64 PRIM_SIZE1 = 0x1;
constexpr u64 PRIM_SIZE2 = 0x2;
constexpr u64 PRIM_SIZE4 = 0x3;
constexpr u64 PRIM_SIZE8 = 0x4;
constexpr u64 PRIM_SIZE_MASK = 0x7;
// Note: Internal number types may be abstract and don't specify sign
constexpr u64 PRIM_SIGNED_INT = 0x8;
constexpr u64 PRIM_UNSIGNED_INT = 0x10;
constexpr u64 PRIM_SIGN_MASK = 0x18;

constexpr u64 PRIM_INTLIKE = 0x20;
constexpr u64 PRIM_FLOATLIKE = 0x40;
constexpr u64 PRIM_NUMBERLIKE = 0x60;
constexpr u64 PRIM_BOOLLIKE = 0x80;
constexpr u64 PRIM_LIKE_MASK = 0xE0;

constexpr u64 PRIM_S8 = 0x29;
constexpr u64 PRIM_S16 = 0x2A;
constexpr u64 PRIM_S32 = 0x2B;
constexpr u64 PRIM_S64 = 0x2C;
constexpr u64 PRIM_INT = 0x2C;

constexpr u64 PRIM_U8 = 0x31;
constexpr u64 PRIM_U16 = 0x32;
constexpr u64 PRIM_U32 = 0x33;
constexpr u64 PRIM_U64 = 0x34;
constexpr u64 PRIM_UINT = 0x34;

constexpr u64 PRIM_F32 = 0x43;
constexpr u64 PRIM_F64 = 0x44;

constexpr u64 PRIM_BOOL = 0x81;
constexpr u64 PRIM_BOOL8 = 0x81;
constexpr u64 PRIM_BOOL16 = 0x82;
constexpr u64 PRIM_BOOL32 = 0x83;
constexpr u64 PRIM_BOOL64 = 0x84;

constexpr u64 PRIM_VOID = 0;
constexpr u64 PRIM_TYPE = 0x100;

const byte *primitive_names[] = {
    "void", // 0
    "","","","","","","","","","","","","","","","", // 0x01-0x10
    "","","","","","","","","","","","","","","","", // 0x11-0x20
    "","","","","","","","", // 0x21-0x28
    "s8",
    "s16",
    "s32",
    "s64",
    "","","","", // 0x2D-0x30
    "u8",
    "u16",
    "u32",
    "u64",
    "","","","","","","","","","","","","","", // 0x34-0x42
    "f32",
    "f64",
    "","","","","","","","","","","","", // 0x45-0x50
    "","","","","","","","","","","","","","","","", // 0x51-0x60
    "","","","","","","","","","","","","","","","", // 0x61-0x70
    "","","","","","","","","","","","","","","","", // 0x71-0x80
    "bool8",
    "bool16",
    "bool32",
    "bool64",
};

struct Primitive_AST : Expr_AST
{
    static constexpr AST_Type type_value = AST_Type::primitive_ast;
    
    u64 primitive;
};


Primitive_AST u8_t_ast;
Primitive_AST u16_t_ast;
Primitive_AST u32_t_ast;
Primitive_AST u64_t_ast;
Primitive_AST s8_t_ast;
Primitive_AST s16_t_ast;
Primitive_AST s32_t_ast;
Primitive_AST s64_t_ast;
Primitive_AST bool8_t_ast;
Primitive_AST bool16_t_ast;
Primitive_AST bool32_t_ast;
Primitive_AST bool64_t_ast;
Primitive_AST f32_t_ast;
Primitive_AST f64_t_ast;
Primitive_AST void_t_ast;
Primitive_AST type_t_ast;

// TODO: remove these
Primitive_AST intlike_t_ast; 
Primitive_AST floatlike_t_ast;
Primitive_AST numberlike_t_ast;
Primitive_AST boollike_t_ast;

declare_static_array(Expr_AST*, number_type_asts, 10) = {
    &u8_t_ast,
    &u16_t_ast,
    &u32_t_ast,
    &u64_t_ast,
    &s8_t_ast,
    &s16_t_ast,
    &s32_t_ast,
    &s64_t_ast,
    &f32_t_ast,
    &f64_t_ast,
};
declare_static_array(Expr_AST*, int_type_asts, 8) = {
    &u8_t_ast,
    &u16_t_ast,
    &u32_t_ast,
    &u64_t_ast,
    &s8_t_ast,
    &s16_t_ast,
    &s32_t_ast,
    &s64_t_ast,
};
declare_static_array(Expr_AST*, sint_type_asts, 4) = {
    &s8_t_ast,
    &s16_t_ast,
    &s32_t_ast,
    &s64_t_ast,
};
declare_static_array(Expr_AST*, uint_type_asts, 4) = {
    &u8_t_ast,
    &u16_t_ast,
    &u32_t_ast,
    &u64_t_ast,
};
declare_static_array(Expr_AST*, float_type_asts, 2) = {
    &f32_t_ast,
    &f64_t_ast,
};
declare_static_array(Expr_AST*, bool_type_asts, 4) = {
    &bool8_t_ast,
    &bool16_t_ast,
    &bool32_t_ast,
    &bool64_t_ast,
};

constexpr u64 IDENT_UNKNOWN = 0;
constexpr u64 IDENT_REFERENCE = 1;
constexpr u64 IDENT_DECL = 2;
constexpr u64 IDENT_FIELD = 3;
constexpr u64 IDENT_LOOP_VAR = 4;
constexpr u64 IDENT_PARAM = 5;
constexpr u64 IDENT_TYPE_MASK = 7;

struct Ident_AST : Expr_AST
{
    static constexpr AST_Type type_value = AST_Type::ident_ast;
    
    union {
        String ident;
        struct {
            Atom atom;
            u64 tagged_expr_ptr;
        };
    };
    u64 scope_index;
    Hashed_Scope *scope;
};

inline
u64 ident_get_type(Ident_AST *ident)
{
    return ident->tagged_expr_ptr & IDENT_TYPE_MASK;
}
inline
Expr_AST *ident_get_expr(Ident_AST *ident)
{
    return (Expr_AST*)(ident->tagged_expr_ptr & ~IDENT_TYPE_MASK);
}
inline
void ident_set_expr(Ident_AST *ident, Expr_AST *expr)
{
    ident->tagged_expr_ptr &= IDENT_TYPE_MASK;
    ident->tagged_expr_ptr |= (u64)expr;
}


// Note: not actually part of the AST structure, just used during parsing
struct Parameter_AST
{
    Ident_AST *name;
    Expr_AST *type;
    Expr_AST *default_value;
};

struct Decl_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::decl_ast;
    
    Ident_AST ident;
    Expr_AST *decl_type;
    Expr_AST *expr;
};

struct Block_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::block_ast;
    
    Array<AST*> statements;
};

struct Function_Type_AST : Expr_AST
{
    static constexpr AST_Type type_value = AST_Type::function_type_ast;
    
    Array<Expr_AST*> parameter_types;
    Array<Expr_AST*> return_types;
};

struct Function_AST : Expr_AST
{
    static constexpr AST_Type type_value = AST_Type::function_ast;
    
    Function_Type_AST *prototype;
    Array<Ident_AST*> param_names;
    Array<Expr_AST*> default_values;
    Block_AST *block;
};

struct Number_AST : Expr_AST
{
    static constexpr AST_Type type_value = AST_Type::number_ast;
    
    String literal;
    union {
        u64 int_value;
        f64 float_value;
    };
};

struct String_AST : Expr_AST
{
    static constexpr AST_Type type_value = AST_Type::string_ast;
    
    String literal;
    String value;
};

struct Bool_AST : Expr_AST
{
    static constexpr AST_Type type_value = AST_Type::bool_ast;
    
    bool value;
};

struct Access_AST : Expr_AST
{
    static constexpr AST_Type type_value = AST_Type::access_ast;
    
    Expr_AST *lhs;
    union {
        String ident;
        struct {
            Atom atom;
            Expr_AST *expr;
        };
    };
};

enum class Binary_Operator : u64
{
    cmp_eq,
    cmp_neq,
    cmp_lt,
    cmp_le,
    cmp_gt,
    cmp_ge,
    add,
    sub,
    mul,
    div,
    subscript,
    lor,
    land,
};

const byte *binary_operator_names[] = {
    "+",
    "-",
    "*",
    "/",
    "[]",
    "||",
    "&&",
};

struct Binary_Operator_AST : Expr_AST
{
    static constexpr AST_Type type_value = AST_Type::binary_operator_ast;
    
    Binary_Operator op;
    Expr_AST *lhs;
    Expr_AST *rhs;
};

struct Function_Call_AST : Expr_AST
{
    static constexpr AST_Type type_value = AST_Type::function_call_ast;
    
    Expr_AST *function;
    Array<Expr_AST*> args;
};

struct While_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::while_ast;
    
    Expr_AST *guard;
    Block_AST *body;
};


struct For_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::for_ast;
    
    Ident_AST *induction_var;
    union {
        struct {
            Ident_AST *index_var;
            Expr_AST *array_expr;
        };
        struct {
            Expr_AST *low_expr;
            Expr_AST *high_expr;
        };
    };
    Block_AST *body;
};

struct If_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::if_ast;
    
    Expr_AST *guard;
    Block_AST *then_block;
    Block_AST *else_block;
};

struct Struct_AST : Expr_AST
{
    static constexpr AST_Type type_value = AST_Type::struct_ast;
    
    Array<Decl_AST*> constants;
    Array<Decl_AST*> fields;
    Hashed_Scope *constant_scope;
    Hashed_Scope *field_scope;
};

struct Enum_AST : Expr_AST
{
    static constexpr AST_Type type_value = AST_Type::enum_ast;
    
    Array<Decl_AST*> values;
    Hashed_Scope *scope;
};

enum class Assign_Operator : u64
{
    equal,
    mul_eq,
    add_eq,
    sub_eq,
    div_eq
};

const byte *assign_names[] = {
    "=",
    "*=",
    "+=",
    "-=",
    "/=",
};


struct Assign_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::assign_ast;
    
    Assign_Operator assign_type;
    // Note: lhs must be an l-value
    Expr_AST *lhs;
    Expr_AST *rhs;
};

enum class Unary_Operator : u64
{
    plus,
    minus,
    deref,
    ref,
    lnot,
};

const byte *unary_operator_names[] = {
    "+",
    "-",
    "*",
    "&",
    "!",
};

struct Unary_Operator_AST : Expr_AST
{
    static constexpr AST_Type type_value = AST_Type::unary_ast;
    
    Unary_Operator op;
    Expr_AST *operand;
};

struct Return_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::return_ast;
    
    Function_AST *function;
    Expr_AST *expr;
};

void init_primitive_types();
void print_dot(Print_Buffer *pb, Array<Decl_AST*> decls);

AST* construct_ast_(AST *new_ast, AST_Type type, u64 line_number, u64 line_offset);

#define construct_ast(pool, type, line_number, line_offset) \
(static_cast<type*>(construct_ast_(pool_alloc3(type,1,pool),type::type_value, (line_number), (line_offset))))


#define case_non_exprs \
case AST_Type::decl_ast: \
case AST_Type::block_ast: \
case AST_Type::while_ast: \
case AST_Type::for_ast: \
case AST_Type::if_ast: \
case AST_Type::assign_ast: \
case AST_Type::return_ast

#define case_exprs \
case AST_Type::ident_ast: \
case AST_Type::function_type_ast: \
case AST_Type::function_ast: \
case AST_Type::function_call_ast: \
case AST_Type::access_ast: \
case AST_Type::binary_operator_ast: \
case AST_Type::number_ast: \
case AST_Type::enum_ast: \
case AST_Type::struct_ast: \
case AST_Type::unary_ast: \
case AST_Type::primitive_ast: \
case AST_Type::string_ast: \
case AST_Type::bool_ast


/* Example switch statements to copy-paste.
TODO: Visitor pattern won't work, is there something else?

switch(ast->type)
        {
            case AST_Type::decl_ast: {
                Decl_AST *decl_ast = static_cast<Decl_AST*>(ast);
            } break;
            case AST_Type::block_ast: {
                Block_AST *block_ast = static_cast<Block_AST*>(ast);
            } break;
            case AST_Type::while_ast: {
                While_AST *while_ast = static_cast<While_AST*>(ast);
            } break;
            case AST_Type::for_ast: {
                For_AST *for_ast = static_cast<For_AST*>(ast);
            } break;
            case AST_Type::if_ast: {
                If_AST *if_ast = static_cast<If_AST*>(ast);
            } break;
            case AST_Type::assign_ast: {
                Assign_AST *assign_ast = static_cast<Assign_AST*>(ast);
            } break;
            case AST_Type::return_ast: {
                Return_AST *return_ast = static_cast<Return_AST*>(ast);
            } break;
            
            case AST_Type::ident_ast: {
                Ident_AST *ident_ast = static_cast<Ident_AST*>(ast);
            } break;
            case AST_Type::function_type_ast: {
                Function_Type_AST *function_type_ast = static_cast<Function_Type_AST*>(ast);
            } break;
            case AST_Type::function_ast: {
                Function_AST *function_ast = static_cast<Function_AST*>(ast);
            } break;
            case AST_Type::function_call_ast: {
                Function_Call_AST *function_call_ast = static_cast<Function_Call_AST*>(ast);
            } break;
            case AST_Type::access_ast: {
            Access_AST *access_ast = static_cast<Access_AST*>(ast);
            } break;
            case AST_Type::binary_operator_ast: {
                Binary_Operator_AST *binop_ast = static_cast<Binary_Operator_AST*>(ast);
            } break;
            case AST_Type::number_ast: {
                Number_AST *number_ast = static_cast<Number_AST*>(ast);
            } break;
            case AST_Type::enum_ast: {
                Enum_AST *enum_ast = static_cast<Enum_AST*>(ast);
            } break;
            case AST_Type::struct_ast: {
                Struct_AST *struct_ast = static_cast<Struct_AST*>(ast);
            } break;
            case AST_Type::unary_ast: {
                Unary_Operator_AST *unop_ast = static_cast<Unary_Operator_AST*>(ast);
            } break;
            case AST_Type::primitive_ast: {
                Primitive_AST *prim_ast = static_cast<Primitive_AST*>(ast);
            } break;
            case AST_Type::string_ast: {
                String_AST *string_ast = static_cast<String_AST*>(ast);
            } break;
            case AST_Type::bool_ast: {
                Bool_AST *bool_ast = static_cast<Bool_AST*>(ast);
            } break;
        }
        
        switch(ast->type)
        {
            case AST_Type::decl_ast:
    case AST_Type::block_ast:
    case AST_Type::while_ast:
    case AST_Type::for_ast:
    case AST_Type::if_ast:
    case AST_Type::assign_ast:
    case AST_Type::return_ast:
    
    case AST_Type::ident_ast:
    case AST_Type::function_type_ast:
    case AST_Type::function_ast:
    case AST_Type::function_call_ast:
    case AST_Type::access_ast:
    case AST_Type::binary_operator_ast:
    case AST_Type::number_ast:
    case AST_Type::enum_ast:
    case AST_Type::struct_ast:
    case AST_Type::unary_ast:
    case AST_Type::primitive_ast:
    case AST_Type::string_ast:
    case AST_Type::bool_ast:
        }
        
*/

#endif // AST_H