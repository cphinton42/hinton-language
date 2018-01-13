
// TODO: remove stdio dependency
#include "stdio.h"

Dynamic_Array<Token> lex_string(String file_contents)
{
    Dynamic_Array<Token> result;
    /* Note: do we want a more conservative guess about the number of tokens
    *  Right now we just allocate as many tokens as there are characters
    */
    result.count = 0;
    result.data = mem_alloc(Token, file_contents.count);
    result.allocated = file_contents.count;
    
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

