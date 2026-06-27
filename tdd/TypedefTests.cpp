#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"
#include <DbgHelp.h>

Test TypedefTests[] =
{
	{"Given code containing a typedef, can find it in the AST", []
		{
			std::string code = R"(typedef int INT;)";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            Assert::IsTrue(ok);
            const auto& vec = maps.typedefMap.begin()->second;
            Assert::AreEqual(1, vec.size(), "there should be 1 element in vector of TypedefInfo structs");
            Assert::AreEqual("using INT = int; // typedef int INT;", vec[0].fullyQualified, "should have found typedef");
        }
    },
	{"can find using-style typedefs in the AST", []
		{
			std::string code = R"(using INT=int;)";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            Assert::IsTrue(ok);
            const auto& vec = maps.typedefMap.begin()->second;
            Assert::AreEqual(1, vec.size(), "there should be 1 element in vector of TypedefInfo structs");
            Assert::AreEqual("using INT = int; // typedef int INT;", vec[0].fullyQualified, "should have found typedef");
        }
    },
    {"can get a typedef's underlying type from another typedef", []
        {
            std::string code = R"(using INT=int;)"
                               R"(typedef INT ANOTHER_INT;)";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++" });
            Assert::IsTrue(ok);

            Assert::AreEqual(2, maps.typedefMap.size(), "there should be two different typedefs in the map");

            auto it1 = maps.typedefMap.begin();
            auto it2 = std::next(it1);

            // alphabetized by key, so ANOTHER_INT comes first
            Assert::AreEqual("using ANOTHER_INT = int; // typedef int ANOTHER_INT;", it1->second[0].fullyQualified, "should have found typedef");
            Assert::AreEqual(        "using INT = int; // typedef int INT;",         it2->second[0].fullyQualified, "should have found using");
        }
    },
};

extern std::pair<int, std::string> RunTest(const std::string& code1, const std::string& code2);

