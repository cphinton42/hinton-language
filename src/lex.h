#ifndef LEX_H
#define LEX_H

enum class Token_Type : u64
{
    ident,
    number,
    key_enum,
    key_for,
    key_struct,
    
    comma,
    semicolon,
    colon,
    colon_eq,
    double_colon,
    dot,
    
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
    add_eq,
    sub_eq,
    div_eq,
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