
// TODO: remove stdio dependency
#include "stdio.h"

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

int main(int argc, char **argv)
{
    String file_contents = read_entire_file("test.txt");
    if(file_contents.data)
    {
        printf("%s\n", file_contents.data);
        
        Dynamic_Array<Token> tokens = lex_string(file_contents);
        for(u64 i = 0; i < tokens.count; ++i)
        {
            printf("Token: %ld, %d:%d, %.*s\n", (u64)tokens[i].type, tokens[i].line_number, tokens[i].line_offset, (u32)tokens[i].contents.count, tokens[i].contents.data);
        }
    }
    
    printf("Hello World!\n");
}
