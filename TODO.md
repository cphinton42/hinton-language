## TODO List

### Immediate TODO's

 - Operators !,~,&,|,&&,||
 - Scopes, linking identifiers
 - Type checking
 - Create general allocator interface (? maybe just make it more uniform ?)
 - Cleanup memory leaks in parsing
 - Put newline on errors at the end of file that doesn't end in a newline
 - Code gen

### Needed to be a serious language

 - Polymorphism
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
