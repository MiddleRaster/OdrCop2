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
    {"Used as the underlying type for a nested using alias template", []
        {
            std::string code1 = "namespace { struct Impl { int x; int y;        }; } struct Outer { template <typename T> using MyAlias = Impl; }; extern void consume(Outer*); void consume(Outer* p) { (void)p; }";
            std::string code2 = "namespace { struct Impl { int x; int y; int z; }; } struct Outer { template <typename T> using MyAlias = Impl; }; extern void consume(Outer*);";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(1, maps.typedefMap.size());
            Assert::AreEqual(1, maps.functionMap.size());

            Assert::AreEqual("void __cdecl consume(Outer * p) { (void)p; }"
                           , maps.functionMap.begin()->second[0].fullyQualified, "serialization");
            Assert::AreEqual("template <T> using Outer::MyAlias = struct (anonymous namespace)::Impl { // sizeof=8\n"
                             "                                       int x;\n"
                             "                                       int y;\n"
                             "                                    }; // no typedef equivalent"
                           , maps.typedefMap.begin()->second[0].fullyQualified, "serialization");
            Assert::AreEqual("struct Outer { // sizeof=1\n"
                            "template <T> using Outer::MyAlias = struct (anonymous namespace)::Impl { // sizeof=8\n"
                            "                                       int x;\n"
                            "                                       int y;\n"
                            "                                    }; // no typedef equivalent\n"
                            "};"
                           , maps.udtMap.begin()->second[0].fullyQualified, "serialization");


            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(2, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: Outer::MyAlias\n"
                                "[tu3.cpp]\n"
                                "template <T> using Outer::MyAlias = struct (anonymous namespace)::Impl { // sizeof=8\n"
                                "                                       int x;\n"
                                "                                       int y;\n"
                                "                                    }; // no typedef equivalent\n"
                                "[tu4.cpp]\n"
                                "template <T> using Outer::MyAlias = struct (anonymous namespace)::Impl { // sizeof=12\n"
                                "                                       int x;\n"
                                "                                       int y;\n"
                                "                                       int z;\n"
                                "                                    }; // no typedef equivalent\n"
                                "\n"
                                "ODR VIOLATION: Outer\n"
                                "[tu3.cpp]\n"
                                "struct Outer { // sizeof=1\n"
                                "template <T> using Outer::MyAlias = struct (anonymous namespace)::Impl { // sizeof=8\n"
                                "                                       int x;\n"
                                "                                       int y;\n"
                                "                                    }; // no typedef equivalent\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "struct Outer { // sizeof=1\n"
                                "template <T> using Outer::MyAlias = struct (anonymous namespace)::Impl { // sizeof=12\n"
                                "                                       int x;\n"
                                "                                       int y;\n"
                                "                                       int z;\n"
                                "                                    }; // no typedef equivalent\n"
                                "};\n", output, "mismatched output");
            }
        }
    },
    {"Nameless and anonymous struct, used as the underlying type for a typedef", []
        {
            std::string code1 = "namespace { struct { int x; int y;       } AnonInstance; } typedef decltype(AnonInstance) PointAlias;";
            std::string code2 = "namespace { struct { int x; int y; int z;} AnonInstance; } typedef decltype(AnonInstance) PointAlias;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(1, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            Assert::AreEqual("using PointAlias = struct (anonymous namespace)::(unnamed struct at tu1.cpp:1:13) { // sizeof=8\n"
                             "                      int x;\n"
                             "                      int y;\n"
                             "                   }; // typedef (anonymous namespace)::(unnamed struct at tu1.cpp:1:13) PointAlias;"
                , maps.typedefMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: PointAlias\n"
                                "[tu3.cpp]\n"
                                "using PointAlias = struct (anonymous namespace)::(unnamed struct at tu3.cpp:1:13) { // sizeof=8\n"
                                "                      int x;\n"
                                "                      int y;\n"
                                "                   }; // typedef (anonymous namespace)::(unnamed struct at tu3.cpp:1:13) PointAlias;\n"
                                "[tu4.cpp]\n"
                                "using PointAlias = struct (anonymous namespace)::(unnamed struct at tu4.cpp:1:13) { // sizeof=12\n"
                                "                      int x;\n"
                                "                      int y;\n"
                                "                      int z;\n"
                                "                   }; // typedef (anonymous namespace)::(unnamed struct at tu4.cpp:1:13) PointAlias;\n"
                              , output, "mismatched output");
            }
        }
    },
};

