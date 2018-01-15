#ifndef PARSE_H
#define PARSE_H

#include "basic.h"
#include "io.h"

// Want to keep program_text, begin_tokens, end_tokens constant
// start_section is only local
// current goes forward and back

struct Lexed_String
{
    String program_text;
    Dynamic_Array<Token> tokens;
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

const byte *binary_operator_names[] = {
    "+",
    "-",
    "*",
    "/"
};

struct Binary_Operator_AST : AST
{
    Binary_Operator op;
    AST *lhs;
    AST *rhs;
};

Dynamic_Array<Decl_AST> parse_tokens(Lexed_String *str);
void print_dot(Print_Buffer *pb, Array<Decl_AST> decls);

#endif // PARSE_H