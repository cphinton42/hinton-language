
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
    
    print_err("\x1B[1;31mError\x1B[0m: %d:%d:\n    %.*s\n    %.*s\n%s\n", line_number, line_offset, (u32)error_text.count, error_text.data, (u32)len, program_text, arrow);
}

int main(int argc, char **argv)
{
    // Note: if this becomes multi-threaded, we can get rid of the globals (writes smaller than 4K are supposed to be atomic IIRC)
    init_std_print_buffers();
    
    String file_contents = read_entire_file("test.txt");
    if(!file_contents.data)
    {
        print_err("Unable to read test.txt\n");
    }
    
    Dynamic_Array<Token> tokens = lex_string(file_contents);
    
    Lexed_String str = {file_contents, tokens};
    Dynamic_Array<Decl_AST> decls = parse_tokens(&str);
    print_dot(&stdout_buf, decls.array);
    
    return 0;
}
