#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"
#include <DbgHelp.h>

Test EnumTests[] =
{
	{"Given code containing an enum definition, find it in the AST", []
		{
			std::string code = R"(enum Color { Red, Green, Blue};)";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            const auto& vec = maps.enumMap.begin()->second;
            Assert::AreEqual("enum Color { Red=0, Green=1, Blue=2 };", vec[0].fullyQualified, "should have found enum");
        }
    },
    {"Can find an enum inside a namespace", []
        {
            std::string code = R"(namespace Hi { enum Color { Red, Green, Blue}; })";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            const auto& vec = maps.enumMap.begin()->second;
            Assert::AreEqual("enum Hi::Color { Red=0, Green=1, Blue=2 };", vec[0].fullyQualified, "should have found enum");
        }
    },
    {"Compare an enum class v. an enum", []
        {
            std::string code1 = R"(namespace Hi { enum class Color : int { Red, Green, Blue}; })";
            std::string code2 = R"(namespace Hi { enum       Color       { Red, Green, Blue}; })";
            std::string code3 = R"(namespace Hi { enum       Color : int { Red, Green, Blue}; })";

            OdrCop2::AllMaps maps;
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, { "-x", "c++", "-std=c++23" }, "tu1.cpp"));
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++", "-std=c++23" }, "tu2.cpp"));
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code3, { "-x", "c++", "-std=c++23" }, "tu3.cpp"));

            auto& vec = maps.enumMap.begin()->second;
            Assert::AreEqual(3, vec.size(), "should have been 3 enums of the same name");

            Assert::AreEqual("tu1.cpp", vec[0].TU, "tu1 should be first");
            Assert::AreEqual("tu2.cpp", vec[1].TU, "tu2 should be second");
            Assert::AreEqual("tu3.cpp", vec[2].TU, "tu3 should be third");

            Assert::AreEqual("enum class Hi::Color : int " "{ Red=0, Green=1, Blue=2 };", vec[0].fullyQualified, "should have 'class' and underlying type");
            Assert::AreEqual("enum"    " Hi::Color "       "{ Red=0, Green=1, Blue=2 };", vec[1].fullyQualified, "should not have 'class' nor underlying type");
            Assert::AreEqual("enum"    " Hi::Color : int " "{ Red=0, Green=1, Blue=2 };", vec[2].fullyQualified, "should not have 'class' but does have underlying type");
        }
    },
    {"Compare two nameless enums", []
        {
            std::string code1 = R"(namespace NS { enum { a    }; })";
            std::string code2 = R"(namespace NS { enum { a, b }; })";
    
            OdrCop2::AllMaps maps;
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, { "-x", "c++", "-std=c++23" }, "tu1.cpp"));
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++", "-std=c++23" }, "tu2.cpp"));

            std::ostringstream oss;
            Assert::AreEqual(1, OdrCop2::OdrViolationReporter::ReportOdrViolations(maps, oss), "there should be no ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: NS::(unnamed enum: a)\n"
                            "[tu1.cpp]\n"
                            "enum NS::(unnamed enum: a) { a=0 };\n"
                            "[tu2.cpp]\n"
                            "enum NS::(unnamed enum: a) { a=0, b=1 };\n"
                            , oss.str());
        }
    },
    {"Two nameless enums inside anonymous namespaces that are used in a struct", []
        {
            std::string code1 = "namespace { enum { A = 1    }; } struct S { int x = A; };";
            std::string code2 = "namespace { enum { A = 1, B }; } struct S { int x = A; };";
    
            OdrCop2::AllMaps maps;
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, { "-x", "c++", "-std=c++23" }, "tu1.cpp"));
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++", "-std=c++23" }, "tu2.cpp"));

            std::ostringstream oss;
            Assert::AreEqual(1, OdrCop2::OdrViolationReporter::ReportOdrViolations(maps, oss), "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: S\n"
                            "[tu1.cpp]\n"
                            "struct S { // sizeof=4\n"
                            "   int x=enum (anonymous namespace)::(unnamed enum) { A=1 }::A;\n"
                            "};\n"
                            "[tu2.cpp]\n"
                            "struct S { // sizeof=4\n"
                            "   int x=enum (anonymous namespace)::(unnamed enum) { A=1, B=2 }::A;\n"
                            "};\n", oss.str());
        }
    },
    {"Two nameless enums inside anonymous namespaces that are returned from a method", []
        {
            std::string code1 = "namespace { enum { A = 1    }; } struct S { auto Foo() { return A; } };";
            std::string code2 = "namespace { enum { A = 1, B }; } struct S { auto Foo() { return A; } };";

            OdrCop2::AllMaps maps;
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, { "-x", "c++", "-std=c++23" }, "tu1.cpp"));
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++", "-std=c++23" }, "tu2.cpp"));

            std::ostringstream oss;
            Assert::AreEqual(2, OdrCop2::OdrViolationReporter::ReportOdrViolations(maps, oss), "there should be 2 ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: S::Foo()\n"
                            "[tu1.cpp]\n"
                            "enum (anonymous namespace)::(unnamed enum) { A=1 } __cdecl S::Foo() { return A; }\n"
                            "[tu2.cpp]\n"
                            "enum (anonymous namespace)::(unnamed enum) { A=1, B=2 } __cdecl S::Foo() { return A; }\n"
                            "\n"
                            "ODR VIOLATION: S\n"
                            "[tu1.cpp]\n"
                            "struct S { // sizeof=1\n"
                            "   enum (anonymous namespace)::(unnamed enum) { A=1 } __cdecl Foo() { return A; }\n"
                            "};\n"
                            "[tu2.cpp]\n"
                            "struct S { // sizeof=1\n"
                            "   enum (anonymous namespace)::(unnamed enum) { A=1, B=2 } __cdecl Foo() { return A; }\n"
                            "};\n", oss.str());
        }
    },
    {"Two identical nameless enums are args to a method are not an ODR violation", []
        {
            std::string code1 = "struct S { enum { A, B } E; void f(decltype(S::E) value) { (void)value; } };";
            std::string code2 = "struct S { enum { A, B } E; void f(decltype(S::E) value) { (void)value; } };";

            OdrCop2::AllMaps maps;
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, { "-x", "c++", "-std=c++23" }, "tu1.cpp"));
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++", "-std=c++23" }, "tu2.cpp"));

            std::ostringstream oss;
            Assert::AreEqual(0, OdrCop2::OdrViolationReporter::ReportOdrViolations(maps, oss), "there should be 1 ODR violation");
            Assert::AreEqual("", oss.str());
        }
    },
    {"Two non-identical nameless enums are args to a method are an ODR violation", []
        {
            std::string code1 = "struct S { enum { A,   } E; void f(decltype(S::E) value) { (void)value; } };";
            std::string code2 = "struct S { enum { A, B } E; void f(decltype(S::E) value) { (void)value; } };";

            OdrCop2::AllMaps maps;
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, { "-x", "c++", "-std=c++23" }, "tu1.cpp"));
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++", "-std=c++23" }, "tu2.cpp"));

            std::ostringstream oss;
            Assert::AreEqual(2, OdrCop2::OdrViolationReporter::ReportOdrViolations(maps, oss), "there should be 2 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: S::(unnamed enum: A)\n"
                            "[tu1.cpp]\n"
                            "enum S::(unnamed enum: A) { A=0 };\n"
                            "[tu2.cpp]\n"
                            "enum S::(unnamed enum: A) { A=0, B=1 };\n"
                            "\n"
                            "ODR VIOLATION: S\n"
                            "[tu1.cpp]\n"
                            "struct S { // sizeof=4\n"
                            "enum S::(unnamed enum: A) { A=0 };   enum S::(unnamed enum: A) { A=0 } E;\n"
                            "   void __cdecl f(decltype(S::E) value) { (void)value; }\n"
                            "};\n"
                            "[tu2.cpp]\n"
                            "struct S { // sizeof=4\n"
                            "enum S::(unnamed enum: A) { A=0, B=1 };   enum S::(unnamed enum: A) { A=0, B=1 } E;\n"
                            "   void __cdecl f(decltype(S::E) value) { (void)value; }\n"
                            "};\n", oss.str());
        }
    },
    {"test for using names enum value A as a template parameter", []
        {
            {   // nameless enum
                std::string code1 = "template<int N>"
                                    "struct X { char ar[N]; };"
                                    "enum { A=1 };"
                                    "void Foo(X<A>&) {}";
                std::string code2 = "template<int N>"
                                    "struct X { char ar[N]; };"
                                    "enum { A=2 };"
                                    "void Foo(X<A>&) {}";

                OdrCop2::AllMaps maps;
                Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, { "-x", "c++", "-std=c++23" }, "tu1.cpp"));
                Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++", "-std=c++23" }, "tu2.cpp"));

                std::ostringstream oss;
                Assert::AreEqual(1, OdrCop2::OdrViolationReporter::ReportOdrViolations(maps, oss), "there should be 1 ODR violation");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: (unnamed enum: A)\n"
                                "[tu1.cpp]\n"
                                "enum (unnamed enum: A) { A=1 };\n"
                                "[tu2.cpp]\n"
                                "enum (unnamed enum: A) { A=2 };\n"
                                , oss.str());
            }

            {   // enum in anonymous namespace
                std::string code1 = "template<int N> struct X {"
                                    "    char ar[N];"
                                    "    void method(){}"
                                    "    static void staticMethod(){}"
                                    "};"
                                    "namespace { enum { A=1 }; }"
                                    "template struct X<A>;";

                std::string code2 = "template<int N> struct X {"
                                    "    char ar[N];"
                                    "    void method(){}"
                                    "    static void staticMethod(){}"
                                    "};"
                                    "namespace { enum { A=2,B,C,D,E,F }; }"
                                    "template struct X<A>;";


                OdrCop2::AllMaps maps;
                Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, { "-x", "c++", "-std=c++23" }, "tu1.cpp"));
                Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++", "-std=c++23" }, "tu2.cpp"));

                std::ostringstream oss;
                Assert::AreEqual(0, OdrCop2::OdrViolationReporter::ReportOdrViolations(maps, oss), "there should be no ODR violation");
                Assert::AreEqual("", oss.str());
            }
        }
    },
};