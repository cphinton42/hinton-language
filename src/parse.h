#ifndef PARSE_H
#define PARSE_H

#include "basic.h"

enum class AST_Type : u32
{
    ident_ast,
    decl_ast,
    block_ast,
    function_ast,
    binary_operator_ast,
    number_ast,
};

struct AST
{
    AST_Type type;
    u32 flags;
    u32 line_number;
    u32 line_offset;
};

struct Ident_AST
{
    AST base;
    String ident;
};

struct Decl_AST
{
    AST base;
    Ident_AST ident;
    AST *type;
    AST *expr;
};

struct Block_AST
{
    AST base;
    u64 n_statements;
    AST *statements;
};

struct Function_AST
{
    AST base;
    u64 n_arguments;
    Ident_AST *arg_names;
    AST *arg_types;
    Block_AST block;
};

struct Number_AST
{
    AST base;
    String literal;
};

enum class Binary_Operator : u64
{
    add,
    sub,
    mul,
    div
};

struct Binary_Operator_AST
{
    AST base;
    Binary_Operator op;
    AST *lhs;
    AST *rhs;
};

Dynamic_Array<Decl_AST> parse_tokens(Array<Token> tokens);

#endif // PARSE_H