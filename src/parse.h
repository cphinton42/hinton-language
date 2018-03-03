#ifndef PARSE_H
#define PARSE_H

#include "basic.h"
#include "io.h"
#include "pool_allocator.h"

struct Parsing_Context
{
    String program_text;
    Array<Token> tokens;
    Pool_Allocator *ast_pool;
};

enum class AST_Type : u16
{
    def_ident_ast,
    refer_ident_ast,
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
constexpr u16 AST_FLAG_TYPECHECKED = 2;

constexpr u16 DECL_FLAG_CONSTANT = 4;

constexpr u16 FOR_FLAG_BY_POINTER = 4;
constexpr u16 FOR_FLAG_OVER_ARRAY = 8;

constexpr u16 EXPR_FLAG_CONSTANT = 4;

constexpr u16 NUMBER_FLAG_FLOATLIKE = 8;


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

struct Expr_AST : AST
{
    Expr_AST *resolved_type;
};

// TODO: audit these and convert to them
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


extern Primitive_AST u8_t_ast;
extern Primitive_AST u16_t_ast;
extern Primitive_AST u32_t_ast;
extern Primitive_AST u64_t_ast;
extern Primitive_AST s8_t_ast;
extern Primitive_AST s16_t_ast;
extern Primitive_AST s32_t_ast;
extern Primitive_AST s64_t_ast;
extern Primitive_AST bool8_t_ast;
extern Primitive_AST bool16_t_ast;
extern Primitive_AST bool32_t_ast;
extern Primitive_AST bool64_t_ast;
extern Primitive_AST f32_t_ast;
extern Primitive_AST f64_t_ast;
extern Primitive_AST void_t_ast;
extern Primitive_AST type_t_ast;
extern Primitive_AST intlike_t_ast;
extern Primitive_AST floatlike_t_ast;
extern Primitive_AST numberlike_t_ast;
extern Primitive_AST boollike_t_ast;


struct Def_Ident_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::def_ident_ast;
    
    String ident;
};

struct Refer_Ident_AST : Expr_AST
{
    static constexpr AST_Type type_value = AST_Type::refer_ident_ast;
    
    String ident;
    // Note: referred_to is probably mutually exclusive with parent
    AST *referred_to;
    Parent_Scope parent;
};

// Note: not actually part of the AST structure, just used during parsing
struct Parameter_AST
{
    Def_Ident_AST *name;
    Expr_AST *type;
    Expr_AST *default_value;
};

struct Decl_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::decl_ast;
    
    Def_Ident_AST ident;
    Expr_AST *decl_type;
    Expr_AST *expr;
    Parent_Scope parent;
};

struct Block_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::block_ast;
    
    Array<AST*> statements;
    Parent_Scope parent;
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
    Array<Def_Ident_AST*> param_names;
    Array<Expr_AST*> default_values;
    Block_AST *block;
    Parent_Scope parent;
};

struct Number_AST : Expr_AST
{
    static constexpr AST_Type type_value = AST_Type::number_ast;
    
    String literal;
    union
    {
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
    
    Def_Ident_AST *induction_var;
    union
    {
        struct
        {
            Def_Ident_AST *index_var;
            Expr_AST *array_expr;
        };
        struct
        {
            Expr_AST *low_expr;
            Expr_AST *high_expr;
        };
    };
    Block_AST *body;
    Parent_Scope parent;
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
};

struct Enum_AST : Expr_AST
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

// TODO: this should actually work for all l-values, not just identifiers
struct Assign_AST : AST
{
    static constexpr AST_Type type_value = AST_Type::assign_ast;
    
    Refer_Ident_AST ident;
    Assign_Operator assign_type;
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
void init_parsing_context(Parsing_Context *ctx, String program_text, Array<Token> tokens, Pool_Allocator *ast_pool);

Dynamic_Array<Decl_AST*> parse_tokens(Parsing_Context *ctx);
void print_dot(Print_Buffer *pb, Array<Decl_AST*> decls);

AST* construct_ast_(AST *new_ast, AST_Type type, u64 line_number, u64 line_offset);

#define construct_ast(pool, type, line_number, line_offset) \
(static_cast<type*>(construct_ast_(pool_alloc(type,(pool),1),type::type_value, (line_number), (line_offset))))


#endif // PARSE_H
