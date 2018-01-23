
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
    
    Parsing_Context ctx;
    init_parsing_context(&ctx, file_contents, tokens.array, 4096);
    Dynamic_Array<Decl_AST*> decls = parse_tokens(&ctx);
    print_dot(&stdout_buf, decls.array);
    
    return 0;
}
