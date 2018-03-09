#ifndef LEX_H
#define LEX_H

enum class Token_Type : u64
{
    ident,
    number,
    
    key_bool,
    key_bool8,
    key_bool16,
    key_bool32,
    key_bool64,
    key_else,
    key_enum,
    key_f32,
    key_f64,
    key_false,
    key_for,
    key_if,
    key_int,
    key_s8,
    key_s16,
    key_s32,
    key_s64,
    key_string,
    key_struct,
    key_true,
    key_type,
    key_u8,
    key_u16,
    key_u32,
    key_u64,
    key_uint,
    key_return,
    key_void,
    key_while,
    
    keywords_begin = key_bool,
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
    
    lt,
    le,
    gt,
    ge,
    mul,
    add,
    sub,
    lor,
    ref,
    land,
    div,
    lnot,
    not_equal,
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
X("bool") \
X("bool8") \
X("bool16") \
X("bool32") \
X("bool64") \
X("else") \
X("enum") \
X("f32") \
X("f64") \
X("false") \
X("for") \
X("if") \
X("int") \
X("s8") \
X("s16") \
X("s32") \
X("s64") \
X("string") \
X("struct") \
X("true") \
X("type") \
X("u8") \
X("u16") \
X("u32") \
X("u64") \
X("uint") \
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
X("<") \
X("<=") \
X(">") \
X(">=") \
X("*") \
X("+") \
X("-") \
X("||") \
X("&") \
X("&&") \
X("/") \
X("!") \
X("!=") \
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

// Note: performance: could halve size by using relative offsets for string and locating source information elsewhere
struct Token
{
    Token_Type type;
    u32 line_number;
    u32 line_offset;
    String contents;
};

Dynamic_Array<Token> lex_string(String file_contents);

#endif // LEX_H