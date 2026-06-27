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
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("struct S { // sizeof=4\n   int x;\n};", vec[0].fullyQualified, "should have gotten the entire struct but not the method body");
        }
    },
    {"struct with single data member and one method", []
        {
            std::string code = R"(struct S { int x; int foo(){ return x; } };)";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("struct S { // sizeof=4\n   int x;\n   int __cdecl foo() { return x; }\n};", vec[0].fullyQualified, "should have gotten the entire struct but not the function body");
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
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("class Class { // sizeof=8\n"
                             "   void __cdecl Private() {}\n"
                             "   const int privateInt=0;\n"
                             "public:\n"
                             "   int __cdecl Public() const { return privateInt; }\n"
                             "   const int publicInt{-1};\n"
                             "};", vec[0].fullyQualified, "should have gotten the entire struct but not the function body");
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
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("class Class { // sizeof=4\n"
                             "public:\n"
                             "   int x : 4=3;\n"
                             "};", vec[0].fullyQualified, "should have gotten the entire struct but not the function body");
        }
    },

    // attributes
    {"class with alignas, [[nodiscard]] and [[maybe_unused]]", []
        {
            std::string code = "class alignas(16) [[nodiscard]] [[maybe_unused]] Class {"
                               "public: int x;"
                               "};";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("class alignas(16) [[nodiscard]] [[maybe_unused]] Class { // sizeof=16\n"
                             "public:\n"
                             "   int x;\n"
                             "};", vec[0].fullyQualified, "should have gotten the alignas, nodiscard and maybe_unused attributes");
        }
    },
    // __declspecs
    {"class with __declspec", []
        {
            std::string code = "class __declspec(uuid(\"0cd80b87-432f-4568-80b9-be5444d0fd23\")) alignas(double) [[nodiscard]] [[maybe_unused]] Class {"
                               "public: int x;"
                               "};";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("class __declspec(uuid(\"0cd80b87-432f-4568-80b9-be5444d0fd23\")) alignas(double) [[nodiscard]] [[maybe_unused]] Class { // sizeof=8\n"
                             "public:\n"
                             "   int x;\n"
                             "};", vec[0].fullyQualified, "should have gotten __declspec along with the alignas, nodiscard and maybe_unused attributes");
        }
    },

    // attributes on data-members
    {"class with attributes on data-members", []
        {
            std::string code = "struct S {"
                               "   alignas(16)                    int a;"
                               "   [[deprecated]]                 int b;"
                               "   [[maybe_unused]]               int c;"
                               "   [[deprecated(\"do not use\")]] int d;"
                               "};";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("struct S { // sizeof=16\n"
                             "   alignas(16) int a;\n"
                             "   [[deprecated]] int b;\n"
                             "   [[maybe_unused]] int c;\n"
                             "   [[deprecated(\"do not use\")]] int d;\n"
                             "};", vec[0].fullyQualified, "should have gotten all attributes on data-members");
        }
    },

    // attributes on methods?
    { "class with attributes on methods", []
        {
            std::string code = "struct S {"
                               "[[nodiscard]]           int A();"
                               "[[deprecated]]          int B();"
                               "[[deprecated(\"msg\")]] int C();"
                               "[[noreturn]]            int D();"
                               "[[maybe_unused]]        int E();"
                               "};";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("struct S { // sizeof=1\n"
                             "   [[nodiscard]] int __cdecl A();\n"
                             "   [[deprecated]] int __cdecl B();\n"
                             "   [[deprecated(\"msg\")]] int __cdecl C();\n"
                             "   [[noreturn]] int __cdecl D();\n"
                             "   [[maybe_unused]] int __cdecl E();\n"
                             "};", vec[0].fullyQualified, "should have gotten all attributes on data-members");
        }
    },

    // bases
    { "class with bases", []
        {
            std::string code = "struct PrivateBase{}; struct PublicBase{}; struct VirtualBase{};"
                               "class Class : PrivateBase, public PublicBase, virtual VirtualBase {"
                               "};";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(4, maps.udtMap.size(), "number of UDTs found");
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("class Class : private PrivateBase, public PublicBase, private virtual VirtualBase { // sizeof=16\n"
                             "};", vec[0].fullyQualified, "should have gotten all bases");
        }
    },

    // final structs/classes
    { "class with final", []
        {
            std::string code = "class Class final {};";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size(), "number of UDTs found");
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("class Class final { // sizeof=1\n};", vec[0].fullyQualified, "should have gotten final");
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
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("template<typename T> class TemplateClass {\n"
                             "public:\n"
                             "   T t;\n"
                             "};", vec[0].fullyQualified, "can get simple class template");
        }
    },
    {"simple class template with a method returing a T", []
        {
            std::string code = "template<typename T> class TemplateClass {"
                               "public:"
                               "    T MyMethod();"
                               "};";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("template<typename T> class TemplateClass {\n"
                             "public:\n"
                             "   T __cdecl MyMethod();\n"
                             "};", vec[0].fullyQualified, "can get simple class template with method returning a T");
        }
    },
    { "simple class template with a method taking a T&", []
        {
            std::string code = "template<typename T> class TemplateClass {"
                               "public:"
                               "    void MyMethod(T&);"
                               "};";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("template<typename T> class TemplateClass {\n"
                             "public:\n"
                             "   void __cdecl MyMethod(T &);\n"
                             "};", vec[0].fullyQualified, "can get simple class template with method taking a T&");
        }
    },
    {"can do class template with int parameter type", []
        {
            std::string code = "template<int N> struct FixedBuffer {"
                               "     int  size() const;"
                               "     int  data[N];"
                               "};";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("template<int N> struct FixedBuffer {\n"
                             "   int __cdecl size() const;\n"
                             "   int data[N];\n"
                             "};", vec[0].fullyQualified, "can get class template with int parameter type");
        }
    },
    {"can do template template parameter", []
        {
            std::string code = "template<template<typename> class Container> class Wrapper {"
                               "public: Container<int>  contents; };";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size());
            const auto& vec = maps.udtMap.begin()->second;
            Assert::AreEqual("template<template<typename> class Container> class Wrapper {\n"
                             "public:\n"
                             "   Container<int> contents;\n"
                             "};", vec[0].fullyQualified, "can get template template parameter");
        }
    },

    {"can do canonical TBCI pattern", []
        {
            std::string code = "struct Empty{};"
                               "template<typename T> struct StructT : private T {};"
                               "using Struct = StructT<Empty>;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(2, maps.    udtMap.size(), "should see 2 UDTs");
            Assert::AreEqual(1, maps.typedefMap.size(), "should see 1 typedef/alias");

            auto it = maps.udtMap.begin();
            Assert::AreEqual("struct Empty { // sizeof=1\n"
                             "};", it->second[0].fullyQualified, "can get Empty struct");
            ++it;
            Assert::AreEqual("template<typename T> struct StructT : private T {\n"
                             "};", it->second[0].fullyQualified, "can get struct refactored to TBCI");
            Assert::AreEqual("using Struct = StructT<Empty>; // typedef StructT<Empty> Struct;", maps.typedefMap.begin()->second[0].fullyQualified, "can get typedef/alias");
        }
    },

    // template key stuff
    {"primary template, instantiation and specialization all have different keys into map", []
        {
            std::string code = "template<class T> struct S { T x; };"
                               "template<> struct S<long> { long y; };"
                               "void test() { S<int> s; }";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.functionMap.size(), "should see 1 function");
            { 
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void __cdecl test() { S<int> s; }", it->second[0].fullyQualified, "can get the 'test' function");
            }

            Assert::AreEqual(3, maps.udtMap.size(), "should see 3 UDTs");
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("template<class T> struct S {\n"
                                 "   T x;\n"
                                 "};", it->second[0].fullyQualified, "can get primary template");
                ++it;
                Assert::AreEqual("template<> struct S<int> { // sizeof=4\n"
                                 "   int x;\n"
                                 "};", it->second[0].fullyQualified, "can get template instantiation");
                ++it;
                Assert::AreEqual("template<> struct S<long> { // sizeof=4\n"
                                 "   long y;\n"
                                 "};", it->second[0].fullyQualified, "can get template specialization");
            }
        }
    },
};
