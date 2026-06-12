#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"
#include <DbgHelp.h>

Test FunctionTests[] =
{
	{"Given code containing a function named Foo, can find Foo in the AST", []
		{
			std::string code = R"(int foo() { return 42; })";

            OdrCop2::AllMaps maps;
            clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual(L"int __cdecl foo(void)", vec[0].fullyQualified);
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
            clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual(L"int __cdecl MyNamespace::foo(void)", vec[0].fullyQualified, "should have found fully qualified function name");
        }
    },
    {"can get fully qualified function name from anonymous namespace", []
        {
            std::string code = R"( namespace { int foo() { return 42; } } )";

            OdrCop2::AllMaps maps;
            clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual(L"int __cdecl `anonymous namespace'::foo(void)", vec[0].fullyQualified, "should have found fully qualified function name within anonymous namespace");
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
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
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
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual("int __cdecl foo(void)", vec[0].fullyQualified, "should have returned fully qualified function name");
        }
    },
    {"Get mangled function name", []
        {
            std::string code = "int foo() { return 42; }";
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
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
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual("int __cdecl foo(char * __ptr64,bool)", vec[0].fullyQualified, "should have found function");
        }
    },
};