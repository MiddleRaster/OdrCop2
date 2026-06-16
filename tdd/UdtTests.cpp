#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"
#include "headers\Utils.h"

Test UdtTests[] =
{
    {"Can find simple struct with one data member", []
        {
            std::string code = R"(struct S { int x; };)";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
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
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
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
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
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
    {"class with a bitfield with an initializer", []
        {
            StderrSuppressor suppressor;

            std::string code =  "class Class {"
                                "public:"
                                "    int x : 4 = 3;"  // legal in C++20
                                "};";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("class Class {\n"
                             "public:    int x : 4=3;\n"
                             "}", vec[0].fullyQualified, "should have gotten the entire struct but not the function body");
        }
    },
    // templates
    {"simple class template", []
        {
            std::string code = "template<typename T> class TemplateClass {"
                               "public:"
                               "    T t;"
                               "};";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("template<typename T> class TemplateClass {\n"
                             "public:    T t;\n"
                             "}", vec[0].fullyQualified, "can get simple class template");
        }
    },
    {"simple class template with a method returing a T", []
        {
            std::string code = "template<typename T> class TemplateClass {"
                               "public:"
                               "    T MyMethod();"
                               "};";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("template<typename T> class TemplateClass {\n"
                             "public:    T __cdecl MyMethod();\n"
                             "}", vec[0].fullyQualified, "can get simple class template with method returning a T");
        }
    },
    { "simple class template with a method taking a T&", []
        {
            std::string code = "template<typename T> class TemplateClass {"
                               "public:"
                               "    void MyMethod(T&);"
                               "};";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("template<typename T> class TemplateClass {\n"
                             "public:    void __cdecl MyMethod(T &);\n"
                             "}", vec[0].fullyQualified, "can get simple class template with method taking a T&");
        }
    },
    {"can do class template with int parameter type", []
        {
            std::string code = "template<int N> struct FixedBuffer {"
                               "     int  size() const;"
                               "     int  data[N];"
                               "};";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("template<int N> struct FixedBuffer {\n"
                             "public:    int __cdecl size() const;\n"
                             "public:    int data[N];\n"
                             "}", vec[0].fullyQualified, "can get class template with int parameter type");
        }
    },


    {"can do template template parameter", []
        {
            std::string code = "template<template<typename> class Container> class Wrapper {"
                               "public: Container<int>  contents; };";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("template<template<typename> class Container> class Wrapper {\n"
                             "public:    Container<int> contents;\n"
                             "}", vec[0].fullyQualified, "can get template template parameter");
        }
    },
};
