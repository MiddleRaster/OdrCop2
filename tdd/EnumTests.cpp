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
            Assert::AreEqual("enum Color { Red=0, Green=1, Blue=2, };", vec[0].fullyQualified, "should have found enum");
        }
    },
    {"Can find an enum inside a namespace", []
        {
            std::string code = R"(namespace Hi { enum Color { Red, Green, Blue}; })";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            const auto& vec = maps.enumMap.begin()->second;
            Assert::AreEqual("enum Hi::Color { Red=0, Green=1, Blue=2, };", vec[0].fullyQualified, "should have found enum");
        }
    },
};