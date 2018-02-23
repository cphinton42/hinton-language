#ifndef PARSE_H
#define PARSE_H

#include "basic.h"
#include "io.h"
#include "pool_allocator.h"

struct Parsing_Context
{
    String program_text;
    Array<Token> tokens;
    Pool_Allocator ast_pool;
};

enum class AST_Type : u16
{
    ident_ast,
    decl_ast,
    block_ast,
    function_type_ast,
    function_ast,
    function_call_ast,
    binary_operator_ast,
    number_ast,
    while_ast,
    for_ast,
    if_ast,
    enum_ast,
    struct_ast,
    assign_ast,
    unary_ast,
    return_ast,
    primitive_ast,
    string_ast,
    bool_ast,
};

constexpr u16 AST_FLAG_SYNTHETIC = 1;

constexpr u16 DECL_FLAG_CONSTANT = 2;

constexpr u16 FOR_FLAG_BY_POINTER = 2;
constexpr u16 FOR_FLAG_OVER_ARRAY = 4;

constexpr u16 NUMBER_FLAG_FLOATLIKE = 2;


struct AST
{
    AST_Type type;
    u16 flags;
    u32 s;
    u32 line_number;
    u32 line_offset;
};

extern u32 next_serial;

struct Parent_Scope
{
    AST *ast;
    u64 index;
};

enum class Primitive_Type : u64
{
    size_1 = 0x1,
    size_2 = 0x2,
    size_4 = 0x3,
    size_8 = 0x4,
    size_mask = 0x7,
    sign_flag = 0x8,
    
    int_like = 0x10,
    bool_like = 0x20,
    float_like = 0x30,
    likeness_mask = 0x30,
    
    u8_t = 0x11,
    u16_t = 0x12,
    u32_t = 0x13,
    u64_t = 0x14,
    uint_t = 0x14,
    
    s8_t = 0x19,
    s16_t = 0x1A,
    s32_t = 0x1B,
    s64_t = 0x1C,
    int_t = 0x1C,
    
    bool_t = 0x21,
    bool8_t = 0x21,
    bool16_t = 0x22,
    bool32_t = 0x23,
    bool64_t = 0x24,
    
    f32_t = 0x3B,
    f64_t = 0x3C,
    
    void_t = 0x0,
};

const byte *primitive_names[] = {
    "void",
    "","","","","","","","","","","","","","","","",
    "u8",
    "u16",
    "u32",
    "u64",
    "","","","",
    "s8",
    "s16",
    "s32",
    "s64",
    "","","","",
    "bool8",
    "bool16",
    "bool32",
    "bool64",
    "","","","","","","","","","","","","","","","","","","","","",
    "f32",
    "f64",
};

struct Primitive_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::primitive_ast;
    
    Primitive_Type primitive;
};

struct Ident_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::ident_ast;
    
    String ident;
    // Note: referred_to is probably mutually exclusive with parent
    AST *referred_to;
    Parent_Scope parent;
};

// Note: not actually part of the AST structure, just used during parsing
struct Parameter_AST
{
    Ident_AST *name;
    AST *type;
    AST *default_value;
};

struct Decl_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::decl_ast;
    
    Ident_AST ident;
    AST *decl_type;
    AST *expr;
    Parent_Scope parent;
};

struct Block_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::block_ast;
    
    Array<AST*> statements;
    Parent_Scope parent;
};

struct Function_Type_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::function_type_ast;
    
    Array<AST*> parameter_types;
    Array<AST*> return_types;
};

struct Function_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::function_ast;
    
    Function_Type_AST *prototype;
    Array<Ident_AST*> param_names;
    Array<AST*> default_values;
    Block_AST *block;
    Parent_Scope parent;
};

struct Number_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::number_ast;
    
    String literal;
    union
    {
        u64 int_value;
        f64 float_value;
    };
};

struct String_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::string_ast;
    
    String literal;
    String value;
};

struct Bool_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::bool_ast;
    
    bool value;
};

enum class Binary_Operator : u64
{
    access,
    add,
    sub,
    mul,
    div,
    subscript,
    lor,
    land,
};

const byte *binary_operator_names[] = {
    ".",
    "+",
    "-",
    "*",
    "/",
    "[]",
    "||",
    "&&",
};

struct Binary_Operator_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::binary_operator_ast;
    
    Binary_Operator op;
    AST *lhs;
    AST *rhs;
};

struct Function_Call_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::function_call_ast;
    
    AST *function;
    Array<AST*> args;
};

struct While_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::while_ast;
    
    AST *guard;
    Block_AST *body;
};


struct For_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::for_ast;
    
    Ident_AST *induction_var;
    union
    {
        struct
        {
            Ident_AST *index_var;
            AST *array_expr;
        };
        struct
        {
            AST *low_expr;
            AST *high_expr;
        };
    };
    Block_AST *body;
    Parent_Scope parent;
};

struct If_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::if_ast;
    
    AST *guard;
    Block_AST *then_block;
    Block_AST *else_block;
};

struct Struct_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::struct_ast;
    
    Array<Decl_AST*> constants;
    Array<Decl_AST*> fields;
};

struct Enum_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::enum_ast;
    
    Array<Decl_AST*> values;
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
    
    Ident_AST ident;
    Assign_Operator assign_type;
    AST *rhs;
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

struct Unary_Operator_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::unary_ast;
    
    Unary_Operator op;
    AST *operand;
};

struct Return_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::return_ast;
    
    AST *expr;
};

void init_parsing_context(Parsing_Context *ctx, String program_text, Array<Token> tokens, u64 pool_block_size);

Dynamic_Array<Decl_AST*> parse_tokens(Parsing_Context *ctx);
void print_dot(Print_Buffer *pb, Array<Decl_AST*> decls);

#endif // PARSE_H