Test AliasTemplateTests[] =
{
    {"Can find Alias Template", []
        {
            std::string code1 = "template <class T> struct Box { T value; }; struct Foo { int  x; }; template <class T> using Boxy = Box<T>; void Use() { Boxy<Foo> v; v.value = Foo{1}; }";
            std::string code2 = "template <class T> struct Box { T value; }; struct Foo { char x; }; template <class T> using Boxy = Box<T>; void Use() { Boxy<Foo> v; v.value = Foo{1}; }";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(3, maps.     udtMap.size());
            Assert::AreEqual(1, maps. typedefMap.size());
            Assert::AreEqual(1, maps.functionMap.size());

            auto it = maps.udtMap.begin();
            Assert::AreEqual("template<class T> struct Box {\n"
                             "   T value;\n"
                             "};"
                           , (*it++).second[0].fullyQualified, "serialization");
            Assert::AreEqual("template<> struct Box<struct Foo> { // sizeof=4\n"
                             "   Foo value;\n"
                             "};"
                           , (*it++).second[0].fullyQualified, "serialization");
            Assert::AreEqual("struct Foo { // sizeof=4\n"
                             "   int x;\n"
                             "};"
                           , (*it++).second[0].fullyQualified, "serialization");

            Assert::AreEqual("void __cdecl Use() {\n"
                             "    Boxy<Foo> v;\n"
                             "    v.value = Foo{1};\n"
                             "}"
                           , maps.functionMap.begin()->second[0].fullyQualified, "serialization");
            Assert::AreEqual("template <T> using Boxy = Box<T>; // no typedef equivalent", maps.typedefMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(2, violations, "wrong number of violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: Box<struct Foo>\n"
                                "[tu3.cpp]\n"
                                "template<> struct Box<struct Foo> { // sizeof=4\n"
                                "   Foo value;\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "template<> struct Box<struct Foo> { // sizeof=1\n"
                                "   Foo value;\n"
                                "};\n"
                                "\n"
                                "ODR VIOLATION: Foo\n"
                                "[tu3.cpp]\n"
                                "struct Foo { // sizeof=4\n"
                                "   int x;\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "struct Foo { // sizeof=1\n"
                                "   char x;\n"
                                "};\n", output, "mismatched output");
            }
        }
    },
    {"Alias template for a function pointer", []
        {
            std::string code1 = "struct Foo { int  x; }; template <class R, class... Args> using Fn = R(*)(Args...); Foo MakeFoo(int  x) { return Foo{x}; } void Use() { Fn<Foo, int>  f = &MakeFoo; f(1); }";
            std::string code2 = "struct Foo { char x; }; template <class R, class... Args> using Fn = R(*)(Args...); Foo MakeFoo(char x) { return Foo{x}; } void Use() { Fn<Foo, char> f = &MakeFoo; f(1); }";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.     udtMap.size());
            Assert::AreEqual(1, maps. typedefMap.size());
            Assert::AreEqual(2, maps.functionMap.size());

            Assert::AreEqual("struct Foo { // sizeof=4\n"
                             "   int x;\n"
                             "};"
                           , maps.udtMap.begin()->second[0].fullyQualified, "serialization");

            auto it = maps.functionMap.begin();
            Assert::AreEqual("Foo __cdecl MakeFoo(int x) { return Foo{x}; }", (*it++).second[0].fullyQualified, "serialization");
            Assert::AreEqual("void __cdecl Use() {\n"
                             "    Fn<Foo, int> f = &MakeFoo;\n"
                             "    f(1);\n"
                             "}"
                           , (*it++).second[0].fullyQualified, "serialization");

            Assert::AreEqual("template <R, Args> using Fn = R (*)(Args...); // no typedef equivalent", maps.typedefMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(2, violations, "wrong number of violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: Use()\n"
                                "[tu3.cpp]\n"
                                "void __cdecl Use() {\n"
                                "    Fn<Foo, int> f = &MakeFoo;\n"
                                "    f(1);\n"
                                "}\n"
                                "[tu4.cpp]\n"
                                "void __cdecl Use() {\n"
                                "    Fn<Foo, char> f = &MakeFoo;\n"
                                "    f(1);\n"
                                "}\n"
                                "\n"
                                "ODR VIOLATION: Foo\n"
                                "[tu3.cpp]\n"
                                "struct Foo { // sizeof=4\n"
                                "   int x;\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "struct Foo { // sizeof=1\n"
                                "   char x;\n"
                                "};\n", output, "mismatched output");
            }
        }
    },
    {"Alias template for a fixed-size array", []
        {
            std::string code1 = "template <class T, size_t N> struct Arr { T data[N]; T& operator[](size_t i) { return data[i]; } }; struct Foo { int  x; }; template <class T, size_t N> using ArrAlias = Arr<T,N>; void Use() { ArrAlias<Foo,4> a; a[0] = Foo{1}; }";
            std::string code2 = "template <class T, size_t N> struct Arr { T data[N]; T& operator[](size_t i) { return data[i]; } }; struct Foo { char x; }; template <class T, size_t N> using ArrAlias = Arr<T,N>; void Use() { ArrAlias<Foo,4> a; a[0] = Foo{1}; }";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(3, maps.     udtMap.size());
            Assert::AreEqual(1, maps. typedefMap.size());
            Assert::AreEqual(3, maps.functionMap.size());

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("template<class T, size_t N> struct Arr {\n"
                                 "   T data[N];\n"
                                 "   T & __cdecl operator[](size_t i) { return data[i]; }\n"
                                 "};"
                               , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("template<> struct Arr<struct Foo, 4> { // sizeof=16\n"
                                 "   Foo data[4];\n"
                                 "   Foo __cdecl operator[](size_t i) { return data[i]; }\n"
                                 "};"
                               , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("struct Foo { // sizeof=4\n"
                                 "   int x;\n"
                                 "};"
                               , (*it++).second[0].fullyQualified, "serialization");
            }

            Assert::AreEqual("template <T, N> using ArrAlias = Arr<T, N>; // no typedef equivalent", maps.typedefMap.begin()->second[0].fullyQualified, "serialization");

            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("T & __cdecl Arr::operator[](size_t i) { return data[i]; }"        , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("Foo __cdecl Arr<Foo, 4>::operator[](size_t i) { return data[i]; }", (*it++).second[0].fullyQualified, "serialization");
            }

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(2, violations, "wrong number of violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: Arr<struct Foo, 4>\n"
                                "[tu3.cpp]\n"
                                "template<> struct Arr<struct Foo, 4> { // sizeof=16\n"
                                "   Foo data[4];\n"
                                "   Foo __cdecl operator[](size_t i) { return data[i]; }\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "template<> struct Arr<struct Foo, 4> { // sizeof=4\n"
                                "   Foo data[4];\n"
                                "   Foo __cdecl operator[](size_t i) { return data[i]; }\n"
                                "};\n"
                                "\n"
                                "ODR VIOLATION: Foo\n"
                                "[tu3.cpp]\n"
                                "struct Foo { // sizeof=4\n"
                                "   int x;\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "struct Foo { // sizeof=1\n"
                                "   char x;\n"
                                "};\n", output, "mismatched output");
            }
        }
    },
    {"Alias template for a smart pointer", []
        {
            std::string code1 = "template <class T> struct Ptr { T* p; Ptr(T* p) : p(p) {} ~Ptr() { delete p; } }; struct Foo { int  x; }; template <class T> using OwnedPtr = Ptr<T>; void Use() { OwnedPtr<Foo> p(new Foo{1}); }";
            std::string code2 = "template <class T> struct Ptr { T* p; Ptr(T* p) : p(p) {} ~Ptr() { delete p; } }; struct Foo { char x; }; template <class T> using OwnedPtr = Ptr<T>; void Use() { OwnedPtr<Foo> p(new Foo{1}); }";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(3, maps.     udtMap.size());
            Assert::AreEqual(1, maps. typedefMap.size());
            Assert::AreEqual(5, maps.functionMap.size());

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct Foo { // sizeof=4\n"
                                 "   int x;\n"
                                 "};"
                               , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("template<class T> struct Ptr {\n"
                                 "   T *p;\n"
                                 "   void __cdecl Ptr<T>(T * p) : p((p)) {}\n"
                                 "   void __cdecl ~Ptr<T>() { delete p; }\n"
                                 "};"
                               , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("template<> struct Ptr<struct Foo> { // sizeof=8\n"
                                 "   Foo *p;\n"
                                 "   void __cdecl Ptr(Foo * p) : p(p) {}\n"
                                 "   void __cdecl ~Ptr() { delete p; }\n"
                                 "};"
                               , (*it++).second[0].fullyQualified, "serialization");
            }

            Assert::AreEqual("template <T> using OwnedPtr = Ptr<T>; // no typedef equivalent", maps.typedefMap.begin()->second[0].fullyQualified, "serialization");

            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void __cdecl Ptr::Ptr<T>(T * p) : p((p)) {}"        , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("void __cdecl Ptr::~Ptr<T>() { delete p; }"          , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("void __cdecl Ptr<Foo>::Ptr(Foo * p) : p(p) {}"      , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("void __cdecl Ptr<Foo>::~Ptr() { delete p; }"        , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("void __cdecl Use() { OwnedPtr<Foo> p(new Foo{1}); }", (*it++).second[0].fullyQualified, "serialization");
            }

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: Foo\n"
                                "[tu3.cpp]\n"
                                "struct Foo { // sizeof=4\n"
                                "   int x;\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "struct Foo { // sizeof=1\n"
                                "   char x;\n"
                                "};\n", output, "mismatched output");
            }
        }
    },
    {"Alias template as a template template parameter", []
        {
            std::string code1 = "template <class T> struct Box { T value; }; template <class T> using Boxy = Box<T>; struct Foo { int  x; }; template <template <class> class C> struct Wrapper { C<Foo> data; }; void Use() { Wrapper<Boxy> w; w.data.value = Foo{1}; }";
            std::string code2 = "template <class T> struct Box { T value; }; template <class T> using Boxy = Box<T>; struct Foo { char x; }; template <template <class> class C> struct Wrapper { C<Foo> data; }; void Use() { Wrapper<Boxy> w; w.data.value = Foo{1}; }";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(5, maps.     udtMap.size());
            Assert::AreEqual(1, maps. typedefMap.size());
            Assert::AreEqual(1, maps.functionMap.size());

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("template<class T> struct Box {\n"
                                 "   T value;\n"
                                 "};"                    
                               , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("template<> struct Box<struct Foo> { // sizeof=4\n"
                                 "   Foo value;\n"
                                 "};"
                               , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("struct Foo { // sizeof=4\n"
                                 "   int x;\n"
                                 "};"
                               , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("template<template<class> class C> struct Wrapper {\n"
                                 "   C<Foo> data;\n"
                                 "};"
                               , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("template<> struct Wrapper<Boxy> { // sizeof=4\n"
                                 "   Boxy<Foo> data;\n"
                                 "};"
                               , (*it++).second[0].fullyQualified, "serialization");
            }

            Assert::AreEqual("template <T> using Boxy = Box<T>; // no typedef equivalent", maps.typedefMap.begin()->second[0].fullyQualified, "serialization");
            Assert::AreEqual("void __cdecl Use() {\n"
                                "    Wrapper<Boxy> w;\n"
                                "    w.data.value = Foo{1};\n"
                                "}", maps.functionMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(3, violations, "wrong number of violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: Box<struct Foo>\n"
                                "[tu3.cpp]\n"
                                "template<> struct Box<struct Foo> { // sizeof=4\n"
                                "   Foo value;\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "template<> struct Box<struct Foo> { // sizeof=1\n"
                                "   Foo value;\n"
                                "};\n"
                                "\n"
                                "ODR VIOLATION: Foo\n"
                                "[tu3.cpp]\n"
                                "struct Foo { // sizeof=4\n"
                                "   int x;\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "struct Foo { // sizeof=1\n"
                                "   char x;\n"
                                "};\n"
                                "\n"
                                "ODR VIOLATION: Wrapper<Boxy>\n"
                                "[tu3.cpp]\n"
                                "template<> struct Wrapper<Boxy> { // sizeof=4\n"
                                "   Boxy<Foo> data;\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "template<> struct Wrapper<Boxy> { // sizeof=1\n"
                                "   Boxy<Foo> data;\n"
                                "};\n", output, "mismatched output");
            }
        }
    },
    {"Alias template for a dependent type (type trait)", []
        {
            std::string code1 = "template <class T> struct RemovePtr     { using type = T; }; template <class T> struct RemovePtr<T*> { using type = T; }; template <class T> using NoPtr = typename RemovePtr<T>::type; struct Foo { int  x; }; void Use() { NoPtr<Foo*> f; f.x = 1; }";
            std::string code2 = "template <class T> struct RemovePtr     { using type = T; }; template <class T> struct RemovePtr<T*> { using type = T; }; template <class T> using NoPtr = typename RemovePtr<T>::type; struct Foo { char x; }; void Use() { NoPtr<Foo*> f; f.x = 'a'; }";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(4, maps.     udtMap.size());
            Assert::AreEqual(0, maps.     varMap.size());
            Assert::AreEqual(0, maps.    enumMap.size());
            Assert::AreEqual(1, maps. typedefMap.size());
            Assert::AreEqual(1, maps.functionMap.size());

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct Foo { // sizeof=4\n"
                                 "   int x;\n"
                                 "};"                    
                               , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("template<class T> struct RemovePtr {\n"
                                 "   using type = T;\n"
                                 "};"
                               , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("template<> struct RemovePtr<struct Foo *> { // sizeof=1\n"
                                 "   using type = Foo;\n"
                                 "};"
                               , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("template<> struct RemovePtr<type-parameter-0-0 *> {\n"
                                 "   using type = T;\n"
                                 "};"
                               , (*it++).second[0].fullyQualified, "serialization");
            }

            Assert::AreEqual("template <T> using NoPtr = typename RemovePtr<T>::type; // no typedef equivalent", maps.typedefMap.begin()->second[0].fullyQualified, "serialization");
            Assert::AreEqual("void __cdecl Use() {\n"
                             "    NoPtr<Foo *> f;\n"
                             "    f.x = 1;\n"
                             "}"
                           , maps.functionMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(2, violations, "wrong number of violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: Use()\n"
                                "[tu3.cpp]\n"
                                "void __cdecl Use() {\n"
                                "    NoPtr<Foo *> f;\n"
                                "    f.x = 1;\n"
                                "}\n"
                                "[tu4.cpp]\n"
                                "void __cdecl Use() {\n"
                                "    NoPtr<Foo *> f;\n"
                                "    f.x = 'a';\n"
                                "}\n"
                                "\n"
                                "ODR VIOLATION: Foo\n"
                                "[tu3.cpp]\n"
                                "struct Foo { // sizeof=4\n"
                                "   int x;\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "struct Foo { // sizeof=1\n"
                                "   char x;\n"
                                "};\n", output, "mismatched output");
            }
        }
    },
};