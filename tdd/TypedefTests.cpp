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
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            const auto& vec = maps.typedefMap.begin()->second;
            Assert::AreEqual(1, vec.size(), "there should be 1 element in vector of TypedefInfo structs");
            Assert::AreEqual("INT = int", vec[0].fullyQualified, "should have found typedef");
        }
    },
	{"can find using-style typedefs in the AST", []
		{
			std::string code = R"(using INT=int;)";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            const auto& vec = maps.typedefMap.begin()->second;
            Assert::AreEqual(1, vec.size(), "there should be 1 element in vector of TypedefInfo structs");
            Assert::AreEqual("INT = int", vec[0].fullyQualified, "should have found typedef");
        }
    },
    {"can get a typedef's underlying type from another typedef", []
        {
            std::string code = R"(using INT=int;)"
                               R"(typedef INT ANOTHER_INT;)";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);

            Assert::AreEqual(2, maps.typedefMap.size(), "there should be two different typedefs in the map");

            auto it1 = maps.typedefMap.begin();
            auto it2 = std::next(it1);

            // alphabetized by key, so ANOTHER_INT comes first
            Assert::AreEqual("ANOTHER_INT = int", it1->second[0].fullyQualified, "should have found typedef");
            Assert::AreEqual(        "INT = int", it2->second[0].fullyQualified, "should have found using");
        }
    },
};