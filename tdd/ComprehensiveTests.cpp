#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"

Test ComprehensiveTests[] =
{
    {"TU34-001: Identical external-linkage struct definitions. Expected ODR violation: NO.", []
        {
            std::string code1 = "namespace OdrCopTU34Tests {"
                                "    struct IdenticalStruct {"
                                "        int a;"
                                "        double b;"
                                "    };"
                                "    IdenticalStruct g_3_001{1, 2.0};"
                                "}";

            std::string code2 = "namespace OdrCopTU34Tests {"
                                "   struct IdenticalStruct {"
                                "       int a;"
                                "       double b;"
                                "   };"
                                "   IdenticalStruct g_4_001{1, 2.0};"
                                "}";

            OdrCop2::AllMaps maps;
            Assert::IsTrue(clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, { "-x", "c++" }));
            Assert::IsTrue(clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++" }));

            std::string out;
            std::ostringstream oss(out);
            Assert::AreEqual(0, OdrCop2::OdrViolationReporter::ReportOdrViolations(maps, oss), "there should be no ODR violations");
            Assert::AreEqual("", out, "there should be no output");
        }
    },
};
