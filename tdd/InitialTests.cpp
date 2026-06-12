#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"
#include <DbgHelp.h>

Test ExploratoryTestsOfClangAST[] =
{
	{"Given code containing a function named Foo, can find Foo in the AST", []
		{
			std::string code = R"(int foo() { return 42; })";

            std::vector<OdrCop2::FunctionInfo> functionInfos;
            clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(functionInfos), code, { "-x", "c++-cpp-output" });
            Assert::AreEqual(L"int __cdecl foo(void)", functionInfos[0].fullyQualified, "should have found function");
        }
    },
    {"can get fully qualified function name", []
		{
			std::string code = R"(
namespace MyNamespace
{
    using MyInt = int;
    MyInt foo()
    {
        return 42;
    } 
})";
            std::vector<OdrCop2::FunctionInfo> functionInfos;
            clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(functionInfos), code, { "-x", "c++-cpp-output" });
            Assert::AreEqual(L"int __cdecl MyNamespace::foo(void)", functionInfos[0].fullyQualified, "should have found fully qualified function name");
        }
    },
    {"can get fully qualified function name from anonymous namespace", []
        {
            std::string code = R"( namespace { int foo() { return 42; } } )";

            std::vector<OdrCop2::FunctionInfo> functionInfos;
            clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(functionInfos), code, { "-x", "c++-cpp-output" });
            Assert::AreEqual(L"int __cdecl `anonymous namespace'::foo(void)", functionInfos[0].fullyQualified, "should have found fully qualified function name within anonymous namespace");
        }
    },
    {"I wonder what happens if there's a compiler error", []
        {
            class StderrSuppressor
            {
                int saved, devNull;
            public:
                StderrSuppressor()
                {
                    saved   = _dup(2);
                    devNull = -1;
                    _sopen_s(&devNull, "nul", _O_WRONLY, _SH_DENYNO, _S_IWRITE);
                    if (saved != -1 && devNull != -1)
                        _dup2(devNull, 2);
                }
               ~StderrSuppressor()
                {
                    if (saved != -1)
                    {
                        _dup2(saved, 2);
                        _close(saved);
                    }
                    if (devNull != -1)
                        _close(devNull);
                }
            } suppress;

            std::string code = R"(void foo() { return 42; })";
            std::vector<OdrCop2::FunctionInfo> functionInfos;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(functionInfos), code, { "-x", "c++-cpp-output" });
            Assert::IsFalse(ok);
        }
    },
    {"Get TU name", []
        {
            std::string code = R"(void foo() {})";
            std::vector<OdrCop2::FunctionInfo> functionInfos;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(functionInfos), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            Assert::AreEqual("input.cc", functionInfos[0].TU, "should have gotten the TU name");
        }
    },
    {"Get return value, both fully qualified and canonical", []
        {
            std::string code = R"(
using IamAtypedef    = int;
IamAtypedef foo() { return 42; }
)";
            std::vector<OdrCop2::FunctionInfo> functionInfos;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(functionInfos), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            Assert::AreEqual("int __cdecl foo(void)", functionInfos[0].fullyQualified, "should have returned fully qualified function name");
        }
    },
    {"Get mangled function name", []
        {
            std::string code = "int foo() { return 42; }";
            std::vector<OdrCop2::FunctionInfo> functionInfos;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(functionInfos), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            Assert::AreEqual("?foo@@YAHXZ", functionInfos[0].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(functionInfos[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE );
            Assert::AreEqual("int __cdecl foo(void)", std::string(buf, result), "should unmangle back to original function name");
        }
    },
    {"Get some function args", []
        {
            std::string code = "int foo(char* p, bool b) { (void)p; (void)b; return 42; }";
            std::vector<OdrCop2::FunctionInfo> functionInfos;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(functionInfos), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            Assert::AreEqual("int __cdecl foo(char * __ptr64,bool)", functionInfos[0].fullyQualified, "should have found function");
        }
    },
    {"Trying out 'inline namespace' syntax", []
        {
            std::string code = 
R"(
namespace SomeNamespace
{
    inline namespace V2 { int foo() { return 42; } }
           namespace V1 { int foo() { return 41; } }
}
)";
            std::vector<OdrCop2::FunctionInfo> functionInfos;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(functionInfos), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            Assert::AreEqual(2, functionInfos.size(), "should have found 2 functions");
            Assert::AreEqual("?foo@V2@SomeNamespace@@YAHXZ", functionInfos[0].mangled, "should return the mangled name");
            Assert::AreEqual("?foo@V1@SomeNamespace@@YAHXZ", functionInfos[1].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(functionInfos[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("int __cdecl SomeNamespace::V2::foo(void)", std::string(buf, result), "should unmangle back to original function name");
                  result = UnDecorateSymbolName(functionInfos[1].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("int __cdecl SomeNamespace::V1::foo(void)", std::string(buf, result), "should unmangle back to original function name");
        }
    },
    {"playing with anonymous namespaces", []
        {
            std::string code =
R"(
namespace { struct Helper { int x; }; }
void process(Helper h) { (void)h; }
)";
            std::vector<OdrCop2::FunctionInfo> functionInfos;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(functionInfos), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, functionInfos.size(), "should have found 1 function");
            Assert::AreEqual("?process@@YAXUHelper@?A0x87D7C4E@@@Z", functionInfos[0].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(functionInfos[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("void __cdecl process(struct `anonymous namespace'::Helper)", std::string(buf, result), "should unmangle back to function with anonymous namespace arg");

            // try out different canonicalizations
            result = UnDecorateSymbolName("?process@@YAXUHelper@?A0x00000000@@@Z",         buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("void __cdecl process(struct `anonymous namespace'::Helper)", std::string(buf, result), "should unmangle back to anonymous namespace arg");
            result = UnDecorateSymbolName("?process@@YAXUHelper@?A0x0000000@@@Z",          buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("void __cdecl process(struct `anonymous namespace'::Helper)", std::string(buf, result), "should unmangle back to anonymous namespace arg");
            result = UnDecorateSymbolName("?process@@YAXUHelper@?A0x0@@@Z",                buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("void __cdecl process(struct `anonymous namespace'::Helper)", std::string(buf, result), "should unmangle back to anonymous namespace arg");
            result = UnDecorateSymbolName("?process@@YAXUHelper@?A0x@@@Z",                 buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("void __cdecl process(struct `anonymous namespace'::Helper)", std::string(buf, result), "should unmangle back to anonymous namespace arg");
        }
    },
    {"playing with nameless unions/structs/classes, take 1", []
        {
            std::string code =
R"(
struct Outer                            // anonymous struct as a parameter type
{
    struct { int x; int y; };           // anonymous struct — members promoted to Outer scope
};
Outer makeOuter() { return Outer{}; }   // Function returning a type containing an anonymous struct
)";
            std::vector<OdrCop2::FunctionInfo> functionInfos;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(functionInfos), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, functionInfos.size(), "should have found 1 function");
            Assert::AreEqual("?makeOuter@@YA?AUOuter@@XZ", functionInfos[0].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(functionInfos[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("struct Outer __cdecl makeOuter(void)", std::string(buf, result), "should unmangle back to function with nameless struct arg");
        }
    },
    {"playing with nameless unions/structs/classes, take 2", []
        {
            std::string code =
R"(
union Variant
{
    struct { float r; float g; float b; };
    int raw;
};
Variant makeVariant() { return Variant{}; }
)";
            std::vector<OdrCop2::FunctionInfo> functionInfos;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(functionInfos), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, functionInfos.size(), "should have found 1 function");
            Assert::AreEqual("?makeVariant@@YA?ATVariant@@XZ", functionInfos[0].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(functionInfos[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("union Variant __cdecl makeVariant(void)", std::string(buf, result), "should unmangle back to function with nameless struct arg");
        }
    },
    {"playing with nameless unions/structs/classes, take 3", []
        {
            std::string code =
R"(
using uint8_t = unsigned char;
using uint32_t = unsigned int;
struct Pixel
{
    union
    {
        struct { uint8_t r, g, b, a; }; // anonymous struct
        uint32_t packed;
    };
    uint32_t getPacked() const { return packed; }
};
)";
            std::vector<OdrCop2::FunctionInfo> functionInfos;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(functionInfos), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, functionInfos.size(), "should have found 1 function");
            Assert::AreEqual("?getPacked@Pixel@@QEBAIXZ", functionInfos[0].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(functionInfos[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("public: unsigned int __cdecl Pixel::getPacked(void)const __ptr64", std::string(buf, result), "should unmangle back to function with nameless struct arg");
        }
    },
};