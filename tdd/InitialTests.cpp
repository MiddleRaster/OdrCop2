#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"
#include <DbgHelp.h>

Test ExploratoryTestsOfClangAST[] =
{
    {"Get TU name", []
        {
            std::string code = R"(void foo() {})";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual("input.cc", vec[0].TU, "should have gotten the TU name");
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
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            Assert::AreEqual(2, maps.functionMap.size(), "should have found 2 functions");

            auto vec1 = maps.functionMap["?foo@V2@SomeNamespace@@YAHXZ"];
            auto vec2 = maps.functionMap["?foo@V1@SomeNamespace@@YAHXZ"];
            Assert::AreEqual("?foo@V2@SomeNamespace@@YAHXZ", vec1[0].mangled, "should return the mangled name");
            Assert::AreEqual("?foo@V1@SomeNamespace@@YAHXZ", vec2[0].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(vec1[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("int __cdecl SomeNamespace::V2::foo(void)", std::string(buf, result), "should unmangle back to original function name");
                  result = UnDecorateSymbolName(vec2[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
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
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual(1, vec.size(), "should have found 1 function");
            Assert::AreEqual("?process@@YAXUHelper@?A0x87D7C4E@@@Z", vec[0].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(vec[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
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
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual(1, vec.size(), "should have found 1 function");
            Assert::AreEqual("?makeOuter@@YA?AUOuter@@XZ", vec[0].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(vec[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
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
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual(1, vec.size(), "should have found 1 function");
            Assert::AreEqual("?makeVariant@@YA?ATVariant@@XZ", vec[0].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(vec[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
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
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual(1, vec.size(), "should have found 1 function");
            Assert::AreEqual("?getPacked@Pixel@@QEBAIXZ", vec[0].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(vec[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("public: unsigned int __cdecl Pixel::getPacked(void)const __ptr64", std::string(buf, result), "should unmangle back to function with nameless struct arg");
        }
    },
};
