#ifndef LEX_H
#define LEX_H

enum class Token_Type : u64
{
    ident,
    number,
    
    key_else,
    key_enum,
    key_false,
    key_for,
    key_if,
    key_struct,
    key_true,
    key_return,
    key_void,
    key_while,
    
    keywords_begin = key_else,
    keywords_last = key_while,
    
    string,
    
    comma,
    semicolon,
    colon,
    colon_eq,
    double_colon,
    dot,
    double_dot,
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
    lor,
    ref,
    land,
    div,
    lnot,
    equal,
    double_equal,
    mul_eq,
    add_eq,
    sub_eq,
    div_eq,
    eof,
};

#define FOR_TOKEN_NAME(X) \
X("identifier") \
X("number") \
X("else") \
X("enum") \
X("false") \
X("for") \
X("if") \
X("struct") \
X("true") \
X("return") \
X("void") \
X("while") \
X("string") \
X(",") \
X(";") \
X(":") \
X(":=") \
X("::") \
X(".") \
X("..") \
X("->") \
X("(") \
X(")") \
X("[") \
X("]") \
X("{") \
X("}") \
X("*") \
X("+") \
X("-") \
X("||") \
X("&") \
X("&&") \
X("/") \
X("!") \
X("=") \
X("==") \
X("*=") \
X("+=") \
X("-=") \
X("/=") \
X("EOF")

#define X(s) str_lit(s),
String token_type_names[] = {
    FOR_TOKEN_NAME(X)
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