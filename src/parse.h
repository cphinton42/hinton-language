#ifndef PARSE_H
#define PARSE_H

#include "basic.h"
#include "io.h"

struct Parsing_Context
{
    String program_text;
    Token *begin_tokens;
    Token *end_tokens;
    
    Token *start_section;
    Token *current;
};

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

struct Ident_AST : AST
{
    String ident;
};

struct Decl_AST : AST
{
    Ident_AST ident;
    AST *decl_type;
    AST *expr;
};

struct Block_AST : AST
{
    u64 n_statements;
    AST *statements;
};

struct Function_AST : AST
{
    u64 n_arguments;
    Ident_AST *arg_names;
    AST *arg_types;
    Block_AST block;
};

struct Number_AST : AST
{
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

Parsing_Context make_parsing_context(String program_text, Array<Token> tokens);
Dynamic_Array<Decl_AST> parse_tokens(Parsing_Context *ctx);
void print_dot(Print_Buffer *pb, Array<Decl_AST> decls);

#endif // PARSE_H