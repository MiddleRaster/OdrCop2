#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"
#include <DbgHelp.h>

Test ExploratoryTestsOfClangAST[] =
{
    {"Get TU name", []
        {
            std::string code = R"(void foo() {})";

            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual("input.cc", vec[0].TU, "should have gotten the TU name");
        }
    },
    {"Trying out 'inline namespace' syntax", []
        {
            std::string code = 
R"(
namespace SomeNamespace
{
    inline namespace V2 { int foo() { return 42; } }
           namespace V1 { int foo() { return 41; } }
}
)";
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            Assert::AreEqual(2, maps.functionMap.size(), "should have found 2 functions");

            auto vec1 = maps.functionMap["?foo@V2@SomeNamespace@@YAHXZ"];
            auto vec2 = maps.functionMap["?foo@V1@SomeNamespace@@YAHXZ"];
            Assert::AreEqual("?foo@V2@SomeNamespace@@YAHXZ", vec1[0].mangled, "should return the mangled name");
            Assert::AreEqual("?foo@V1@SomeNamespace@@YAHXZ", vec2[0].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(vec1[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("int __cdecl SomeNamespace::V2::foo(void)", std::string(buf, result), "should unmangle back to original function name");
                  result = UnDecorateSymbolName(vec2[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("int __cdecl SomeNamespace::V1::foo(void)", std::string(buf, result), "should unmangle back to original function name");
        }
    },
    {"playing with anonymous namespaces", []
        {
            std::string code =
R"(
namespace { struct Helper { int x; }; }
void process(Helper h) { (void)h; }
)";
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual(1, vec.size(), "should have found 1 function");
            Assert::AreEqual("?process@@YAXUHelper@?A0x87D7C4E@@@Z", vec[0].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(vec[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("void __cdecl process(struct `anonymous namespace'::Helper)", std::string(buf, result), "should unmangle back to function with anonymous namespace arg");

            // try out different canonicalizations
            result = UnDecorateSymbolName("?process@@YAXUHelper@?A0x00000000@@@Z",         buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("void __cdecl process(struct `anonymous namespace'::Helper)", std::string(buf, result), "should unmangle back to anonymous namespace arg");
            result = UnDecorateSymbolName("?process@@YAXUHelper@?A0x0000000@@@Z",          buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("void __cdecl process(struct `anonymous namespace'::Helper)", std::string(buf, result), "should unmangle back to anonymous namespace arg");
            result = UnDecorateSymbolName("?process@@YAXUHelper@?A0x0@@@Z",                buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("void __cdecl process(struct `anonymous namespace'::Helper)", std::string(buf, result), "should unmangle back to anonymous namespace arg");
            result = UnDecorateSymbolName("?process@@YAXUHelper@?A0x@@@Z",                 buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("void __cdecl process(struct `anonymous namespace'::Helper)", std::string(buf, result), "should unmangle back to anonymous namespace arg");
        }
    },
    {"playing with nameless unions/structs/classes, take 1", []
        {
            std::string code =
R"(
struct Outer                            // anonymous struct as a parameter type
{
    struct { int x; int y; };           // anonymous struct — members promoted to Outer scope
};
Outer makeOuter() { return Outer{}; }   // Function returning a type containing an anonymous struct
)";
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual(1, vec.size(), "should have found 1 function");
            Assert::AreEqual("?makeOuter@@YA?AUOuter@@XZ", vec[0].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(vec[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("struct Outer __cdecl makeOuter(void)", std::string(buf, result), "should unmangle back to function with nameless struct arg");
        }
    },
    {"playing with nameless unions/structs/classes, take 2", []
        {
            std::string code =
R"(
union Variant
{
    struct { float r; float g; float b; };
    int raw;
};
Variant makeVariant() { return Variant{}; }
)";
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual(1, vec.size(), "should have found 1 function");
            Assert::AreEqual("?makeVariant@@YA?ATVariant@@XZ", vec[0].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(vec[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("union Variant __cdecl makeVariant(void)", std::string(buf, result), "should unmangle back to function with nameless struct arg");
        }
    },
    {"playing with nameless unions/structs/classes, take 3", []
        {
            std::string code =
R"(
using uint8_t = unsigned char;
using uint32_t = unsigned int;
struct Pixel
{
    union
    {
        struct { uint8_t r, g, b, a; }; // anonymous struct
        uint32_t packed;
    };
    uint32_t getPacked() const { return packed; }
};
)";
            OdrCop2::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code, { "-x", "c++-cpp-output" });
            Assert::IsTrue(ok);
            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual(1, vec.size(), "should have found 1 function");
            Assert::AreEqual("?getPacked@Pixel@@QEBAIXZ", vec[0].mangled, "should return the mangled name");

            char buf[1024];
            DWORD result = UnDecorateSymbolName(vec[0].mangled.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
            Assert::AreEqual("public: unsigned int __cdecl Pixel::getPacked(void)const __ptr64", std::string(buf, result), "should unmangle back to function with nameless struct arg");
        }
    },
};

struct OdrViolationReporter
{
    static int ReportEnumOdrViolations(const std::map<std::string, std::vector<OdrCop2::EnumInfo>>& map, auto&& out)
    {
        /*
        ODR VIOLATION: Hi::Color
          [tu1.cpp]
            enum class Hi::Color : int { Red=0, Green=1, Blue=2 };
          [tu2.cpp]
            enum Hi::Color { Red=0, Green=1, Blue=2 };
          [tu3.cpp]
            enum Hi::Color : int { Red=0, Green=1, Blue=2 };
        */
        int violationCount = 0;
        for (auto& [name, items] : map)
        {
            if (items.size() < 2)
                continue;

            //if (true == skipAnonymous(items[0]))
            //    continue;

            if (std::all_of(items.begin() + 1, items.end(), [&](const auto& x) { return x == items[0]; }))
                continue;

            // find mismatch index
            //int mismatch = -1;
            //for (size_t m = 1; m < items.size(); ++m)
            //{
            //    if (-1 != (mismatch = getMismatchIndex(items[0], items[m])))
            //        break;
            //}

            ++violationCount;
            out << "ODR VIOLATION: " << name << '\n';

            std::vector<bool> printed(items.size(), false);
            for (size_t i=0; i<items.size(); ++i)
            {
                if (printed[i])
                    continue;

                out << "  ["  << items[i].TU << "]\n";
                out << "    " << items[i].fullyQualified << '\n';
                printed[i] = true;

                for (size_t j=i+1; j<items.size(); ++j)
                {
                    if (!printed[j] && (items[i] == items[j]))
                    {
                        out << items[j].TU << " - same as above\n";
                        printed[j] = true;
                    }
                }
            }
        }
        return violationCount;
    }
    static int ReportTypedefOdrViolations(const std::map<std::string, std::vector<OdrCop2::TypedefInfo>>& map, auto&& out)
    {
        /*
        ODR VIOLATION: typedef/alias Hi::INT
          [tu1.cpp]
            Hi::INT = int
          [tu2.cpp]
            Hi::INT = unsigned int
        */
        int violationCount = 0;
        for (auto& [name, items] : map)
        {
            if (items.size() < 2)
                continue;

            //if (true == skipAnonymous(items[0]))
            //    continue;

            if (std::all_of(items.begin() + 1, items.end(), [&](const auto& x) { return x == items[0]; }))
                continue;

            // find mismatch index
            //int mismatch = -1;
            //for (size_t m = 1; m < items.size(); ++m)
            //{
            //    if (-1 != (mismatch = getMismatchIndex(items[0], items[m])))
            //        break;
            //}

            ++violationCount;
            out << "ODR VIOLATION: typedef/alias " << name << '\n';

            std::vector<bool> printed(items.size(), false);
            for (size_t i=0; i<items.size(); ++i)
            {
                if (printed[i])
                    continue;

                out << "  ["  << items[i].TU << "]\n";
                out << "    " << items[i].fullyQualified << '\n';
                printed[i] = true;

                for (size_t j=i+1; j<items.size(); ++j)
                {
                    if (!printed[j] && (items[i] == items[j]))
                    {
                        out << "  [" << items[j].TU << "] - same as above\n";
                        printed[j] = true;
                    }
                }
            }
        }
        return violationCount;
    }
    static int ReportFunctionOdrViolations(const std::map<std::string, std::vector<OdrCop2::FunctionInfo>>& map, auto&& out)
    {
        /*
        ODR VIOLATION: ?foo@@YAXXZ
          [tu1.cpp]
            inline void __cdecl foo(void);
          [tu2.cpp]
            void __cdecl foo(void);
          [tu3.cpp] - same as above
        */
        int violationCount = 0;
        for (auto& [name, items] : map)
        {
            if (items.size() < 2)
                continue;

            //if (true == skipAnonymous(items[0]))
            //    continue;

            if (std::all_of(items.begin() + 1, items.end(), [&](const auto& x) { return x == items[0]; }))
                continue;

            // find mismatch index
            //int mismatch = -1;
            //for (size_t m = 1; m < items.size(); ++m)
            //{
            //    if (-1 != (mismatch = getMismatchIndex(items[0], items[m])))
            //        break;
            //}

            ++violationCount;
            out << "ODR VIOLATION: " << name << '\n';

            std::vector<bool> printed(items.size(), false);
            for (size_t i=0; i<items.size(); ++i)
            {
                if (printed[i])
                    continue;

                out << "  ["  << items[i].TU << "]\n";
                out << "    " << items[i].fullyQualified << '\n';
                printed[i] = true;

                for (size_t j=i+1; j<items.size(); ++j)
                {
                    if (!printed[j] && (items[i] == items[j]))
                    {
                        out << "  [" << items[j].TU << "] - same as above\n";
                        printed[j] = true;
                    }
                }
            }
        }
        return 1;
    }
};

Test ExploringOdrViolationReportingTests[] =
{
    {"Report enum ODR violations", []
        {
            std::string code1 = R"(namespace Hi { enum class Color : int { Red, Green, Blue}; })";
            std::string code2 = R"(namespace Hi { enum       Color       { Red, Green, Blue}; })";
            std::string code3 = R"(namespace Hi { enum       Color : int { Red, Green, Blue}; })";

            OdrCop2::AllMaps maps;
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, { "-x", "c++-cpp-output" }, "tu1.cpp"));
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++-cpp-output" }, "tu2.cpp"));
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code3, { "-x", "c++-cpp-output" }, "tu3.cpp"));

            Assert::AreEqual(1, maps.enumMap.size(), "should be only one enum name");

            const auto& vec = maps.enumMap.begin()->second;
            Assert::AreNotEqual(vec[0].fullyQualified, vec[1].fullyQualified, "first and second enums are different");
            Assert::AreNotEqual(vec[0].fullyQualified, vec[2].fullyQualified, "first and third enums are different");
            Assert::AreNotEqual(vec[1].fullyQualified, vec[2].fullyQualified, "second and third enums are different");

            std::ostringstream oss;
            int violations = OdrViolationReporter::ReportEnumOdrViolations(maps.enumMap, oss);
            Assert::AreEqual(1, violations, "should have been one ODR violation");
            Assert::AreEqual("ODR VIOLATION: Hi::Color\n  [tu1.cpp]\n    enum class Hi::Color : int { Red=0, Green=1, Blue=2 };\n  [tu2.cpp]\n    enum Hi::Color { Red=0, Green=1, Blue=2 };\n  [tu3.cpp]\n    enum Hi::Color : int { Red=0, Green=1, Blue=2 };\n", oss.str());
        }
    },
    {"Report typedef/using ODR violations", []
        {
            std::string code1 = R"(namespace Hi { typedef int INT; })";
            std::string code2 = R"(namespace Hi { using INT = unsigned int; })";

            OdrCop2::AllMaps maps;
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, { "-x", "c++-cpp-output" }, "tu1.cpp"));
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++-cpp-output" }, "tu2.cpp"));
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++-cpp-output" }, "tu3.cpp"));

            Assert::AreEqual(1, maps.typedefMap.size(), "should be only one typedef name");

            const auto& vec = maps.typedefMap.begin()->second;
            Assert::AreNotEqual(vec[0].fullyQualified, vec[1].fullyQualified, "first and second typedefs are different");
            Assert::AreEqual   (vec[1].fullyQualified, vec[2].fullyQualified, "first and second typedefs are the same");

            std::ostringstream oss;
            int violations = OdrViolationReporter::ReportTypedefOdrViolations(maps.typedefMap, oss);
            Assert::AreEqual(1, violations, "should have been one ODR violation");
            Assert::AreEqual("ODR VIOLATION: typedef/using Hi::INT\n  [tu1.cpp]\n    Hi::INT = int\n  [tu2.cpp]\n    Hi::INT = unsigned int\n  [tu3.cpp] - same as above\n", oss.str());
        }
    },
    {"Report function ODR violations", []
        {
            std::string code1 = "inline void foo(){}";
            std::string code2 =        "void foo(){}";

            OdrCop2::AllMaps maps;
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, { "-x", "c++-cpp-output" }, "tu1.cpp"));
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++-cpp-output" }, "tu2.cpp"));
            Assert::AreEqual(true, clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++-cpp-output" }, "tu3.cpp"));

            Assert::AreEqual(1, maps.functionMap.size(), "should be only one function name");

            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreNotEqual(vec[0].fullyQualified, vec[1].fullyQualified, "first and second functions are different");
            Assert::AreEqual   (vec[1].fullyQualified, vec[2].fullyQualified, "first and second functions are the same");

            std::ostringstream oss;
            int violations = OdrViolationReporter::ReportFunctionOdrViolations(maps.functionMap, oss);
            Assert::AreEqual(1, violations, "should have been one ODR violation");
            Assert::AreEqual("ODR VIOLATION: ?foo@@YAXXZ\n  [tu1.cpp]\n    inline void __cdecl foo(void)\n  [tu2.cpp]\n    void __cdecl foo(void)\n  [tu3.cpp] - same as above\n", oss.str());
        }
    },
};
