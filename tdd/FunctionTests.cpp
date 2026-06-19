#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"
#include <DbgHelp.h>

#include "headers\Utils.h"

Test FunctionTests[] =
{
	{"Given code containing a function named Foo, can find Foo in the AST", []
		{
			std::string code = R"(int foo() { return 42; })";

            OdrCop2::AllMaps maps;
            clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual(L"int __cdecl foo() { return 42; }", vec[0].fullyQualified);
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
            OdrCop2::AllMaps maps;
            clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual(L"int __cdecl MyNamespace::foo() { return 42; }", vec[0].fullyQualified, "should have found fully qualified function name");
        }
    },
    {"internal-linkage/anonymous namespace functions are never part of an ODR violation", []
        {
            std::string code = R"( namespace { int foo() { return 42; } } )";

            OdrCop2::AllMaps maps;
            clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            Assert::AreEqual(0, maps.functionMap.size(), "no internal-linkage functions should have been found");
        }
    },
    {"I wonder what happens if there's a compiler error", []
        {
            StderrSuppressor suppress;
            std::string code = R"(void foo() { return 42; })";
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            Assert::IsFalse(ok);
        }
    },
    {"Get return value, both fully qualified and canonical", []
        {
            std::string code = R"(
using IamAtypedef    = int;
IamAtypedef foo() { return 42; }
)";
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            Assert::IsTrue(ok);
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual("int __cdecl foo() { return 42; }", vec[0].fullyQualified, "should have returned fully qualified function name");
        }
    },
    {"Get mangled function name", []
        {
            std::string code = "int foo() { return 42; }";
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            Assert::IsTrue(ok);
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual("?foo@@YAHXZ", vec[0].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(vec[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE );
            Assert::AreEqual("int __cdecl foo(void)", std::string(buf, result), "should unmangle back to original function name");
        }
    },
    {"Get some function args", []
        {
            std::string code = "int foo(char* p, bool b) { (void)p; (void)b; return 42; }";
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            Assert::IsTrue(ok);
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual("int __cdecl foo(char *, bool) {\n"
                             "    (void)p;\n"
                             "    (void)b;\n"
                             "    return 42;\n"
                             "}", vec[0].fullyQualified, "should have found function");
        }
    },
    {"uninstantiated function templates and specializations work", []
        {
            std::string code = "template <typename T> T   foo     (const T  & t) { return t  ; }"
                               "template <          > int foo<int>(const int& i) { return i+1; }";
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            Assert::IsTrue(ok);
            Assert::AreEqual(2, maps.functionMap.size(), "there should be two different typedefs in the map");

            auto it1 = maps.functionMap.begin();
            auto it2 = std::next(it1);

            // alphabetized by key, so the specialization comes first
            Assert::AreEqual("int __cdecl foo<int>(const int &) { return i + 1; }", it1->second[0].fullyQualified, "should have found the function template specialization");
            Assert::AreEqual("template <typename T> T __cdecl foo(const T &);", it2->second[0].fullyQualified, "should have found the function template");
        }
    },
};