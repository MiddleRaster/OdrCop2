#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"

Test UdtTests[] =
{
    {"Can find simple struct with one data member", []
        {
            std::string code = R"(struct S { int x; };)";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("struct S {\npublic:    int x;\n}", vec[0].fullyQualified, "should have gotten the entire struct but not the method body");
        }
    },
    {"struct with single data member and one method", []
        {
            std::string code = R"(struct S { int x; int foo(){ return x; } };)";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("struct S {\npublic:    int x;\npublic:    int __cdecl foo();\n}", vec[0].fullyQualified, "should have gotten the entire struct but not the function body");
        }
    },
    {"class with one private method and one private data-member and one public method and one public data-member", []
        {
            std::string code =  "class Class {"
                                "    void Private() {};"
                                "    const int privateInt = 0;"
                                "public:"
                                "    int Public() const { return privateInt; }"
                                "    const int publicInt{-1};"
                                "};";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("class Class {\n"
                             "private:   void __cdecl Private();\n"
                             "private:   const int privateInt=0;\n"
                             "public:    int __cdecl Public() const;\n"
                             "public:    const int publicInt{-1};\n"
                             "}", vec[0].fullyQualified, "should have gotten the entire struct but not the function body");
        }
    },
};