Test AnonymousEnums[] =
{
    {"Anonymous namespace enum used as a field", []
        {
            std::string code1 = "namespace { enum Color { Red, Green, Blue        }; } struct Outer { Color field; }; extern void consume(Outer*); void consume(Outer* p) { (void)p; }";
            std::string code2 = "namespace { enum Color { Red, Green, Blue, Alpha }; } struct Outer { Color field; }; extern void consume(Outer*);";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(1, maps.functionMap.size());

            Assert::AreEqual("void __cdecl consume(Outer * p) { (void)p; }"
                           , maps.functionMap.begin()->second[0].fullyQualified, "serialization");
            Assert::AreEqual("struct Outer { // sizeof=4\n"
                             "   enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } field;\n"
                             "};"
                           , maps.udtMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: Outer\n"
                                "[tu3.cpp]\n"
                                "struct Outer { // sizeof=4\n"
                                "   enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } field;\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "struct Outer { // sizeof=4\n"
                                "   enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2, Alpha=3 } field;\n"
                                "};\n", output, "mismatched output");
            }
        }
    },
    {"Anonymous namespace enum used as a field where it's an array", []
        {
            std::string code1 = "namespace { enum Color { Red, Green, Blue        }; } struct Outer { Color field[2]; }; extern void consume(Outer*); void consume(Outer* p) { (void)p; }";
            std::string code2 = "namespace { enum Color { Red, Green, Blue, Alpha }; } struct Outer { Color field[2]; }; extern void consume(Outer*);";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(1, maps.functionMap.size());

            Assert::AreEqual("void __cdecl consume(Outer * p) { (void)p; }"
                           , maps.functionMap.begin()->second[0].fullyQualified, "serialization");
            Assert::AreEqual("struct Outer { // sizeof=8\n"
                             "   enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } field[2];\n"
                             "};"
                           , maps.udtMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: Outer\n"
                                "[tu3.cpp]\n"
                                "struct Outer { // sizeof=8\n"
                                "   enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } field[2];\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "struct Outer { // sizeof=8\n"
                                "   enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2, Alpha=3 } field[2];\n"
                                "};\n", output, "mismatched output");
            }
        }
    },
    {"Anonymous namespace enum used as an argument to a function", []
        {
            std::string code1 = "namespace { enum Color { Red, Green, Blue        }; } extern void consume(Color); void consume(Color c) { (void)c; }";
            std::string code2 = "namespace { enum Color { Red, Green, Blue, Alpha }; } extern void consume(Color);";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(1, maps.functionMap.size());

            Assert::AreEqual("void __cdecl consume(enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } c) { (void)c; }"
                           , maps.functionMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code1);
                Assert::AreEqual(0, violations, "wrong number of violations");
                Assert::AreEqual("", output, "mismatched output");
            }
            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(0, violations, "wrong number of ODR violations");
                Assert::AreEqual("", output, "mismatched output");
            }
        }
    },
    {"Anonymous namespace enum used as an array argument (which ALWAYS decays to a pointer) to a function", []
        {
            std::string code1 = "namespace { enum Color { Red, Green, Blue        }; } extern void consume(Color c[2]); void consume(Color c[2]) { (void)c; }";
            std::string code2 = "namespace { enum Color { Red, Green, Blue, Alpha }; } extern void consume(Color c[2]);";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(1, maps.functionMap.size());

            Assert::AreEqual("void __cdecl consume(enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } * c) { (void)c; }"
                           , maps.functionMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(0, violations, "wrong number of ODR violations");
                Assert::AreEqual("", output, "mismatched output");
            }
        }
    },
    {"Anonymous namespace enum used as a reference to a fixed-size array argument to a function", []
        {
            std::string code1 = "namespace { enum Color { Red, Green, Blue        }; } extern void consume(Color (&c)[2]); void consume(Color (&c)[2]) { (void)c; }";
            std::string code2 = "namespace { enum Color { Red, Green, Blue, Alpha }; } extern void consume(Color (&c)[2]);";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(1, maps.functionMap.size());

            Assert::AreEqual("void __cdecl consume(enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } (&c)[2]) { (void)c; }"
                           , maps.functionMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(0, violations, "wrong number of ODR violations");
                Assert::AreEqual("", output, "mismatched output");
            }
        }
    },

    {"Anonymous namespace enum used as a function return value", []
        {
            std::string code1 = "namespace { enum Color { Red, Green, Blue,       }; } extern Color consume(); Color consume() { return Red; }";
            std::string code2 = "namespace { enum Color { Red, Green, Blue, Alpha }; } extern Color consume();";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(1, maps.functionMap.size());

            Assert::AreEqual("enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } __cdecl consume() { return Red; }"
                           , maps.functionMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(0, violations, "wrong number of ODR violations");
                Assert::AreEqual("", output, "mismatched output");
            }
        }
    },
    {"Anonymous namespace enum used as a global variable", []
        {
            std::string code1 = "namespace { enum Color { Red, Green, Blue,       }; } extern Color globalColor; Color globalColor = Red;";
            std::string code2 = "namespace { enum Color { Red, Green, Blue, Alpha }; } extern Color globalColor;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(1, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            Assert::AreEqual("enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } globalColor;"
                           , maps.varMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: globalColor\n"
                                "[tu3.cpp]\n"
                                "enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } globalColor;\n"
                                "[tu3.cpp] - same as above\n"
                                "[tu4.cpp]\n"
                                "enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2, Alpha=3 } globalColor;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Anonymous namespace enum used as a global variable that is an array", []
        {
            std::string code1 = "namespace { enum Color { Red, Green, Blue,       }; } extern Color globalColor[2]; Color globalColor[2] = {Red, Blue};";
            std::string code2 = "namespace { enum Color { Red, Green, Blue, Alpha }; } extern Color globalColor[2];";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(1, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            Assert::AreEqual("enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } globalColor[2];"
                           , maps.varMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: globalColor\n"
                                "[tu3.cpp]\n"
                                "enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } globalColor[2];\n"
                                "[tu3.cpp] - same as above\n"
                                "[tu4.cpp]\n"
                                "enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2, Alpha=3 } globalColor[2];\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Anonymous namespace enum used as a global variable with no extern", []
        {
            std::string code1 = "namespace { enum Color { Red, Green, Blue,       }; }        Color globalColor = Red;";
            std::string code2 = "namespace { enum Color { Red, Green, Blue, Alpha }; } extern Color globalColor;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(1, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            Assert::AreEqual("enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } globalColor;"
                           , maps.varMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: globalColor\n"
                                "[tu3.cpp]\n"
                                "enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } globalColor;\n"
                                "[tu4.cpp]\n"
                                "enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2, Alpha=3 } globalColor;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Anonymous namespace enum used as an inline global variable", []
        {
            std::string code1 = "namespace { enum Color { Red, Green, Blue }; } inline Color globalColor = Red;";
            std::string code2 = "namespace { enum Color { Red, Green, Blue }; } inline Color globalColor;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(1, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            Assert::AreEqual("inline enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } globalColor = Red;"
                           , maps.varMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: globalColor\n"
                                "[tu3.cpp]\n"
                                "inline enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } globalColor = Red;\n"
                                "[tu4.cpp]\n"
                                "inline enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } globalColor;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Anonymous namespace enum used as an inline global variable pointer", []
        {
            std::string code1 = "namespace { enum Color { Red, Green, Blue }; } Color c = Red;  inline const Color* globalColor = &c;";
            std::string code2 = "namespace { enum Color { Red, Green, Blue }; } Color c = Blue; inline const Color* globalColor = &c;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(2, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());


            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } c;"
                               , (*it++).second[0].fullyQualified, "serialization");
                Assert::AreEqual("inline const enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } * globalColor = &c;"
                               , (*it++).second[0].fullyQualified, "serialization");
            }

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(0, violations, "wrong number of ODR violations");
                Assert::AreEqual("", output, "mismatched output");
            }
        }
    },

    {"Anonymous namespace enum used as the type of a global variable that is an array", []
        {
            std::string code1 = "namespace { enum Color { Red, Green, Blue        }; } inline Color globalColor[2] = {Red,Green};";
            std::string code2 = "namespace { enum Color { Red, Green, Blue, Alpha }; } inline Color globalColor[2] = {Red,Green};";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(1, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("inline enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } globalColor[2]{Red, Green};"
                               , (*it++).second[0].fullyQualified, "serialization");
            }

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: globalColor\n"
                                "[tu3.cpp]\n"
                                "inline enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } globalColor[2]{Red, Green};\n"
                                "[tu4.cpp]\n"
                                "inline enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2, Alpha=3 } globalColor[2]{Red, Green};\n"
                              , output, "mismatched output");
            }
        }
    },



    {"Anonymous namespace enum used as a template type arg", []
        {
            std::string code1 = "namespace { enum Color { Red, Green, Blue };        } template<typename T> struct Box { T value; }; extern void consume(Box<Color>*); void consume(Box<Color>* p) { (void)p; }";
            std::string code2 = "namespace { enum Color { Red, Green, Blue, Alpha }; } template<typename T> struct Box { T value; }; extern void consume(Box<Color>*);";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(1, maps.functionMap.size());

            Assert::AreEqual("template<typename T> struct Box {\n"
                             "   T value;\n"
                             "};"
                            , maps.udtMap.begin()->second[0].fullyQualified, "serialization");
            Assert::AreEqual("void __cdecl consume(Box<enum (anonymous namespace)::Color> * p) { (void)p; }"
                            , maps.functionMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(0, violations, "wrong number of ODR violations");
                Assert::AreEqual("", output, "mismatched output");
            }
        }
    },
    {"Anonymous namespace enum used as a non-type template argument", []
        {
            std::string code1 = "namespace { enum Color { Red, Green, Blue,       }; } template<Color C> struct Box { }; extern void consume(Box<Red>*); void consume(Box<Red>* p) { (void)p; }";
            std::string code2 = "namespace { enum Color { Red, Green, Blue, Alpha }; } template<Color C> struct Box { }; extern void consume(Box<Red>*);";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(1, maps.functionMap.size());

            Assert::AreEqual("template<enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } C> struct Box {\n"
                             "};"
                            , maps.udtMap.begin()->second[0].fullyQualified, "serialization");
            Assert::AreEqual("void __cdecl consume(Box<enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 }::Red> * p) { (void)p; }"
                            , maps.functionMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: Box<>\n"
                                "[tu3.cpp]\n"
                                "template<enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } C> struct Box {\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "template<enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2, Alpha=3 } C> struct Box {\n"
                                "};\n", output, "mismatched output");
            }
        }
    },

    {"Anonymous namespace enum used as a typedef or using alias", []
        {
            std::string code1 = "namespace { enum Color { Red, Green, Blue,       }; } typedef Color ColorAlias;";
            std::string code2 = "namespace { enum Color { Red, Green, Blue, Alpha }; } using ColorAlias = Color;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(1, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            Assert::AreEqual("using ColorAlias = enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 }; // typedef enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } ColorAlias;"
                            , maps.typedefMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: ColorAlias\n"
                                "[tu3.cpp]\n"
                                "using ColorAlias = enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 }; // typedef enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } ColorAlias;\n"
                                "[tu4.cpp]\n"
                                "using ColorAlias = enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2, Alpha=3 }; // typedef enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2, Alpha=3 } ColorAlias;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Nameless enum used as a typedef or using alias", []
        {
            std::string code1 =            "typedef enum { Red, Green, Blue,       } ColorAlias;";
            std::string code2 = "using ColorAlias = enum { Red, Green, Blue, Alpha };";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(1, maps.enumMap.size());
            Assert::AreEqual(1, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            Assert::AreEqual("using ColorAlias = enum (unnamed enum: Red) { Red=0, Green=1, Blue=2 }; // typedef enum (unnamed enum: Red) { Red=0, Green=1, Blue=2 } ColorAlias;"
                            , maps.typedefMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(2, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: (unnamed enum: Red)\n"
                                "[tu3.cpp]\n"
                                "enum (unnamed enum: Red) { Red=0, Green=1, Blue=2 };\n"
                                "[tu4.cpp]\n"
                                "enum (unnamed enum: Red) { Red=0, Green=1, Blue=2, Alpha=3 };\n"
                                "\n"
                                "ODR VIOLATION: ColorAlias\n"
                                "[tu3.cpp]\n"
                                "using ColorAlias = enum (unnamed enum: Red) { Red=0, Green=1, Blue=2 }; // typedef enum (unnamed enum: Red) { Red=0, Green=1, Blue=2 } ColorAlias;\n"
                                "[tu4.cpp]\n"
                                "using ColorAlias = enum (unnamed enum: Red) { Red=0, Green=1, Blue=2, Alpha=3 }; // typedef enum (unnamed enum: Red) { Red=0, Green=1, Blue=2, Alpha=3 } ColorAlias;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Nameless enum in an anonymous namespace used as a typedef or using alias", []
        {
            std::string code1 = "namespace { enum { Red, Green, Blue,       }; } typedef decltype(Red) ColorAlias;";
            std::string code2 = "namespace { enum { Red, Green, Blue, Alpha }; } using ColorAlias = decltype(Red);";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(1, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            Assert::AreEqual("using ColorAlias = enum (anonymous namespace)::(unnamed enum: Red) { Red=0, Green=1, Blue=2 }; // typedef enum (anonymous namespace)::(unnamed enum: Red) { Red=0, Green=1, Blue=2 } ColorAlias;"
                            , maps.typedefMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: ColorAlias\n"
                                "[tu3.cpp]\n"
                                "using ColorAlias = enum (anonymous namespace)::(unnamed enum: Red) { Red=0, Green=1, Blue=2 }; // typedef enum (anonymous namespace)::(unnamed enum: Red) { Red=0, Green=1, Blue=2 } ColorAlias;\n"
                                "[tu4.cpp]\n"
                                "using ColorAlias = enum (anonymous namespace)::(unnamed enum: Red) { Red=0, Green=1, Blue=2, Alpha=3 }; // typedef enum (anonymous namespace)::(unnamed enum: Red) { Red=0, Green=1, Blue=2, Alpha=3 } ColorAlias;\n"
                              , output, "mismatched output");
            }
        }
    },

};

Test AnonymousFunctions[] =
{
    {"Function in an anonymous namespace used as a non-type template parameter", []
        {
            std::string code1 = "namespace { int Func(int x) { return x + 1; } } template<int(*F)(int)> struct Wrapper {}; typedef Wrapper<Func> WrapperAlias;";
            std::string code2 = "namespace { int Func(int x) { return x + 2; } } template<int(*F)(int)> struct Wrapper {}; typedef Wrapper<Func> WrapperAlias;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(1, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            Assert::AreEqual("template<int (*F)(int)> struct Wrapper {\n"
                             "};", maps.udtMap.begin()->second[0].fullyQualified, "serialization");

            Assert::AreEqual("using WrapperAlias = Wrapper<&(int __cdecl (anonymous namespace)::Func(int x) { return x + 1; })>; // typedef Wrapper<&(int __cdecl (anonymous namespace)::Func(int x) { return x + 1; })> WrapperAlias;"
                            , maps.typedefMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: WrapperAlias\n"
                                "[tu3.cpp]\n"
                                "using WrapperAlias = Wrapper<&(int __cdecl (anonymous namespace)::Func(int x) { return x + 1; })>; // typedef Wrapper<&(int __cdecl (anonymous namespace)::Func(int x) { return x + 1; })> WrapperAlias;\n"
                                "[tu4.cpp]\n"
                                "using WrapperAlias = Wrapper<&(int __cdecl (anonymous namespace)::Func(int x) { return x + 2; })>; // typedef Wrapper<&(int __cdecl (anonymous namespace)::Func(int x) { return x + 2; })> WrapperAlias;\n"
                               , output, "mismatched output");
            }
        }
    },
    {"Two-line Function in an anonymous namespace used as a non-type template parameter to test indenting", []
        {
            std::string code1 = "namespace { int Func(int x) { int y = x+1; return y; } } template<int(*F)(int)> struct Wrapper {}; typedef Wrapper<Func> WrapperAlias;";
            std::string code2 = "namespace { int Func(int x) { int y = x+2; return y; } } template<int(*F)(int)> struct Wrapper {}; typedef Wrapper<Func> WrapperAlias;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(1, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            Assert::AreEqual("template<int (*F)(int)> struct Wrapper {\n"
                             "};", maps.udtMap.begin()->second[0].fullyQualified, "serialization");

            Assert::AreEqual("using WrapperAlias = Wrapper<&(int __cdecl (anonymous namespace)::Func(int x) {\n"
                             "                                   int y = x + 1;\n"
                             "                                   return y;\n"
                             "                               })>; // typedef Wrapper<&(int __cdecl (anonymous namespace)::Func(int x) {\n"
                             "                                                             int y = x + 1;\n"
                             "                                                             return y;\n"
                             "                                                         })> WrapperAlias;"
                           , maps.typedefMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: WrapperAlias\n"
                                "[tu3.cpp]\n"
                                "using WrapperAlias = Wrapper<&(int __cdecl (anonymous namespace)::Func(int x) {\n"
                                "                                   int y = x + 1;\n"
                                "                                   return y;\n"
                                "                               })>; // typedef Wrapper<&(int __cdecl (anonymous namespace)::Func(int x) {\n"
                                "                                                             int y = x + 1;\n"
                                "                                                             return y;\n"
                                "                                                         })> WrapperAlias;\n"
                                "[tu4.cpp]\n"
                                "using WrapperAlias = Wrapper<&(int __cdecl (anonymous namespace)::Func(int x) {\n"
                                "                                   int y = x + 2;\n"
                                "                                   return y;\n"
                                "                               })>; // typedef Wrapper<&(int __cdecl (anonymous namespace)::Func(int x) {\n"
                                "                                                             int y = x + 2;\n"
                                "                                                             return y;\n"
                                "                                                         })> WrapperAlias;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Three two-line Functions in an anonymous namespace used as a non-type template parameter to test indenting", []
        {
            std::string code1 = "namespace { int Foo(int x) { int y = x+1; return y; } void Bar() { int x=1; --x; } void Baz() { int y=1; y--; } } template<int(*F)(int), typename T, void(*B)(), void(*Z)()> struct Wrapper {}; typedef Wrapper<Foo,int,Bar,Baz> WrapperAlias;";
            std::string code2 = "namespace { int Foo(int x) { int y = x+2; return y; } void Bar() { int x=1; --x; } void Baz() { int y=1; y--; } } template<int(*F)(int), typename T, void(*B)(), void(*Z)()> struct Wrapper {}; typedef Wrapper<Foo,int,Bar,Baz> WrapperAlias;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(1, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            Assert::AreEqual("template<int (*F)(int), typename T, void (*B)(), void (*Z)()> struct Wrapper {\n"
                             "};", maps.udtMap.begin()->second[0].fullyQualified, "serialization");

            Assert::AreEqual("using WrapperAlias = Wrapper<&(int __cdecl (anonymous namespace)::Foo(int x) {\n"
                             "                                   int y = x + 1;\n"
                             "                                   return y;\n"
                             "                               }), int, &(void __cdecl (anonymous namespace)::Bar() {\n"
                             "                                              int x = 1;\n"
                             "                                              --x;\n"
                             "                                          }), &(void __cdecl (anonymous namespace)::Baz() {\n"
                             "                                                    int y = 1;\n"
                             "                                                    y--;\n"
                             "                                                })>; // typedef Wrapper<&(int __cdecl (anonymous namespace)::Foo(int x) {\n"
                             "                                                                              int y = x + 1;\n"
                             "                                                                              return y;\n"
                             "                                                                          }), int, &(void __cdecl (anonymous namespace)::Bar() {\n"
                             "                                                                                         int x = 1;\n"
                             "                                                                                         --x;\n"
                             "                                                                                     }), &(void __cdecl (anonymous namespace)::Baz() {\n"
                             "                                                                                               int y = 1;\n"
                             "                                                                                               y--;\n"
                             "                                                                                           })> WrapperAlias;"
                           , maps.typedefMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: WrapperAlias\n"
                                "[tu3.cpp]\n"
                                "using WrapperAlias = Wrapper<&(int __cdecl (anonymous namespace)::Foo(int x) {\n"
                                "                                   int y = x + 1;\n"
                                "                                   return y;\n"
                                "                               }), int, &(void __cdecl (anonymous namespace)::Bar() {\n"
                                "                                              int x = 1;\n"
                                "                                              --x;\n"
                                "                                          }), &(void __cdecl (anonymous namespace)::Baz() {\n"
                                "                                                    int y = 1;\n"
                                "                                                    y--;\n"
                                "                                                })>; // typedef Wrapper<&(int __cdecl (anonymous namespace)::Foo(int x) {\n"
                                "                                                                              int y = x + 1;\n"
                                "                                                                              return y;\n"
                                "                                                                          }), int, &(void __cdecl (anonymous namespace)::Bar() {\n"
                                "                                                                                         int x = 1;\n"
                                "                                                                                         --x;\n"
                                "                                                                                     }), &(void __cdecl (anonymous namespace)::Baz() {\n"
                                "                                                                                               int y = 1;\n"
                                "                                                                                               y--;\n"
                                "                                                                                           })> WrapperAlias;\n"
                                "[tu4.cpp]\n"
                                "using WrapperAlias = Wrapper<&(int __cdecl (anonymous namespace)::Foo(int x) {\n"
                                "                                   int y = x + 2;\n"
                                "                                   return y;\n"
                                "                               }), int, &(void __cdecl (anonymous namespace)::Bar() {\n"
                                "                                              int x = 1;\n"
                                "                                              --x;\n"
                                "                                          }), &(void __cdecl (anonymous namespace)::Baz() {\n"
                                "                                                    int y = 1;\n"
                                "                                                    y--;\n"
                                "                                                })>; // typedef Wrapper<&(int __cdecl (anonymous namespace)::Foo(int x) {\n"
                                "                                                                              int y = x + 2;\n"
                                "                                                                              return y;\n"
                                "                                                                          }), int, &(void __cdecl (anonymous namespace)::Bar() {\n"
                                "                                                                                         int x = 1;\n"
                                "                                                                                         --x;\n"
                                "                                                                                     }), &(void __cdecl (anonymous namespace)::Baz() {\n"
                                "                                                                                               int y = 1;\n"
                                "                                                                                               y--;\n"
                                "                                                                                           })> WrapperAlias;\n"
                              , output, "mismatched output");
            }
        }
    },

};

Test AnonymousObject[] =
{
    {"Used as the type for a global variable", []
        {
            std::string code1 = "namespace { struct Foo { int a; double b; }; Foo instance; } decltype(instance) other;";
            std::string code2 = "namespace { struct Foo { int a; long   b; }; Foo instance; } decltype(instance) other;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(1, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            Assert::AreEqual("struct (anonymous namespace)::Foo { // sizeof=16\n"
                             "   int a;\n"
                             "   double b;\n"
                             "} other;"
                           , maps.varMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: other\n"
                                "[tu3.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=16\n"
                                "   int a;\n"
                                "   double b;\n"
                                "} other;\n"
                                "[tu4.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=8\n"
                                "   int a;\n"
                                "   long b;\n"
                                "} other;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Used as the type for a global variable with decltype(()) which monkeys with &", []
        {
            std::string code1 = "namespace { struct Foo { int a; double b; }; Foo instance; } decltype((instance)) other = instance;";
            std::string code2 = "namespace { struct Foo { int a; long   b; }; Foo instance; } decltype( instance ) other = instance;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(1, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            Assert::AreEqual("struct (anonymous namespace)::Foo { // sizeof=16\n"
                             "   int a;\n"
                             "   double b;\n"
                             "} & other;"
                           , maps.varMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: other\n"
                                "[tu3.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=16\n"
                                "   int a;\n"
                                "   double b;\n"
                                "} & other;\n"
                                "[tu4.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=8\n"
                                "   int a;\n"
                                "   long b;\n"
                                "} other;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Used as a const type for a global variable:  has internal-linkage!", []
        {
            std::string code1 = "namespace { struct Foo { int a; double b; }; const Foo instance{1,2}; } decltype(instance) other = instance;";
            std::string code2 = "namespace { struct Foo { int a; long   b; };       Foo instance{1,2}; } decltype(instance) other = instance;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(0, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());
        }
    },
    {"Used as a const type for an extern global variable", []
        {
            std::string code1 = "namespace { struct Foo { int a; double b; }; const Foo instance{1,2}; } extern decltype(instance) other = instance;";
            std::string code2 = "namespace { struct Foo { int a; long   b; };       Foo instance{1,2}; }        decltype(instance) other = instance;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(1, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            Assert::AreEqual("const struct (anonymous namespace)::Foo { // sizeof=16\n"
                             "         int a;\n"
                             "         double b;\n"
                             "      } other;"
                           , maps.varMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: other\n"
                                "[tu3.cpp]\n"
                                "const struct (anonymous namespace)::Foo { // sizeof=16\n"
                                "         int a;\n"
                                "         double b;\n"
                                "      } other;\n"
                                "[tu4.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=8\n"
                                "   int a;\n"
                                "   long b;\n"
                                "} other;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Used with an anonymous namespace enum as well to test composition", []
        {
            std::string code1 = "namespace { enum Color { Red, Green, Blue        }; Color current = Red; } decltype(current) other_color;";
            std::string code2 = "namespace { enum Color { Red, Green, Blue, Alpha }; Color current = Red; } decltype(current) other_color;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(1, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            Assert::AreEqual("enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } other_color;"
                           , maps.varMap.begin()->second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: other_color\n"
                                "[tu3.cpp]\n"
                                "enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2 } other_color;\n"
                                "[tu4.cpp]\n"
                                "enum (anonymous namespace)::Color { Red=0, Green=1, Blue=2, Alpha=3 } other_color;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Used as part of a chained decltype", []
        {
            std::string code1 = "namespace { struct Foo { int a;        }; Foo instance; } decltype(instance) a; decltype(a) b;";
            std::string code2 = "namespace { struct Foo { int a; int b; }; Foo instance; } decltype(instance) a; decltype(a) b;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(2, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            auto it = maps.varMap.begin();
            Assert::AreEqual("struct (anonymous namespace)::Foo { // sizeof=4\n"
                             "   int a;\n"
                             "} a;"
                           , (*it++).second[0].fullyQualified, "serialization");
            Assert::AreEqual("struct (anonymous namespace)::Foo { // sizeof=4\n"
                             "   int a;\n"
                             "} b;"
                           , (*it++).second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(2, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: a\n"
                                "[tu3.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=4\n"
                                "   int a;\n"
                                "} a;\n"
                                "[tu4.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=8\n"
                                "   int a;\n"
                                "   int b;\n"
                                "} a;\n"
                                "\n"
                                "ODR VIOLATION: b\n"
                                "[tu3.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=4\n"
                                "   int a;\n"
                                "} b;\n"
                                "[tu4.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=8\n"
                                "   int a;\n"
                                "   int b;\n"
                                "} b;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Used as the type of for an array", []
        {
            std::string code1 = "namespace { struct Foo { int a;        }; Foo instances[4]; } decltype(instances) other_array; decltype(instances[0]) other_elem = instances[1];";
            std::string code2 = "namespace { struct Foo { int a; int b; }; Foo instances[4]; } decltype(instances) other_array; decltype(instances[0]) other_elem = instances[1];";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(2, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            auto it = maps.varMap.begin();
            Assert::AreEqual("struct (anonymous namespace)::Foo { // sizeof=4\n"
                             "   int a;\n"
                             "} other_array[4];"
                           , (*it++).second[0].fullyQualified, "serialization");
            Assert::AreEqual("struct (anonymous namespace)::Foo { // sizeof=4\n"
                             "   int a;\n"
                             "} & other_elem;"
                           , (*it++).second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(2, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: other_array\n"
                                "[tu3.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=4\n"
                                "   int a;\n"
                                "} other_array[4];\n"
                                "[tu4.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=8\n"
                                "   int a;\n"
                                "   int b;\n"
                                "} other_array[4];\n"
                                "\n"
                                "ODR VIOLATION: other_elem\n"
                                "[tu3.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=4\n"
                                "   int a;\n"
                                "} & other_elem;\n"
                                "[tu4.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=8\n"
                                "   int a;\n"
                                "   int b;\n"
                                "} & other_elem;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Use as Pointer to the anonymous-namespace object", []
        {
            std::string code1 = "namespace { struct Foo { int a;        }; Foo instance; Foo* ptr = &instance; } decltype(ptr) same_ptr_type; decltype(*ptr) deref=instance;";
            std::string code2 = "namespace { struct Foo { int a; int b; }; Foo instance; Foo* ptr = &instance; } decltype(ptr) same_ptr_type; decltype(*ptr) deref=instance;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(2, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            auto it = maps.varMap.begin();
            Assert::AreEqual("struct (anonymous namespace)::Foo { // sizeof=4\n"
                            "   int a;\n"
                            "} & deref;"
                           , (*it++).second[0].fullyQualified, "serialization");
            Assert::AreEqual("struct (anonymous namespace)::Foo { // sizeof=4\n"
                             "   int a;\n"
                             "} * same_ptr_type;"
                           , (*it++).second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(2, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: deref\n"
                                "[tu3.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=4\n"
                                "   int a;\n"
                                "} & deref;\n"
                                "[tu4.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=8\n"
                                "   int a;\n"
                                "   int b;\n"
                                "} & deref;\n"
                                "\n"
                                "ODR VIOLATION: same_ptr_type\n"
                                "[tu3.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=4\n"
                                "   int a;\n"
                                "} * same_ptr_type;\n"
                                "[tu4.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=8\n"
                                "   int a;\n"
                                "   int b;\n"
                                "} * same_ptr_type;\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Used to initialize a global with a function pointer", []
        {
            std::string code1 = "namespace { enum Color{Red}; struct Foo { int a;        }; Foo(*factory)(const Color, volatile Foo*, int&); } decltype(factory) other_factory;";
            std::string code2 = "namespace { enum Color{Red}; struct Foo { int a; int b; }; Foo(*factory)(const Color, volatile Foo*, int&); } decltype(factory) other_factory;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(1, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            auto it = maps.varMap.begin();
            Assert::AreEqual("struct (anonymous namespace)::Foo { // sizeof=4\n"
                             "   int a;\n"
                             "} (*other_factory)(enum (anonymous namespace)::Color { Red=0 }, volatile struct (anonymous namespace)::Foo { // sizeof=4\n"
                             "                                                                            int a;\n"
                             "                                                                         } *, int &);"
                           , (*it++).second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: other_factory\n"
                                "[tu3.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=4\n"
                                "   int a;\n"
                                "} (*other_factory)(enum (anonymous namespace)::Color { Red=0 }, volatile struct (anonymous namespace)::Foo { // sizeof=4\n"
                                "                                                                            int a;\n"
                                "                                                                         } *, int &);\n"
                                "[tu4.cpp]\n"
                                "struct (anonymous namespace)::Foo { // sizeof=8\n"
                                "   int a;\n"
                                "   int b;\n"
                                "} (*other_factory)(enum (anonymous namespace)::Color { Red=0 }, volatile struct (anonymous namespace)::Foo { // sizeof=8\n"
                                "                                                                            int a;\n"
                                "                                                                            int b;\n"
                                "                                                                         } *, int &);\n"
                              , output, "mismatched output");
            }
        }
    },
    {"Used to initialize a global with a function pointer where one arg is a template instantiation", []
        {
            std::string code1 = "namespace { template <typename T> struct Box { T value; }; struct Foo { int a; }; Foo (*factory)(Box<int>); } decltype(factory) other_factory;";
            std::string code2 = "namespace { template <typename T> struct Box { T  type; }; struct Foo { int a; }; Foo (*factory)(Box<int>); } decltype(factory) other_factory;";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, {"-x", "c++", "-std=c++23"}, "tu1.cpp");
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size());
            Assert::AreEqual(1, maps.varMap.size());
            Assert::AreEqual(0, maps.enumMap.size());
            Assert::AreEqual(0, maps.typedefMap.size());
            Assert::AreEqual(0, maps.functionMap.size());

            auto it = maps.varMap.begin();
            Assert::AreEqual("struct (anonymous namespace)::Foo { // sizeof=4\n"
                             "   int a;\n"
                             "} (*other_factory)((anonymous namespace)::Box<int>);"
                           , (*it++).second[0].fullyQualified, "serialization");

            {
                const auto& [violations, output] = RunTest(code1, code2);
                Assert::AreEqual(0, violations, "wrong number of ODR violations"); // overloads, not odrs
                Assert::AreEqual("", output, "mismatched output");
            }
        }
    },

};