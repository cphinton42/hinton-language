## TODO List

### Immediate TODO's

 - Type checking - the exploration is real
 - Assignments need to take l-values, not just identifiers
 - Structs and enums should not capture variable identifiers from local scope
 - Deduplicate all types (e.g. check a hash and strict equality)
   Types probably need to keep line information separate from type information
   Types also need to be canonicalized, identifiers can stand for complex types
   For now, create more abstract functions to deal with the more complicated types
 - Code gen

### Notes
 - Probably need to differentiate the concept of typechecking
   - local matching of types
   - constraints by directives, constant checking, etc.
   - transitivity
 - Probably undo intern of Primitive_AST to get back line information
   Having synthetic versions is be convenient for type inference, do both
 - Differentiate synthetic flag: polymorphic, inferred, internal
 - Declarations introduce a new scope for recursion's sake.
   Doesn't make much sense when declaring a struct's field or enum value though
   Typechecking might take care of that?
 - Default arguments can use other arguments.
   This needs to be checked for circular definitions.
 - Need to check for illegal double declarations
   Overloading and shadowing will be ok, but not double declaration in a single scope
 - Operator typechecking will be totally different once overloading is added

### Misc TODO's

 - Create general allocator interface (? maybe just make it more uniform ?)
   For example, to supply to array_add
 - Make an array_trim
 - Allow suffixes on number literals for more control without requiring more type annotations (f and u for float and unsigned)
 - Cleanup memory leaks in parsing
 - Put newline on errors at the end of file that doesn't end in a newline
 - Other escape sequences in strings (such as \UXXXXXX)

### Needed to be a serious language

 - Polymorphism
   - Scopes may need to change a bit
 - FFI
 - intrinsics

### Other TODO's

 - built-in array type, string type
 - vector types (SIMD)

### Ideas

 - Group types of AST's in memory? e.g. maybe all identifiers, or all declarations?
 - Calling convention abstraction
 - Stack inspection
 - Stack-like context instead of thread-locals
   (using a dedicated register/additional function parameter)
 - Allocator concept - anything implementing an allocator has aliasing rules
   - built-in stack allocator that uses stack pointer (instead of alloca)
 - Manual aliasing annotations