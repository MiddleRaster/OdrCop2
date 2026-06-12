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
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            const auto& vec = maps.enumMap.begin()->second;
            Assert::AreEqual("enum Color { Red=0, Green=1, Blue=2 };", vec[0].fullyQualified, "should have found enum");
        }
    },
    {"Can find an enum inside a namespace", []
        {
            std::string code = R"(namespace Hi { enum Color { Red, Green, Blue}; })";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
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
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, { "-x", "c++-cpp-output" }, "tu1.cpp"));
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++-cpp-output" }, "tu2.cpp"));
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code3, { "-x", "c++-cpp-output" }, "tu3.cpp"));

            auto& vec = maps.enumMap.begin()->second;
            Assert::AreEqual(3, vec.size(), "should have been 3 enums of the same name");

            Assert::AreEqual("tu1.cpp", vec[0].TU, "tu1 should be first");
            Assert::AreEqual("tu2.cpp", vec[1].TU, "tu2 should be second");
            Assert::AreEqual("tu3.cpp", vec[2].TU, "tu3 should be second");

            Assert::AreEqual("enum class Hi::Color : int " "{ Red=0, Green=1, Blue=2 };", vec[0].fullyQualified, "should have 'class' and underlying type");
            Assert::AreEqual("enum"    " Hi::Color "       "{ Red=0, Green=1, Blue=2 };", vec[1].fullyQualified, "should not have 'class' nor underlying type");
            Assert::AreEqual("enum"    " Hi::Color : int " "{ Red=0, Green=1, Blue=2 };", vec[2].fullyQualified, "should not have 'class' but does have underlying type");
        }
    },
};