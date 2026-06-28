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
    {"Used as a function return type", []
        {
            std::string code1 = "namespace { struct Config {  int width;  int height; }; } Config MakeConfig() { return {0,0}; }";
            std::string code2 = "namespace { struct Config { char width; char height; }; } Config MakeConfig() { return {0,0}; }";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.functionMap.size());
            Assert::AreEqual("struct (anonymous namespace)::Config { // sizeof=8\n"
                             "   int width;\n"
                             "   int height;\n"
                             "} __cdecl MakeConfig() { return {0, 0}; }"
                           , maps.functionMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "these are overloads, not ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: MakeConfig()\n"
                                "[tu3.cpp]\n"
                                "struct (anonymous namespace)::Config { // sizeof=8\n"
                                "   int width;\n"
                                "   int height;\n"
                                "} __cdecl MakeConfig() { return {0, 0}; }\n"
                                "[tu4.cpp]\n"
                                "struct (anonymous namespace)::Config { // sizeof=2\n"
                                "   char width;\n"
                                "   char height;\n"
                                "} __cdecl MakeConfig() { return {0, 0}; }\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Used as a function static const return type reference", []
        {
            std::string code1 = "namespace { struct Config {  int width;  int height; }; } const Config& MakeConfig() { static Config cfg{0, 0}; return cfg; }";
            std::string code2 = "namespace { struct Config { char width; char height; }; } const Config& MakeConfig() { static Config cfg{0, 0}; return cfg; }";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.functionMap.size());
            Assert::AreEqual("const struct (anonymous namespace)::Config { // sizeof=8\n"
                             "         int width;\n"
                             "         int height;\n"
                             "      } & __cdecl MakeConfig() {\n"
                             "    static Config cfg{0, 0};\n"
                             "    return cfg;\n"
                             "}"
                           , maps.functionMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "these are overloads, not ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: MakeConfig()\n"
                                "[tu3.cpp]\n"
                                "const struct (anonymous namespace)::Config { // sizeof=8\n"
                                "         int width;\n"
                                "         int height;\n"
                                "      } & __cdecl MakeConfig() {\n"
                                "    static Config cfg{0, 0};\n"
                                "    return cfg;\n"
                                "}\n"
                                "[tu4.cpp]\n"
                                "const struct (anonymous namespace)::Config { // sizeof=2\n"
                                "         char width;\n"
                                "         char height;\n"
                                "      } & __cdecl MakeConfig() {\n"
                                "    static Config cfg{0, 0};\n"
                                "    return cfg;\n"
                                "}\n", output, "mismatched output");
            }
        }
    },
    {"Used as an external-linkage global", []
        {
            std::string code1 = "namespace { struct Config {  int width;  int height; }; } Config g_cfg;";
            std::string code2 = "namespace { struct Config { char width; char height; }; } Config g_cfg;;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.varMap.size());
            Assert::AreEqual("struct (anonymous namespace)::Config { // sizeof=8\n"
                             "   int width;\n"
                             "   int height;\n"
                             "} g_cfg;"
                           , maps.varMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "these are overloads, not ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: g_cfg\n"
                                "[tu3.cpp]\n"
                                "struct (anonymous namespace)::Config { // sizeof=8\n"
                                "   int width;\n"
                                "   int height;\n"
                                "} g_cfg;\n"
                                "[tu4.cpp]\n"
                                "struct (anonymous namespace)::Config { // sizeof=2\n"
                                "   char width;\n"
                                "   char height;\n"
                                "} g_cfg;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Used as an external-linkage global const pointer", []
        {
            std::string code1 = "namespace { struct Config {  int width;  int height; }; } const Config* g_cfg;";
            std::string code2 = "namespace { struct Config { char width; char height; }; } const Config* g_cfg;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.varMap.size());
            Assert::AreEqual("const struct (anonymous namespace)::Config { // sizeof=8\n"
                             "         int width;\n"
                             "         int height;\n"
                             "      } * g_cfg;"
                           , maps.varMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "these are overloads, not ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: g_cfg\n"
                                "[tu3.cpp]\n"
                                "const struct (anonymous namespace)::Config { // sizeof=8\n"
                                "         int width;\n"
                                "         int height;\n"
                                "      } * g_cfg;\n"
                                "[tu4.cpp]\n"
                                "const struct (anonymous namespace)::Config { // sizeof=2\n"
                                "         char width;\n"
                                "         char height;\n"
                                "      } * g_cfg;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Used as a template type arg of a class template used as an argument to a function", []
        {
            std::string code1 = "namespace { struct Config { int width; int height;            }; } template<typename T> struct Box { T value; }; void Process(Box<Config> box) {}";
            std::string code2 = "namespace { struct Config { int width; int height; int depth; }; } template<typename T> struct Box { T value; }; void Process(Box<Config> box) {}";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(2,  maps.udtMap.size());

            auto it = maps.udtMap.begin();
            Assert::AreEqual("template<typename T> struct Box {\n"
                             "   T value;\n"
                             "};"
                           , (*it++).second[0].fullyQualified, "serialization");
            Assert::AreEqual("template<> struct Box<struct (anonymous namespace)::Config { // sizeof=8\n"
                             "                         int width;\n"
                             "                         int height;\n"
                             "                      }> { // sizeof=8\n"
                             "   struct (anonymous namespace)::Config { // sizeof=8\n"
                             "      int width;\n"
                             "      int height;\n"
                             "   } value;\n"
                             "};"
                           , (*it++).second[0].fullyQualified, "serialization");
            Assert::AreEqual(1,  maps.functionMap.size());
            Assert::AreEqual("void __cdecl Process(Box<struct (anonymous namespace)::Config { // sizeof=8\n"
                             "                            int width;\n"
                             "                            int height;\n"
                             "                         }> box) {}"
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
    {"Used as a template type arg of a class template used as a const& argument to a function", []
        {
            std::string code1 = "namespace { struct Config { int width; int height;            }; } template<typename T> struct Box { T value; }; void Process(const Box<Config>& box) {}";
            std::string code2 = "namespace { struct Config { int width; int height; int depth; }; } template<typename T> struct Box { T value; }; void Process(const Box<Config>& box) {}";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1,  maps.udtMap.size()); 
            // N.B.:  NOTE: just adding const& to the function's argument caused the Box<Config> to become an implicit instantiation which are never traversed at the top-level.

            auto it = maps.udtMap.begin();
            Assert::AreEqual("template<typename T> struct Box {\n"
                             "   T value;\n"
                             "};"
                           , (*it++).second[0].fullyQualified, "serialization");
            //Assert::AreEqual("template<> struct Box<struct (anonymous namespace)::Config { // sizeof=8\n" // see N.B. comment, above
            //                 "                         int width;\n"
            //                 "                         int height;\n"
            //                 "                      }> { // sizeof=8\n"
            //                 "   struct (anonymous namespace)::Config { // sizeof=8\n"
            //                 "      int width;\n"
            //                 "      int height;\n"
            //                 "   } value;\n"
            //                 "};"
            //               , (*it++).second[0].fullyQualified, "serialization");
            Assert::AreEqual(1,  maps.functionMap.size());
            Assert::AreEqual("void __cdecl Process(const Box<struct (anonymous namespace)::Config { // sizeof=8\n"
                             "                                  int width;\n"
                             "                                  int height;\n"
                             "                               }> & box) {}"
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

    {"Used as a template type arg of a function template", []
        {
            std::string code1 = "namespace { struct Foo { int  x; }; } template <typename T> void Process(T val) {} void CallProcess() { Process(Foo{42}); }";
            std::string code2 = "namespace { struct Foo { char x; }; } template <typename T> void Process(T val) {} void CallProcess() { Process(Foo{42}); }";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(3, maps.functionMap.size());

            auto it = maps.functionMap.begin();
            Assert::AreEqual("void __cdecl CallProcess() { Process(Foo{42}); }", (*it++).second[0].fullyQualified, "serialization");
            Assert::AreEqual("void __cdecl Process<struct (anonymous namespace)::Foo { // sizeof=4\n"
                             "                        int x;\n"
                             "                     }>(struct (anonymous namespace)::Foo { // sizeof=4\n"
                             "                           int x;\n"
                             "                        } val) {}"
                           , (*it++).second[0].fullyQualified, "serialization");
            Assert::AreEqual("template <typename T> void __cdecl Process(T val) {}"
                           , (*it++).second[0].fullyQualified, "serialization");

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
    {"Used as a template type arg of a function template, pointer variation", []
        {
            std::string code1 = "namespace { struct Foo { int  x; }; } template <typename T> void Process(T* val) {} void CallProcess() { Foo f; Process(&f); }";
            std::string code2 = "namespace { struct Foo { char x; }; } template <typename T> void Process(T* val) {} void CallProcess() { Foo f; Process(&f); }";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(3, maps.functionMap.size());

            auto it = maps.functionMap.begin();
            Assert::AreEqual("void __cdecl CallProcess() {\n"
                             "    Foo f;\n"
                             "    Process(&f);\n"
                             "}"
                           , (*it++).second[0].fullyQualified, "serialization");
            Assert::AreEqual("void __cdecl Process<struct (anonymous namespace)::Foo { // sizeof=4\n"
                             "                        int x;\n"
                             "                     }>(struct (anonymous namespace)::Foo { // sizeof=4\n"
                             "                           int x;\n"
                             "                        } * val) {}"
                           , (*it++).second[0].fullyQualified, "serialization");
            Assert::AreEqual("template <typename T> void __cdecl Process(T * val) {}"
                           , (*it++).second[0].fullyQualified, "serialization");

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
    {"Used as a template type arg of a function template, const reference version", []
        {
            std::string code1 = "namespace { struct Foo { int  x; }; } template <typename T> void Process(const T& val) {} void CallProcess() { Process(Foo{42}); }";
            std::string code2 = "namespace { struct Foo { char x; }; } template <typename T> void Process(const T& val) {} void CallProcess() { Process(Foo{42}); }";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(3, maps.functionMap.size());

            auto it = maps.functionMap.begin();
            Assert::AreEqual("void __cdecl CallProcess() { Process(Foo{42}); }"
                          , (*it++).second[0].fullyQualified, "serialization");
            Assert::AreEqual("void __cdecl Process<struct (anonymous namespace)::Foo { // sizeof=4\n"
                             "                        int x;\n"
                             "                     }>(const struct (anonymous namespace)::Foo { // sizeof=4\n"
                             "                                 int x;\n"
                             "                              } & val) {}"
                           , (*it++).second[0].fullyQualified, "serialization");
            Assert::AreEqual("template <typename T> void __cdecl Process(const T & val) {}"
                           , (*it++).second[0].fullyQualified, "serialization");

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

    {"Used as an underlying type of a typedef", []
        {
            std::string code1 = "namespace { struct Impl { int x; int y;        }; } typedef Impl MyImpl; extern void consume(MyImpl*); void consume(MyImpl* p) { (void)p; }";
            std::string code2 = "namespace { struct Impl { int x; int y; int z; }; } typedef Impl MyImpl; extern void consume(MyImpl*);"; // note that consume is extern, no need to repeat it here

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(1, maps.typedefMap.size());
            Assert::AreEqual(1, maps.functionMap.size());

            Assert::AreEqual("void __cdecl consume(struct (anonymous namespace)::Impl { // sizeof=8\n"
                             "                        int x;\n"
                             "                        int y;\n"
                             "                     } * p) { (void)p; }"
                           , maps.functionMap.begin()->second[0].fullyQualified, "serialization");
            Assert::AreEqual("using MyImpl = struct (anonymous namespace)::Impl { // sizeof=8\n"
                             "                  int x;\n"
                             "                  int y;\n"
                             "               }; // typedef (anonymous namespace)::Impl MyImpl;"
                           , maps.typedefMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "these are overloads, not ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: MyImpl\n"
                                "[tu3.cpp]\n"
                                "using MyImpl = struct (anonymous namespace)::Impl { // sizeof=8\n"
                                "                  int x;\n"
                                "                  int y;\n"
                                "               }; // typedef (anonymous namespace)::Impl MyImpl;\n"
                                "[tu4.cpp]\n"
                                "using MyImpl = struct (anonymous namespace)::Impl { // sizeof=12\n"
                                "                  int x;\n"
                                "                  int y;\n"
                                "                  int z;\n"
                                "               }; // typedef (anonymous namespace)::Impl MyImpl;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Used as an underlying type of a using alias", []
        {
            std::string code1 = "namespace { struct Impl { int x; int y;        }; } using MyImpl = Impl; extern void consume(MyImpl*); void consume(MyImpl* p) { (void)p; }";
            std::string code2 = "namespace { struct Impl { int x; int y; int z; }; } using MyImpl = Impl; extern void consume(MyImpl*);"; // note that consume is extern, no need to repeat it here

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(1, maps.typedefMap.size());
            Assert::AreEqual(1, maps.functionMap.size());

            Assert::AreEqual("void __cdecl consume(struct (anonymous namespace)::Impl { // sizeof=8\n"
                             "                        int x;\n"
                             "                        int y;\n"
                             "                     } * p) { (void)p; }"
                           , maps.functionMap.begin()->second[0].fullyQualified, "serialization");
            Assert::AreEqual("using MyImpl = struct (anonymous namespace)::Impl { // sizeof=8\n"
                             "                  int x;\n"
                             "                  int y;\n"
                             "               }; // typedef (anonymous namespace)::Impl MyImpl;"
                           , maps.typedefMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "these are overloads, not ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: MyImpl\n"
                                "[tu3.cpp]\n"
                                "using MyImpl = struct (anonymous namespace)::Impl { // sizeof=8\n"
                                "                  int x;\n"
                                "                  int y;\n"
                                "               }; // typedef (anonymous namespace)::Impl MyImpl;\n"
                                "[tu4.cpp]\n"
                                "using MyImpl = struct (anonymous namespace)::Impl { // sizeof=12\n"
                                "                  int x;\n"
                                "                  int y;\n"
                                "                  int z;\n"
                                "               }; // typedef (anonymous namespace)::Impl MyImpl;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Used as the underlying type for a using alias template", []
        {
            std::string code1 = "namespace { struct Impl { int x; int y;        }; } template <typename T> using MyAlias = Impl; extern void consume(MyAlias<int>*); void consume(MyAlias<int>* p) { (void)p; }";
            std::string code2 = "namespace { struct Impl { int x; int y; int z; }; } template <typename T> using MyAlias = Impl; extern void consume(MyAlias<int>*);";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(1, maps.typedefMap.size());
            Assert::AreEqual(1, maps.functionMap.size());

            Assert::AreEqual("void __cdecl consume(struct (anonymous namespace)::Impl { // sizeof=8\n"
                             "                        int x;\n"
                             "                        int y;\n"
                             "                     } * p) { (void)p; }"
                           , maps.functionMap.begin()->second[0].fullyQualified, "serialization");
            Assert::AreEqual("template <T> using MyAlias = struct (anonymous namespace)::Impl { // sizeof=8\n"
                             "                                int x;\n"
                             "                                int y;\n"
                             "                             }; // no typedef equivalent"
                , maps.typedefMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "these are overloads, not ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: MyAlias\n"
                                "[tu3.cpp]\n"
                                "template <T> using MyAlias = struct (anonymous namespace)::Impl { // sizeof=8\n"
                                "                                int x;\n"
                                "                                int y;\n"
                                "                             }; // no typedef equivalent\n"
                                "[tu4.cpp]\n"
                                "template <T> using MyAlias = struct (anonymous namespace)::Impl { // sizeof=12\n"
                                "                                int x;\n"
                                "                                int y;\n"
                                "                                int z;\n"
                                "                             }; // no typedef equivalent\n"
                              , output, "mismatched output");
            }
        }
    },

};