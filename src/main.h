#ifndef MAIN_H
#define MAIN_H

#include "basic.h"

String read_entire_file(const byte *file_name);
void report_error(byte *program_text, u32 line_number, u32 line_offset, String error_text);

#endif // MAIN_H