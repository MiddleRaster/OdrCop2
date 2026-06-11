#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"

Test ExploratoryTestsOfClangAST[] =
{
	{"Given code containing a function named Foo, can find Foo in the AST", []
		{
			std::string code = R"(int foo() { return 42; })";

            std::vector<OdrCop2::FunctionInfo> functionInfos;

            clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(functionInfos), code, { "-x", "c++-cpp-output" });
            Assert::AreEqual(L"foo", functionInfos[0].fullyQualifiedName,                           "should have found function 'foo'");
            Assert::AreEqual(L"int", functionInfos[0].returnType.fullyQualifiedReturnValueTypeName, "should have found the return type");
            Assert::AreEqual(L"int", functionInfos[0].returnType.canonicalizedReturnValuetypeName,  "should have found the canonicalized return type");
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
            Assert::AreEqual(L"MyNamespace::foo", functionInfos[0].fullyQualifiedName,                           "should have found fully qualified function name");
            Assert::AreEqual(L"MyInt",            functionInfos[0].returnType.fullyQualifiedReturnValueTypeName, "should have found the fully qualified return type");
            Assert::AreEqual(L"int",              functionInfos[0].returnType.canonicalizedReturnValuetypeName,  "should have found the canonicalized return type");
        }
    },
    {"can get fully qualified function name from anonymous namespace", []
        {
            std::string code = R"( namespace { int foo() { return 42; } } )";

            std::vector<OdrCop2::FunctionInfo> functionInfos;
            clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(functionInfos), code, { "-x", "c++-cpp-output" });
            Assert::AreEqual(L"(anonymous namespace)::foo", functionInfos[0].fullyQualifiedName,                           "should have found fully qualified function name within anonymous namespace");
            Assert::AreEqual(L"int",                        functionInfos[0].returnType.fullyQualifiedReturnValueTypeName, "should have found the return type");
            Assert::AreEqual(L"int",                        functionInfos[0].returnType.canonicalizedReturnValuetypeName,  "should have found the canonicalized return type");
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
            Assert::AreEqual("IamAtypedef", functionInfos[0].returnType.fullyQualifiedReturnValueTypeName, "should return typedef'd return type");
            Assert::AreEqual("int",         functionInfos[0].returnType.canonicalizedReturnValuetypeName,  "should return canonicalized return type");
        }
    },
};