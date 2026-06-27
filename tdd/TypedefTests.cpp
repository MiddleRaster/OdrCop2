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

};