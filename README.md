# OdrCop2
An ODR violations detector using LLVM/Clang's AST parser.  
There appears to be a paucity of ODR violation-detecting tools for MSVC, so I thought I'd write one.  

## How it works
The idea is to do a clean build of your project while logging to a binlog file, which OdrCop2 uses to know what files to parse.  
Do the following in order:
```
1. msbuild YourSolution.slnx /t:YourProject:Rebuild /p:Configuration=Debug /p:Platform=x64 /bl:some.binlog // generates a .binlog file
2. bl2oc.exe <path-to-.binlog> [output-dir] // generates a "compile_commands.json" file
3. OdrCop2 <folder to compile_commands.json> // outputs ODR violations to std out
```  


### What's completed
1. enums
2. typedefs/aliases
3. functions
4. UDTs
5. unnamed structs/classes/unions/enums
6. anonymous namespaces

### What's left to do
1. hooking up the COFF/.obj file reader (might not need to do this)
2. testing, lots of testing

