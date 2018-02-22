## TODO List

### Immediate TODO's

 - Scopes, linking identifiers
 - Add true, false keywords
 - Declarations introduce a new scope for recursion's sake.
   Doesn't make much sense when declaring a struct's field or enum value though
   Typechecking might take care of that?
 - Type checking
 - Code gen

### Misc TODO's

 - Create general allocator interface (? maybe just make it more uniform ?)
 - Cleanup memory leaks in parsing
 - Put newline on errors at the end of file that doesn't end in a newline

### Needed to be a serious language

 - Polymorphism
   - Scopes may need to change a bit
 - FFI
 - intrinsics

### Other TODO's

 - are built-in types keywords, or just bound identifiers?
 - built-in array type, string type
 - vector types (SIMD)

### Ideas

 - Calling convention abstraction
 - Stack inspection
 - Stack-like context instead of thread-locals
   (using a dedicated register/additional function parameter)
