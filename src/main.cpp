
// TODO: remove stdio dependency
#include "stdio.h"

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


enum class AST_Type : u32
{
    function_ast,
    enum_ast,
    struct_ast,
};

struct AST
{
    AST_Type type;
    u32 flags;
};

// TODO: defer

String read_entire_file(const byte *file_name)
{
    String result = {0};
    
    FILE *f = fopen(file_name, "rb");
    if(!f)
    {
        return result;
    }
    
    fseek(f, 0, SEEK_END);
    u64 len = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    byte *memory = mem_alloc(byte, len+1);
    if(!memory)
    {
        fclose(f);
        return result;
    }
    
    u64 bytes_read = fread(memory, 1, len, f);
    if(bytes_read < len)
    {
        fclose(f);
        mem_dealloc(memory, len+1);
        return result;
    }
    
    result.count = len+1;
    result.data = memory;
    
    fclose(f);
    result[len] = '\0';
    
    return result;
}

void report_error(byte *program_text, u32 line_number, u32 line_offset, String error_text)
{
    byte *end = program_text;
    while(*end && *end != '\n')
    {
        ++end;
    }
    u64 len = end - program_text;
    
    byte arrow[line_offset+5];
    fill_memory(arrow, '-', line_offset+3);
    arrow[line_offset+3] = '^';
    arrow[line_offset+4] = '\0';
    
    // TODO: remove color codes when not outputing to a terminal
    // TODO: perhaps change to highlight in the program text, rather than use an arrow ?
    
    printf("\x1B[1;31mError\x1B[0m: %d:%d:\n    %.*s\n    %.*s\n%s\n", line_number, line_offset, (u32)error_text.count, error_text.data, (u32)len, program_text, arrow);
}


