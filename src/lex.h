#ifndef LEX_H
#define LEX_H

enum class Token_Type : u64
{
    ident,
    number,
    key_enum,
    key_for,
    key_if,
    key_struct,
    
    comma,
    semicolon,
    colon,
    colon_eq,
    double_colon,
    dot,
    arrow,
    
    open_paren,
    close_paren,
    open_sqr,
    close_sqr,
    open_brace,
    close_brace,
    
    mul,
    add,
    sub,
    bor,
    lor,
    band,
    land,
    div,
    equal,
    double_equal,
    mul_eq,
    add_eq,
    sub_eq,
    div_eq,
    eof,
};

const byte *token_type_names[] = {
    "identifier",
    "number",
    "enum",
    "for",
    "if",
    "struct",
    ",",
    ";",
    ":",
    ":=",
    "::",
    ".",
    "->",
    "(",
    ")",
    "[",
    "]",
    "{",
    "}",
    "*",
    "+",
    "-",
    "|",
    "||",
    "&",
    "&&",
    "/",
    "=",
    "==",
    "*=",
    "+=",
    "-=",
    "/=",
    "EOF"
};

struct Token
{
    Token_Type type;
    u32 line_number;
    u32 line_offset;
    String contents;
};

Dynamic_Array<Token> lex_string(String file_contents);

#endif // LEX_H