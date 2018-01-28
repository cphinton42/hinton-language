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

enum class AST_Type : u32
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
};

constexpr u32 DECL_FLAG_CONSTANT = 1;

struct AST
{
    AST_Type type;
    u32 flags;
    u32 line_number;
    u32 line_offset;
};

struct Ident_AST : AST
{
    String ident;
};

// Note: not actually part of the AST structure, just used during parsing
struct Parameter_AST
{
    Ident_AST *name;
    AST *type;
};

struct Decl_AST : AST
{
    Ident_AST ident;
    AST *decl_type;
    AST *expr;
};

struct Block_AST : AST
{
    Array<AST*> statements;
};

struct Function_Type_AST : AST
{
    Array<AST*> parameter_types;
    Array<AST*> return_types;
};

struct Function_AST : AST
{
    Function_Type_AST *prototype;
    Array<Ident_AST*> param_names;
    Block_AST *block;
};

struct Number_AST : AST
{
    String literal;
};

enum class Binary_Operator : u64
{
    access,
    add,
    sub,
    mul,
    div,
    subscript,
};

const byte *binary_operator_names[] = {
    ".",
    "+",
    "-",
    "*",
    "/",
    "[]",
};

struct Binary_Operator_AST : AST
{
    Binary_Operator op;
    AST *lhs;
    AST *rhs;
};

struct Function_Call_AST : AST
{
    AST *function;
    Array<AST*> args;
};

struct While_AST : AST
{
    AST *guard;
    Block_AST *body;
};

struct For_AST : AST
{
    Block_AST *body;
};

struct If_AST : AST
{
    AST *guard;
    Block_AST *then_block;
    Block_AST *else_block;
};

struct Struct_AST : AST
{
    Array<Decl_AST*> constants;
    Array<Decl_AST*> fields;
};

struct Enum_AST : AST
{
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
};

const byte *unary_operator_names[] = {
    "+",
    "-",
    "*",
    "&",
};

struct Unary_Operator_AST : AST
{
    Unary_Operator op;
    AST *operand;
};


void init_parsing_context(Parsing_Context *ctx, String program_text, Array<Token> tokens, u64 pool_block_size);

Dynamic_Array<Decl_AST*> parse_tokens(Parsing_Context *ctx);
void print_dot(Print_Buffer *pb, Array<Decl_AST*> decls);

#endif // PARSE_H