Dynamic_Array<Token> lex_file(String file_contents)
{
    Dynamic_Array<Token> result;
    /* Note: do we want a more conservative guess about the number of tokens
    *  Right now we just allocate as many tokens as there are characters
    */
    result.count = 0;
    result.data = mem_alloc(Token, file_contents.count);
    result.allocated - file_contents.count;
    
    byte *point = file_contents.data;
    u32 line_number = 1;
    u32 line_offset = 1;
    
    byte *start;
    byte c = *point;
    
    auto add_token = [&](Token_Type type, String contents) {
        array_add(&result, {type, line_number, line_offset, contents});
        line_offset += contents.count;
    };
    
    // TODO: other whitespace besides ' ', '\t', and '\n'?
    // TODO: \r\n ?
    while(c)
    {
        if(c == ' ' || c == '\t')
        {
            ++point;
            c = *point;
            
            ++line_offset;
        }
        else if(c == '\n')
        {
            ++point;
            c = *point;
            
            line_offset = 1;
            ++line_number;
        }
        // Try to skip comments, but we might get '/' or '/='
        else if(c == '/')
        {
            // This could be a token, save the start
            start = point;
            ++point;
            c = *point;
            
            
            if(c == '*')
            {
                // Begin multi-line comment
                ++point;
                c = *point;
                line_offset += 2; // 2 for the /*
                
                u32 comment_nesting = 1;
                
                while(true)
                {
                    if(c == '*')
                    {
                        ++point;
                        c = *point;
                        ++line_offset;
                        
                        if(c == '/')
                        {
                            ++point;
                            c = *point;
                            ++line_offset;
                            
                            --comment_nesting;
                            if(comment_nesting == 0)
                            {
                                break;
                            }
                        }
                    }
                    else if(c == '/')
                    {
                        ++point;
                        c = *point;
                        ++line_offset;
                        
                        if(c == '*')
                        {
                            ++point;
                            c = *point;
                            ++line_offset;
                            
                            ++comment_nesting;
                        }
                    }
                    else if(c == '\n')
                    {
                        ++point;
                        c = *point;
                        
                        line_offset = 1;
                        ++line_number;
                    }
                    else if(c)
                    {
                        ++point;
                        c = *point;
                        ++line_offset;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else if(c == '/')
            {
                // Begin EOL comment
                do
                {
                    ++point;
                    c = *point;
                }
                while(c && c != '\n');
            }
            else if(c == '=')
            {
                // '/='
                ++point;
                c = *point;
                
                add_token(Token_Type::div_eq, make_array(2, start));
            }
            else
            {
                // '/'
                add_token(Token_Type::div, make_array(1, start));
            }
        }
        else if(('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
        {
            start = point;
            
            do
            {
                ++point;
                c = *point;
            }
            while(('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9'));
            
            u64 len = point - start;
            String contents = make_array(len, start);
            Token_Type type = Token_Type::ident;
            
            String keywords[] = {
                str_lit("enum"),
                str_lit("for"),
                str_lit("struct"),
            };
            Token_Type key_types[] = {
                Token_Type::key_enum,
                Token_Type::key_for,
                Token_Type::key_struct
            };
            
            for(u64 i = 0; i < static_array_size(keywords); ++i)
            {
                if(contents == keywords[i])
                {
                    type = key_types[i];
                    break;
                }
            }
            
            add_token(type, contents);
        }
        else if('0' <= c && c <= '9')
        {
            // TODO: numbers that start with .
            
            start = point;
            
            do
            {
                ++point;
                c = *point;
            }
            while('0' <= c && c <= '9');
            
            if(c == '.')
            {
                do
                {
                    ++point;
                    c = *point;
                }
                while('0' <= c && c <= '9');
                
                if(c == 'e' || c == 'E')
                {
                    ++point;
                    c = *point;
                    if(c == '+' || c == '-')
                    {
                        ++point;
                        c = *point;
                    }
                    if(c < '0' || '9' < c)
                    {
                        u64 len = point - start;
                        line_offset += len;
                        report_error(start, line_number, line_offset, str_lit("Floating point literal cannot have an empty exponent."));
                        continue;
                    }
                    else
                    {
                        do
                        {
                            ++point;
                            c = *point;
                        }
                        while('0' <= c && c <= '9');
                    }
                }
            }
            
            u64 len = point - start;
            add_token(Token_Type::number, make_array(len, start));
        }
        else if(c == '+')
        {
            start = point;
            ++point;
            c = *point;
            
            if(c == '=')
            {
                ++point;
                c = *point;
                
                add_token(Token_Type::add_eq, make_array(2, start));
            }
            else
            {
                add_token(Token_Type::add, make_array(1, start));
            }
        }
        else if(c == '-')
        {
            start = point;
            ++point;
            c = *point;
            
            if(c == '=')
            {
                ++point;
                c = *point;
                
                add_token(Token_Type::sub_eq, make_array(2, start));
            }
            else
            {
                add_token(Token_Type::sub, make_array(1, start));
            }
        }
        else if(c == '=')
        {
            start = point;
            ++point;
            c = *point;
            
            if(c == '=')
            {
                ++point;
                c = *point;
                
                add_token(Token_Type::double_equal, make_array(2, start));
            }
            else
            {
                add_token(Token_Type::equal, make_array(1, start));
            }
        }
        else if(c == ',')
        {
            add_token(Token_Type::comma, make_array(1, point));
            
            ++point;
            c = *point;
        }
        else if(c == ';')
        {
            add_token(Token_Type::semicolon, make_array(1, point));
            
            ++point;
            c = *point;
        }
        else if(c == ':')
        {
            start = point;
            
            ++point;
            c = *point;
            
            if(c == ':')
            {
                ++point;
                c = *point;
                
                add_token(Token_Type::double_colon, make_array(2, start));
            }
            else if(c == '=')
            {
                ++point;
                c = *point;
                
                add_token(Token_Type::colon_eq, make_array(2, start));
            }
            else
            {
                add_token(Token_Type::colon, make_array(1, start));
            }
        }
        else
        {
            printf("Unexpected character\n");
            ++point;
            ++line_offset;
            c = *point;
        }
        
        
        
        /*
        identifier_start:
        c = *point;
        if(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'))
        {
            ++point;
            goto identifier_cont;
        }
        
        identifier_cont:
        c = *point;
        if(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9'))
        {
            ++point;
            goto identifier_cont;
        }
        else
        {
        
        }
        */
    }
    
    return result;
}

int main(int argc, char **argv)
{
    String file_contents = read_entire_file("test.txt");
    if(file_contents.data)
    {
        printf("%s\n", file_contents.data);
        
        Dynamic_Array<Token> tokens = lex_file(file_contents);
        for(u64 i = 0; i < tokens.count; ++i)
        {
            printf("Token: %ld, %d:%d, %.*s\n", (u64)tokens[i].type, tokens[i].line_number, tokens[i].line_offset, (u32)tokens[i].contents.count, tokens[i].contents.data);
        }
    }
    
    printf("Hello World!\n");
}
