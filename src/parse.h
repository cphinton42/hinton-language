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
    call,
    subscript,
};

const byte *binary_operator_names[] = {
    ".",
    "+",
    "-",
    "*",
    "/",
    "()",
    "[]",
};

struct Binary_Operator_AST : AST
{
    Binary_Operator op;
    AST *lhs;
    AST *rhs;
};

void init_parsing_context(Parsing_Context *ctx, String program_text, Array<Token> tokens, u64 pool_block_size);

Dynamic_Array<Decl_AST> parse_tokens(Parsing_Context *ctx);
void print_dot(Print_Buffer *pb, Array<Decl_AST> decls);

#endif // PARSE_H