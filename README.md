# OdrCop2
An ODR violations detector using LLVM/Clang's AST parser

## The big idea
There appears to be a paucity of ODR violation-detecting tools for MSVC, so I thought I'd write one.  
The idea is to include a .targets file inside your .vcxproj file to build with the MSVC toolset in order to preprocess your C++ code into .i files.  
The .targets file automatically matches your project's CL switches exactly, so the .i files are produced as part of your normal build.  

Then, once the .i files are produced, run OdrCop2 on the .i files:  any ODR violations will be flagged.

## Usage
From a command prompt, run:  
```OdrCop2.exe <folder of .i files> [more folders ...]```


### What works so far
Almost nothing:
1. The .targets file is ready and works (see the demo folder).
2. I got all the LLVM sources to build and I can link against the .libs and headers.
3. I have a few TDD tests, a proof-of-concept that can output function names.

