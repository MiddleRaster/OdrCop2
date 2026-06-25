#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"

/*
Legend:
•	✓ = meaningful, distinct test case
•	— = not applicable
•	(covered) = redundant with another test

Internal-linkage entity |	Data member type |	Base class |	Function parameter |	Function return |	Variable type |	Template type arg |	NTTP arg |	Alias/typedef member |
Class/struct/union      |           ✓        |      ✓      |            ✓          |            ✓       |       ✓         |	        ✓	      |     —    |  	    ✓            |  
Enum	                |           ✓        |      —      |        	✓          |        	✓       |   	✓         |         ✓         |	    ✓    |      	✓            |
Function                |       	—        |  	—      |        	—          |        	—       |   	—         |         —         | 	✓    |	        —            |
Object/variable         |       	—        |  	—      |        	—          |        	—       |   	✓         |     	—         | 	✓    |      	—            |
Class template          |       	✓        |  	✓      |        	✓          |        	✓       |   	✓         |     	✓         | 	—    |      	✓            |
Variable template       |       	—        |  	—      |        	—          |        	—       |   	✓         |     	—         | 	✓    |      	—            |
That gives 24 distinct cells.

Additional ODR-significant positions
These are worth separate tests because they are part of declaration identity:
Usage position            |	     Class	     |     Enum	   |         Function       |	Variable    |
Static data member type   |       	✓        |  	✓      |        	—           |   	—       |
Default function argument |	        ✓        |  	✓      |        	✓           |   	✓       |
Requires-clause/constraint|     	✓        |  	✓      |         	✓           |   	✓       |
Friend declaration        |  	    ✓        |  	✓      |        	✓           |   	✓       |
Deduction guide           |     	✓        |  	✓      |        	—           |   	—       |

Things I would
not
make separate rows
Candidate row                           |         	Reason                                                          |
Typedef in anonymous namespace	        |   Same underlying type identity.                                          |
Alias in anonymous namespace	        |   Same underlying type identity.                                          |
Trailing return type	                |   Same as return type.                                                    |
Using-declaration                       |	Usually reduces to one of the above cases.                              |
Lambda closure type                     |	Better tested as its own category, not as an anonymous-namespace type.  |
Anonymous namespace namespace itself    |	Not an entity type.                                                     |
*/

extern std::pair<int, std::string> RunTest(const std::string& code1, const std::string& code2);

