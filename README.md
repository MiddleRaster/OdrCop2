# OdrCop2
An ODR violations detector using LLVM/Clang's AST parser

## How it works
There appears to be a paucity of ODR violation-detecting tools for MSVC, so I thought I'd write one.  
The idea is to do a clean build of your project while logging to a binlog file; e.g.,  
```msbuild yourProject.vcxproj /t:clean /bl:some.binlog```.  
Then, run OCReplay.exe on that .binlog file to generate a ```compile_commands.json``` file, which OdrCop2.exe needs in order to use LLVM/Clang's AST, thus allowing ODR violations to be detected.


### What works so far
Almost nothing:
1. But I got all the LLVM sources to build and I can link against the .libs and headers.
2. OCReplay can correctly generate the compile_commands.json file. 
3. I have a few TDD tests, a proof-of-concept that can output function names.
4. There is the very beginning of OdrCop2, which can spew function names properly from a TU, including from #included headers.
