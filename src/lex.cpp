
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
    
    // Note: add_token handles line_offset
    auto add_token = [&](Token_Type type, String contents) {
        array_add(&result, {type, line_number, line_offset, contents});
        line_offset += contents.count;
    };
    
    // TODO: other whitespace besides ' ', '\t', and '\n'?
    // TODO: \r\n ?
    while(c)
    {
        switch(c)
        {
            case ' ':
            case '\t': {
                c = *(++point);
                ++line_offset;
            } break;
            case '\r': {
                c = *(++point);
                if(c == '\n')
                {
                    c = *(++point);
                }
                line_offset = 1;
                ++line_number;
            } break;
            case '\n': {
                c = *(++point);
                
                line_offset = 1;
                ++line_number;
            } break;
            case '/': {
                // Could be a comment, '/', or '/='
                start = point;
                c = *(++point);
                
                if(c == '*')
                {
                    // Begin multi-line comment
                    c = *(++point);
                    line_offset += 2; // 2 for the /*
                    
                    u32 comment_nesting = 1;
                    
                    while(true)
                    {
                        // Looking for /* and */ to change comment nesting
                        
                        if(c == '*')
                        {
                            c = *(++point);
                            ++line_offset;
                            
                            if(c == '/')
                            {
                                c = *(++point);
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
                            c = *(++point);
                            ++line_offset;
                            
                            if(c == '*')
                            {
                                c = *(++point);
                                ++line_offset;
                                ++comment_nesting;
                            }
                        }
                        else if(c == '\r')
                        {
                            c = *(++point);
                            if(c == '\n')
                            {
                                c = *(++point);
                            }
                            line_offset = 1;
                            ++line_number;
                        }
                        else if(c == '\n')
                        {
                            c = *(++point);
                            line_offset = 1;
                            ++line_number;
                        }
                        else if(c)
                        {
                            c = *(++point);
                            ++line_offset;
                        }
                        else
                        {
                            // TODO: warning about file ending before comment ended
                            break;
                        }
                    }
                }
                else if(c == '/')
                {
                    // Begin EOL comment
                    // Note: line_offset and line_number will be handled next loop
                    do
                    {
                        c = *(++point);
                    }
                    while(c && c != '\n' && c != '\r');
                }
                else if(c == '=')
                {
                    // '/='
                    c = *(++point);
                    add_token(Token_Type::div_eq, make_array(2, start));
                }
                else
                {
                    // '/'
                    add_token(Token_Type::div, make_array(1, start));
                }
            } break;
            case 'a': case 'b': case 'c': case 'd':
            case 'e': case 'f': case 'g': case 'h':
            case 'i': case 'j': case 'k': case 'l':
            case 'm': case 'n': case 'o': case 'p':
            case 'q': case 'r': case 's': case 't':
            case 'u': case 'v': case 'w': case 'x':
            case 'y': case 'z': case 'A': case 'B':
            case 'C': case 'D': case 'E': case 'F':
            case 'G': case 'H': case 'I': case 'J':
            case 'K': case 'L': case 'M': case 'N':
            case 'O': case 'P': case 'Q': case 'R':
            case 'S': case 'T': case 'U': case 'V':
            case 'W': case 'X': case 'Y': case 'Z':
            case '_': {
                
                start = point;
                do
                {
                    c = *(++point);
                }
                while(('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9') || (c == '_'));
                
                u64 len = point - start;
                String contents = make_array(len, start);
                Token_Type type = Token_Type::ident;
                
                // Easiest way to do keywords right now
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
            } break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9': {
                // TODO: numbers that start with .
                // TODO: perhaps more strict number parsing ? optionally ?
                start = point;
                do
                {
                    c = *(++point);
                }
                while('0' <= c && c <= '9');
                
                if(c == '.')
                {
                    do
                    {
                        c = *(++point);
                    }
                    while('0' <= c && c <= '9');
                    
                    if(c == 'e' || c == 'E')
                    {
                        c = *(++point);
                        if(c == '+' || c == '-')
                        {
                            c = *(++point);
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
                                c = *(++point);
                            }
                            while('0' <= c && c <= '9');
                        }
                    }
                }
                
                u64 len = point - start;
                add_token(Token_Type::number, make_array(len, start));
            } break;
            case ':': {
                start = point;
                c = *(++point);
                
                if(c == ':')
                {
                    c = *(++point);
                    add_token(Token_Type::double_colon, make_array(2, start));
                }
                else if(c == '=')
                {
                    c = *(++point);
                    add_token(Token_Type::colon_eq, make_array(2, start));
                }
                else
                {
                    add_token(Token_Type::colon, make_array(1, start));
                }
            } break;
            case '+': {
                start = point;
                c = *(++point);
                
                if(c == '=')
                {
                    c = *(++point);
                    add_token(Token_Type::add_eq, make_array(2, start));
                }
                else
                {
                    add_token(Token_Type::add, make_array(1, start));
                }
            } break;
            case '-':
            {
                start = point;
                c = *(++point);
                
                if(c == '=')
                {
                    c = *(++point);
                    add_token(Token_Type::sub_eq, make_array(2, start));
                }
                else if(c == '>')
                {
                    c = *(++point);
                    add_token(Token_Type::arrow, make_array(2, start));
                }
                else
                {
                    add_token(Token_Type::sub, make_array(1, start));
                }
            } break;
            case '=': {
                start = point;
                c = *(++point);
                
                if(c == '=')
                {
                    c = *(++point);
                    add_token(Token_Type::double_equal, make_array(2, start));
                }
                else
                {
                    add_token(Token_Type::equal, make_array(1, start));
                }
            } break;
            case ',': {
                add_token(Token_Type::comma, make_array(1, point));
                c = *(++point);
            } break;
            case ';': {
                add_token(Token_Type::semicolon, make_array(1, point));
                c = *(++point);
            } break;
            case '(': {
                add_token(Token_Type::open_paren, make_array(1, point));
                c = *(++point);
            } break;
            case ')': {
                add_token(Token_Type::close_paren, make_array(1, point));
                c = *(++point);
            } break;
            case '[': {
                add_token(Token_Type::open_sqr, make_array(1, point));
                c = *(++point);
            } break;
            case ']': {
                add_token(Token_Type::close_sqr, make_array(1, point));
                c = *(++point);
            } break;
            case '{': {
                add_token(Token_Type::open_brace, make_array(1, point));
                c = *(++point);
            } break;
            case '}': {
                add_token(Token_Type::close_brace, make_array(1, point));
                c = *(++point);
            } break;
            default: {
                // TODO: report better error
                print_err("Unexpected character\n");
                ++point;
                ++line_offset;
                c = *point;
            } break;
        }
    }
    
    array_add(&result, {Token_Type::eof, line_number, line_offset, str_lit("EOF")});
    return result;
}
