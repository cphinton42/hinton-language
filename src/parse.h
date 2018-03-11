#ifndef PARSE_H
#define PARSE_H

#include "ast.h"
#include "basic.h"
#include "io.h"
#include "lex.h"
#include "pool_allocator.h"

struct Parsing_Context
{
    String program_text;
    Array<Token> tokens;
    Pool_Allocator *ast_pool;
};

void init_parsing_context(Parsing_Context *ctx, String program_text, Array<Token> tokens, Pool_Allocator *ast_pool);
Dynamic_Array<Decl_AST*> parse_tokens(Parsing_Context *ctx);

#endif // PARSE_H
