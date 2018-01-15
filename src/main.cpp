
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
    if(!tokens.data)
    {
        return 1;
    }
    
    Lexed_String str = {file_contents, tokens};
    Dynamic_Array<Decl_AST> decls = parse_tokens(&str);
    print_dot(&stdout_buf, decls.array);
    
    return 0;
}