Test AnonymousUdts[] =
{
    {"Struct used as a field", []
        {
            std::string code1 = "namespace { struct Point { int x; int y; }; } struct Widget { Point origin; };";
            std::string code2 = "namespace { struct Point { int x; int y; int z; }; } struct Widget { Point origin; };";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size());
            Assert::AreEqual("struct Widget { // sizeof=8\n"
                             "   struct (anonymous namespace)::Point { // sizeof=8\n"
                             "      int x;\n"
                             "      int y;\n"
                             "   } origin;\n"
                             "};"
                           , maps.udtMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: Widget\n"
                                "[tu3.cpp]\n"
                                "struct Widget { // sizeof=8\n"
                                "   struct (anonymous namespace)::Point { // sizeof=8\n"
                                "      int x;\n"
                                "      int y;\n"
                                "   } origin;\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "struct Widget { // sizeof=12\n"
                                "   struct (anonymous namespace)::Point { // sizeof=12\n"
                                "      int x;\n"
                                "      int y;\n"
                                "      int z;\n"
                                "   } origin;\n"
                                "};\n", output, "mismatched output");
            }
        }
    },
    {"Union used as a field", []
        {
            std::string code1 = "namespace { union Variant { int i; float f; double d; }; } struct Holder { Variant val; int tag; };";
            std::string code2 = "namespace { union Variant { int i; float f;           }; } struct Holder { Variant val; int tag; };";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size());
            Assert::AreEqual("struct Holder { // sizeof=16\n"
                             "   union (anonymous namespace)::Variant { // sizeof=8\n"
                             "      int i;\n"
                             "      float f;\n"
                             "      double d;\n"
                             "   } val;\n"
                             "   int tag;\n"
                             "};"
                           , maps.udtMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: Holder\n"
                                "[tu3.cpp]\n"
                                "struct Holder { // sizeof=16\n"
                                "   union (anonymous namespace)::Variant { // sizeof=8\n"
                                "      int i;\n"
                                "      float f;\n"
                                "      double d;\n"
                                "   } val;\n"
                                "   int tag;\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "struct Holder { // sizeof=8\n"
                                "   union (anonymous namespace)::Variant { // sizeof=4\n"
                                "      int i;\n"
                                "      float f;\n"
                                "   } val;\n"
                                "   int tag;\n"
                                "};\n", output, "mismatched output");
            }
        }
    },
    {"Used as a base class", []
        {
            std::string code1 = "namespace { struct Base { int x; int y;        }; } struct Derived : Base { int radius; };";
            std::string code2 = "namespace { struct Base { int x; int y; int z; }; } struct Derived : Base { int radius; };";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size());
            Assert::AreEqual("struct Derived : public struct (anonymous namespace)::Base { // sizeof=8\n"
                             "                           int x;\n"
                             "                           int y;\n"
                             "                        } { // sizeof=12\n"
                             "   int radius;\n"
                             "};"
                           , maps.udtMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: Derived\n"
                                "[tu3.cpp]\n"
                                "struct Derived : public struct (anonymous namespace)::Base { // sizeof=8\n"
                                "                           int x;\n"
                                "                           int y;\n"
                                "                        } { // sizeof=12\n"
                                "   int radius;\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "struct Derived : public struct (anonymous namespace)::Base { // sizeof=12\n"
                                "                           int x;\n"
                                "                           int y;\n"
                                "                           int z;\n"
                                "                        } { // sizeof=16\n"
                                "   int radius;\n"
                                "};\n", output, "mismatched output");
            }
        }
    },
    {"Used as a base class with different access specifiers", []
        {
            std::string code1 = "namespace { struct Base { int x; int y; }; } struct Derived : public    Base { int radius; };";
            std::string code2 = "namespace { struct Base { int x; int y; }; } struct Derived : protected Base { int radius; };";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size());
            Assert::AreEqual("struct Derived : public struct (anonymous namespace)::Base { // sizeof=8\n"
                             "                           int x;\n"
                             "                           int y;\n"
                             "                        } { // sizeof=12\n"
                             "   int radius;\n"
                             "};"
                           , maps.udtMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: Derived\n"
                                "[tu3.cpp]\n"
                                "struct Derived : public struct (anonymous namespace)::Base { // sizeof=8\n"
                                "                           int x;\n"
                                "                           int y;\n"
                                "                        } { // sizeof=12\n"
                                "   int radius;\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "struct Derived : protected struct (anonymous namespace)::Base { // sizeof=8\n"
                                "                              int x;\n"
                                "                              int y;\n"
                                "                           } { // sizeof=12\n"
                                "   int radius;\n"
                                "};\n", output, "mismatched output");
            }
        }
    }, 
    {"Used as base classes in different order", []
        {
            std::string code1 = "namespace { struct BaseA { int x; }; struct BaseB { int y; }; } struct Derived : BaseA, BaseB { int width; };";
            std::string code2 = "namespace { struct BaseA { int x; }; struct BaseB { int y; }; } struct Derived : BaseB, BaseA { int width; };";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size());
            Assert::AreEqual("struct Derived : public struct (anonymous namespace)::BaseA { // sizeof=4\n"
                             "                           int x;\n"
                             "                        }\n"
                             "               , public struct (anonymous namespace)::BaseB { // sizeof=4\n"
                             "                           int y;\n"
                             "                        } { // sizeof=12\n"
                             "   int width;\n"
                             "};"
                           , maps.udtMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: Derived\n"
                                "[tu3.cpp]\n"
                                "struct Derived : public struct (anonymous namespace)::BaseA { // sizeof=4\n"
                                "                           int x;\n"
                                "                        }\n"
                                "               , public struct (anonymous namespace)::BaseB { // sizeof=4\n"
                                "                           int y;\n"
                                "                        } { // sizeof=12\n"
                                "   int width;\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "struct Derived : public struct (anonymous namespace)::BaseB { // sizeof=4\n"
                                "                           int y;\n"
                                "                        }\n"
                                "               , public struct (anonymous namespace)::BaseA { // sizeof=4\n"
                                "                           int x;\n"
                                "                        } { // sizeof=12\n"
                                "   int width;\n"
                                "};\n", output, "mismatched output");
            }
        }
    },
    {"Used as a function argument", []
        {
            std::string code1 = "namespace { struct Config { int width; int height;            }; } void Render(Config cfg, int flags) { (void)flags; }";
            std::string code2 = "namespace { struct Config { int width; int height; int depth; }; } void Render(Config cfg, int flags) { (void)flags; };";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.functionMap.size());
            Assert::AreEqual("void __cdecl Render(struct (anonymous namespace)::Config { // sizeof=8\n"
                             "                       int width;\n"
                             "                       int height;\n"
                             "                    } cfg, int flags) { (void)flags; }"
                           , maps.functionMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(0, violations, "these are overloads, not ODR violations");
                Assert::AreEqual("", output, "mismatched output");
            }
        }
    },
    {"Used as a function argument const&", []
        {
            std::string code1 = "namespace { struct Config { int width; int height;            }; } void Render(const Config& cfg, int flags) { (void)flags; }";
            std::string code2 = "namespace { struct Config { int width; int height; int depth; }; } void Render(const Config& cfg, int flags) { (void)flags; };";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.functionMap.size());
            Assert::AreEqual("void __cdecl Render(const struct (anonymous namespace)::Config { // sizeof=8\n"
                             "                             int width;\n"
                             "                             int height;\n"
                             "                          } & cfg, int flags) { (void)flags; }"
                           , maps.functionMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(0, violations, "these are overloads, not ODR violations");
                Assert::AreEqual("", output, "mismatched output");
            }
        }
    },
};