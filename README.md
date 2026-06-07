# OdrCop2
An ODR violations detector using LLVM/Clang's AST parser

## The big idea
There appears to be a paucity of ODR violation-detecting tools for MSVC, so I thought I'd write one.
The idea is to include a .targets file inside your .vcxproj file to use Microsoft's clang-cl.exe to generate a compile_commands.json file,
with every single CL.exe switch so that LLVM/Clang can read the .cpp files with exactly the right options set,
thus allowing ODR violations to be detected.


### What works so far
Almost nothing:
1. The .targets file not is ready. 
2. But I got all the LLVM sources to build and I can link against the .libs and headers.
3. I have a few TDD tests, a proof-of-concept that can output function names.
