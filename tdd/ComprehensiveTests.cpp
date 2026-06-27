#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"

std::pair<int,std::string> RunTest(const std::string& code1, const std::string& code2)
{
    OdrCop2::AllMaps maps;
    Assert::IsTrue(clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, { "-x", "c++", "-std=c++23" }, "tu3.cpp"), "compiler error");
    Assert::IsTrue(clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++", "-std=c++23" }, "tu4.cpp"), "compiler error");

    std::ostringstream oss;
    int violations = OdrCop2::OdrViolationReporter::ReportOdrViolations(maps, oss);
    return {violations, oss.str()};
}

Test ComprehensiveTests[] =
{
    {"TU34-001: Identical external-linkage struct definitions. Expected ODR violation: NO.", []
        {
            const auto& [violations, output] = RunTest( "namespace OdrCopTU34Tests { struct IdenticalStruct { int a; double b; }; }",
                                                        "namespace OdrCopTU34Tests { struct IdenticalStruct { int a; double b; }; }");
            Assert::AreEqual(0, violations, "there should be no ODR violations");
            Assert::AreEqual("", output, "there should be no output");
        }
    },
    {"TU34-002: Same external-linkage struct name, different member type. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentMemberType { int value; };"
                                                      , "struct DifferentMemberType { long value; };");

            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                             "ODR VIOLATION: DifferentMemberType\n"
                             "[tu3.cpp]\n"
                             "struct DifferentMemberType { // sizeof=4\n"
                             "   int value;\n"
                             "};\n"
                             "[tu4.cpp]\n"
                             "struct DifferentMemberType { // sizeof=4\n"
                             "   long value;\n"
                             "};\n", output);
        }
    },
    {"TU34-003: Same external-linkage struct name, same members in a different order. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentMemberOrder { int first; double second; };"
                                                    ,   "struct DifferentMemberOrder { double second; int first; };");

            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentMemberOrder\n"
                            "[tu3.cpp]\n"
                            "struct DifferentMemberOrder { // sizeof=16\n"
                            "   int first;\n"
                            "   double second;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentMemberOrder { // sizeof=16\n"
                            "   double second;\n"
                            "   int first;\n"
                            "};\n", output);
        }
    },
    {"TU34-004: Same external-linkage struct name, different member count. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentMemberCount { int only; };"
                                                    ,   "struct DifferentMemberCount { int only; int extra; };");

            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentMemberCount\n"
                            "[tu3.cpp]\n"
                            "struct DifferentMemberCount { // sizeof=4\n"
                            "   int only;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentMemberCount { // sizeof=8\n"
                            "   int only;\n"
                            "   int extra;\n"
                            "};\n", output);
        }
    },
    {"TU34-005: Same external-linkage enum name, identical enumerators and values. Expected ODR violation: NO.", []
        {
            const auto& [violations, output] = RunTest( "enum class IdenticalScopedEnum : int { zero = 0, one = 1 };"
                                                    ,   "enum class IdenticalScopedEnum : int { zero = 0, one = 1 };");

            Assert::AreEqual(0, violations, "there should be no ODR violation");
            Assert::AreEqual("", output);
        }
    },
    {"TU34-006: Same external-linkage enum name, different enumerator values. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "enum class DifferentEnumeratorValues : int { zero = 0, one = 1 };"
                                                    ,   "enum class DifferentEnumeratorValues : int { zero = 0, one = 2 };");

            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentEnumeratorValues\n"
                            "[tu3.cpp]\n"
                            "enum class DifferentEnumeratorValues : int { zero=0, one=1 };\n"
                            "[tu4.cpp]\n"
                            "enum class DifferentEnumeratorValues : int { zero=0, one=2 };\n", output);
        }
    },
    {"TU34-007: Same external-linkage enum name, different underlying type. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "enum class DifferentEnumUnderlyingType :          int { value = 1  };"
                                                    ,   "enum class DifferentEnumUnderlyingType : unsigned int { value = 1U };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentEnumUnderlyingType\n"
                            "[tu3.cpp]\n"
                            "enum class DifferentEnumUnderlyingType : int { value=1 };\n"
                            "[tu4.cpp]\n"
                            "enum class DifferentEnumUnderlyingType : unsigned int { value=1 };\n", output);
        }
    },
    {"TU34-008: Same external-linkage union name, identical members. Expected ODR violation: NO.", []
        {
            const auto& [violations, output] = RunTest( "union IdenticalUnion { int i; float f; };"
                                                    ,   "union IdenticalUnion { int i; float f; };");
            Assert::AreEqual(0, violations, "there should be no ODR violation");
            Assert::AreEqual("", output);
        }
    },
    {"TU34-009: Same external-linkage union name, different member type. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "union DifferentUnionMember { int i;  float f; };"
                                                    ,   "union DifferentUnionMember { int i; double f; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentUnionMember\n"
                            "[tu3.cpp]\n"
                            "union DifferentUnionMember { // sizeof=4\n"
                            "   int i;\n"
                            "   float f;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "union DifferentUnionMember { // sizeof=8\n"
                            "   int i;\n"
                            "   double f;\n"
                            "};\n", output);
        }
    },
    {"TU34-010: Same external-linkage class name, struct vs class default access differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest("struct StructVsClassDefaultAccess { int value; };"
                                                    ,   "class StructVsClassDefaultAccess"
                                                        "{"
                                                        "    int value;"
                                                        "public:"
                                                        "    StructVsClassDefaultAccess() : value(10) {}"
                                                        "    int get() const { return value; }"
                                                        "};");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: StructVsClassDefaultAccess\n"
                            "[tu3.cpp]\n"
                            "struct StructVsClassDefaultAccess { // sizeof=4\n"
                            "   int value;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "class StructVsClassDefaultAccess { // sizeof=4\n"
                            "   int value;\n"
                            "public:\n"
                            "   void __cdecl StructVsClassDefaultAccess() : value(10) {}\n"
                            "   int __cdecl get() const { return value; }\n"
                            "};\n", output);
        }
    },
    {"TU34-011: Same external-linkage class name, explicit member access differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "class DifferentMemberAccess"
                                                        "{"
                                                        "public:"
                                                        "    int publicValue;"
                                                        "private:"
                                                        "    int privateValue;"
                                                        "public:"
                                                        "    DifferentMemberAccess() : publicValue(11), privateValue(12) {}"
                                                        "    int getPrivateValue() const { return privateValue; }"
                                                        "};"
                                                    ,   "class DifferentMemberAccess"
                                                        "{"
                                                        "private:"
                                                        "    int publicValue;"
                                                        "public:"
                                                        "    int privateValue;"
                                                        "    DifferentMemberAccess() : publicValue(11), privateValue(12) {}"
                                                        "    int getPublicValue() const { return publicValue; }"
                                                        "};");

            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentMemberAccess\n"
                            "[tu3.cpp]\n"
                            "class DifferentMemberAccess { // sizeof=8\n"
                            "public:\n"
                            "   int publicValue;\n"
                            "private:\n"
                            "   int privateValue;\n"
                            "public:\n"
                            "   void __cdecl DifferentMemberAccess() : publicValue(11), privateValue(12) {}\n"
                            "   int __cdecl getPrivateValue() const { return privateValue; }\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "class DifferentMemberAccess { // sizeof=8\n"
                            "private:\n"
                            "   int publicValue;\n"
                            "public:\n"
                            "   int privateValue;\n"
                            "   void __cdecl DifferentMemberAccess() : publicValue(11), privateValue(12) {}\n"
                            "   int __cdecl getPublicValue() const { return publicValue; }\n"
                            "};\n", output);
        }
    },
    {"TU34-012: Same external-linkage class name, identical access and members. Expected ODR violation: NO.", []
        {
            const auto& [violations, output] = RunTest( "class IdenticalClass { public: int value; IdenticalClass() : value(12) {} };"
                                                    ,   "class IdenticalClass { public: int value; IdenticalClass() : value(12) {} };");
            Assert::AreEqual(0, violations, "there should be no ODR violation");
            Assert::AreEqual("", output);
        }
    },
    {"TU34-013: Same derived class name, different direct base class. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct BaseAForDifferentBase { int a; };"
                                                        "struct BaseBForDifferentBase { int b; };"
                                                        "struct DifferentBaseClass : BaseAForDifferentBase { int own; };"
                                                    ,   "struct BaseAForDifferentBase { int a; };"
                                                        "struct BaseBForDifferentBase { int b; };"
                                                        "struct DifferentBaseClass : BaseBForDifferentBase { int own; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentBaseClass\n"
                            "[tu3.cpp]\n"
                            "struct DifferentBaseClass : public BaseAForDifferentBase { // sizeof=8\n"
                            "   int own;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentBaseClass : public BaseBForDifferentBase { // sizeof=8\n"
                            "   int own;\n"
                            "};\n", output);
        }
    },
    {"TU34-014: Same derived class name, same bases but in a different order. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct BaseAForBaseOrder { int a; };"
                                                        "struct BaseBForBaseOrder { int b; };"
                                                        "struct DifferentBaseOrder : BaseAForBaseOrder, BaseBForBaseOrder { int own; };"
                                                    ,   "struct BaseAForBaseOrder { int a; };"
                                                        "struct BaseBForBaseOrder { int b; };"
                                                        "struct DifferentBaseOrder : BaseBForBaseOrder, BaseAForBaseOrder { int own; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentBaseOrder\n"
                            "[tu3.cpp]\n"
                            "struct DifferentBaseOrder : public BaseAForBaseOrder, public BaseBForBaseOrder { // sizeof=12\n"
                            "   int own;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentBaseOrder : public BaseBForBaseOrder, public BaseAForBaseOrder { // sizeof=12\n"
                            "   int own;\n"
                            "};\n", output);
        }
    },
    {"TU34-015: Same derived class name, non-virtual base vs virtual base. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct BaseForVirtualBase { int base; }; struct DifferentVirtualBase :         BaseForVirtualBase { int own; };"
                                                    ,   "struct BaseForVirtualBase { int base; }; struct DifferentVirtualBase : virtual BaseForVirtualBase { int own; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentVirtualBase\n"
                            "[tu3.cpp]\n"
                            "struct DifferentVirtualBase : public BaseForVirtualBase { // sizeof=8\n"
                            "   int own;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentVirtualBase : public virtual BaseForVirtualBase { // sizeof=24\n"
                            "   int own;\n"
                            "};\n", output);
        }
    },
    {"TU34-016: Same external-linkage class name, different virtual function table shape. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentVirtualShape { virtual int first() { return 16; }                                      int data; };"
                                                    ,   "struct DifferentVirtualShape { virtual int first() { return 16; } virtual int second() { return 160; } int data; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentVirtualShape\n"
                            "[tu3.cpp]\n"
                            "struct DifferentVirtualShape { // sizeof=16\n"
                            "   virtual int __cdecl first() { return 16; }\n"
                            "   int data;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentVirtualShape { // sizeof=16\n"
                            "   virtual int __cdecl first() { return 16; }\n"
                            "   virtual int __cdecl second() { return 160; }\n"
                            "   int data;\n"
                            "};\n", output);
        }
    },
    {"TU34-017: Same external-linkage class name, virtual function name differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentVirtualFunctionName { virtual int alpha() { return 17; } int data; };"
                                                    ,   "struct DifferentVirtualFunctionName { virtual int beta () { return 17; } int data; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentVirtualFunctionName\n"
                            "[tu3.cpp]\n"
                            "struct DifferentVirtualFunctionName { // sizeof=16\n"
                            "   virtual int __cdecl alpha() { return 17; }\n"
                            "   int data;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentVirtualFunctionName { // sizeof=16\n"
                            "   virtual int __cdecl beta() { return 17; }\n"
                            "   int data;\n"
                            "};\n", output);
        }
    },
    {"TU34-018: Same external-linkage class name, virtualness of a method differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentMethodVirtualness { virtual int value() { return 18; } int data; };"
                                                    ,   "struct DifferentMethodVirtualness {         int value() { return 18; } int data; };");
            Assert::AreEqual(2, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentMethodVirtualness::value()\n"
                            "[tu3.cpp]\n"
                            "virtual int __cdecl DifferentMethodVirtualness::value() { return 18; }\n"
                            "[tu4.cpp]\n"
                            "int __cdecl DifferentMethodVirtualness::value() { return 18; }\n"
                            "\n"
                            "ODR VIOLATION: DifferentMethodVirtualness\n"
                            "[tu3.cpp]\n"
                            "struct DifferentMethodVirtualness { // sizeof=16\n"
                            "   virtual int __cdecl value() { return 18; }\n"
                            "   int data;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentMethodVirtualness { // sizeof=4\n"
                            "   int __cdecl value() { return 18; }\n"
                            "   int data;\n"
                            "};\n", output);
        }
    },
    {"TU34-019: Same external-linkage class name, const qualification of a method differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentMethodConstness { int value() const { return 19; } int data; };"
                                                    ,   "struct DifferentMethodConstness { int value()       { return 19; } int data; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentMethodConstness\n"
                            "[tu3.cpp]\n"
                            "struct DifferentMethodConstness { // sizeof=4\n"
                            "   int __cdecl value() const { return 19; }\n"
                            "   int data;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentMethodConstness { // sizeof=4\n"
                            "   int __cdecl value() { return 19; }\n"
                            "   int data;\n"
                            "};\n", output);
        }
    },
    /*
    Careful readers will note that the number of violations above is 1, but the number of violations below is 2. You probably wonder why that is.
    A const method is an overload, if there's a non-const method of the same name. And that means that the mangled name must be different.
    On the other hand, with MSVC, noexcept is *not* part of mangling (but it part of their metadata that they use at runtime to call std::terminate if an except is thrown).
    For free functions with and without noexcept, this does exactly the right thing, but for methods it's a little odd:
        they mangle to the same thing and thus show up as ODR violations.
    I think this is a good thing.

    After doing a little TDD, I've uncovered the general rule:
    If an attribute doesn't participate in overload resolution, then it doesn't participate in mangling.
    */
    {"TU34-020: Same external-linkage class name, noexcept specification differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentMethodNoexcept { int value() noexcept { return 20; } int data; };"
                                                    ,   "struct DifferentMethodNoexcept { int value()          { return 20; } int data; };");
            Assert::AreEqual(2, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentMethodNoexcept::value()\n"
                            "[tu3.cpp]\n"
                            "int __cdecl DifferentMethodNoexcept::value() noexcept { return 20; }\n"
                            "[tu4.cpp]\n"
                            "int __cdecl DifferentMethodNoexcept::value() { return 20; }\n"
                            "\n"
                            "ODR VIOLATION: DifferentMethodNoexcept\n"
                            "[tu3.cpp]\n"
                            "struct DifferentMethodNoexcept { // sizeof=4\n"
                            "   int __cdecl value() noexcept { return 20; }\n"
                            "   int data;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentMethodNoexcept { // sizeof=4\n"
                            "   int __cdecl value() { return 20; }\n"
                            "   int data;\n"
                            "};\n", output);
        }
    },
    {"Same external-linkage class name, [[attributes]] differ. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentMethodAttributes {               int value() { return 20; } int data; };"
                                                    ,   "struct DifferentMethodAttributes { [[nodiscard]] int value() { return 20; } int data; };");
            Assert::AreEqual(2, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentMethodAttributes::value()\n"
                            "[tu3.cpp]\n"
                            "int __cdecl DifferentMethodAttributes::value() { return 20; }\n"
                            "[tu4.cpp]\n"
                            "[[nodiscard]] int __cdecl DifferentMethodAttributes::value() { return 20; }\n"
                            "\n"
                            "ODR VIOLATION: DifferentMethodAttributes\n"
                            "[tu3.cpp]\n"
                            "struct DifferentMethodAttributes { // sizeof=4\n"
                            "   int __cdecl value() { return 20; }\n"
                            "   int data;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentMethodAttributes { // sizeof=4\n"
                            "   [[nodiscard]] int __cdecl value() { return 20; }\n"
                            "   int data;\n"
                            "};\n", output);
        }
    },
    {"TU34-021: Same external-linkage class name, static data member type differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentStaticDataMember { static const int  value = 21;  int payload; };"
                                                    ,   "struct DifferentStaticDataMember { static const long value = 21L; int payload; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentStaticDataMember\n"
                            "[tu3.cpp]\n"
                            "struct DifferentStaticDataMember { // sizeof=4\n"
                            "   static const int value=21;\n"
                            "   int payload;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentStaticDataMember { // sizeof=4\n"
                            "   static const long value=21L;\n"
                            "   int payload;\n"
                            "};\n", output);
        }
    },
    {"TU34-022: Same external-linkage class name, static member function return type differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentStaticMemberFunction { static int  value() { return 22;  } int payload; };"
                                                    ,   "struct DifferentStaticMemberFunction { static long value() { return 22L; } int payload; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentStaticMemberFunction\n"
                            "[tu3.cpp]\n"
                            "struct DifferentStaticMemberFunction { // sizeof=4\n"
                            "   static int __cdecl value() { return 22; }\n"
                            "   int payload;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentStaticMemberFunction { // sizeof=4\n"
                            "   static long __cdecl value() { return 22L; }\n"
                            "   int payload;\n"
                            "};\n", output);
        }
    },
    {"TU34-023: Same external-linkage struct name, bitfield width differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentBitfieldWidth { unsigned int a : 3; unsigned int b : 5; };"
                                                    ,   "struct DifferentBitfieldWidth { unsigned int a : 4; unsigned int b : 4; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentBitfieldWidth\n"
                            "[tu3.cpp]\n"
                            "struct DifferentBitfieldWidth { // sizeof=4\n"
                            "   unsigned int a : 3;\n"
                            "   unsigned int b : 5;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentBitfieldWidth { // sizeof=4\n"
                            "   unsigned int a : 4;\n"
                            "   unsigned int b : 4;\n"
                            "};\n", output);
        }
    },
    {"TU34-024: Same external-linkage struct name, bitfield signedness differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentBitfieldSignedness {   signed int a : 4; };"
                                                    ,   "struct DifferentBitfieldSignedness { unsigned int a : 4; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentBitfieldSignedness\n"
                            "[tu3.cpp]\n"
                            "struct DifferentBitfieldSignedness { // sizeof=4\n"
                            "   int a : 4;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentBitfieldSignedness { // sizeof=4\n"
                            "   unsigned int a : 4;\n"
                            "};\n", output);
        }
    },
    {"TU34-025: Same external-linkage struct name, anonymous union member type differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentAnonymousUnionMember { int tag; union { int i; float  f; }; };"
                                                    ,   "struct DifferentAnonymousUnionMember { int tag; union { int i; double f; }; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentAnonymousUnionMember\n"
                            "[tu3.cpp]\n"
                            "struct DifferentAnonymousUnionMember { // sizeof=8\n"
                            "   int tag;\n"
                            "   union  { // sizeof=4\n"
                            "      int i;\n"
                            "      float f;\n"
                            "   };\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentAnonymousUnionMember { // sizeof=16\n"
                            "   int tag;\n"
                            "   union  { // sizeof=8\n"
                            "      int i;\n"
                            "      double f;\n"
                            "   };\n"
                            "};\n", output);
        }
    },
    {"TU34-026: Same external-linkage struct name, anonymous struct member order differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentAnonymousStructOrder { int tag; struct { int x; double y; }; };"
                                                    ,   "struct DifferentAnonymousStructOrder { int tag; struct { double y; int x; }; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentAnonymousStructOrder\n"
                            "[tu3.cpp]\n"
                            "struct DifferentAnonymousStructOrder { // sizeof=24\n"
                            "   int tag;\n"
                            "   struct  { // sizeof=16\n"
                            "      int x;\n"
                            "      double y;\n"
                            "   };\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentAnonymousStructOrder { // sizeof=24\n"
                            "   int tag;\n"
                            "   struct  { // sizeof=16\n"
                            "      double y;\n"
                            "      int x;\n"
                            "   };\n"
                            "};\n", output);
        }
    },
    {"TU34-027: Same external-linkage struct name, identical anonymous union. Expected ODR violation: NO.", []
        {
            const auto& [violations, output] = RunTest( "struct IdenticalAnonymousUnion { int tag; union { int i; float f; }; };"
                                                    ,   "struct IdenticalAnonymousUnion { int tag; union { int i; float f; }; };");
            Assert::AreEqual(0, violations, "there should be no ODR violation");
            Assert::AreEqual("", output);
        }
    },
    {"TU34-028: Same external-linkage struct name, typedef target differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentTypedefTarget { typedef int  Alias; Alias value; };"
                                                    ,   "struct DifferentTypedefTarget { typedef long Alias; Alias value; };");
            Assert::AreEqual(2, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentTypedefTarget::Alias\n"
                                            "[tu3.cpp]\n"
                            "using DifferentTypedefTarget::Alias = int; // typedef int DifferentTypedefTarget::Alias;\n"
                            "[tu4.cpp]\n"
                            "using DifferentTypedefTarget::Alias = long; // typedef long DifferentTypedefTarget::Alias;\n"
                                "\n"
                            "ODR VIOLATION: DifferentTypedefTarget\n"
                            "[tu3.cpp]\n"
                            "struct DifferentTypedefTarget { // sizeof=4\n"
                            "   typedef int Alias;\n"
                            "   Alias value;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentTypedefTarget { // sizeof=4\n"
                            "   typedef long Alias;\n"
                            "   Alias value;\n"
                            "};\n", output);
        }
    },
    {"TU34-029: Same external-linkage struct name, using-alias target differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentUsingAliasTarget { using Alias = int ; Alias value; };"
                                                    ,   "struct DifferentUsingAliasTarget { using Alias = long; Alias value; };");
            Assert::AreEqual(2, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentUsingAliasTarget::Alias\n"
                            "[tu3.cpp]\n"
                            "using DifferentUsingAliasTarget::Alias = int; // typedef int DifferentUsingAliasTarget::Alias;\n"
                            "[tu4.cpp]\n"
                            "using DifferentUsingAliasTarget::Alias = long; // typedef long DifferentUsingAliasTarget::Alias;\n"
                            "\n"
                            "ODR VIOLATION: DifferentUsingAliasTarget\n"
                            "[tu3.cpp]\n"
                            "struct DifferentUsingAliasTarget { // sizeof=4\n"
                            "   using Alias = int;\n"
                            "   Alias value;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentUsingAliasTarget { // sizeof=4\n"
                            "   using Alias = long;\n"
                            "   Alias value;\n"
                            "};\n", output);
        }
    },
    {"TU34-030: Same external-linkage class template specialization, identical definition. Expected ODR violation: NO.", []
        {
            const auto& [violations, output] = RunTest( "template<typename T> struct IdenticalTemplate { T value; };"
                                                    ,   "template<typename T> struct IdenticalTemplate { T value; };");
            Assert::AreEqual(0, violations, "there should be no ODR violation");
            Assert::AreEqual("", output);
        }
    },
    {"TU34-031: Same external-linkage class template specialization, different member type. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "template<typename T> struct DifferentTemplateDefinition { T value; int  extra; };"
                                                    ,   "template<typename T> struct DifferentTemplateDefinition { T value; long extra; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentTemplateDefinition<>\n"
                            "[tu3.cpp]\n"
                            "template<typename T> struct DifferentTemplateDefinition {\n"
                            "   T value;\n"
                            "   int extra;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "template<typename T> struct DifferentTemplateDefinition {\n"
                            "   T value;\n"
                            "   long extra;\n"
                            "};\n", output);
        }
    },
    {"TU34-032: Same external-linkage template with non-type argument, definition differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "template<int N> struct DifferentNonTypeTemplateDefinition { int  values[N]; };"
                                                    ,   "template<int N> struct DifferentNonTypeTemplateDefinition { long values[N]; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentNonTypeTemplateDefinition<>\n"
                            "[tu3.cpp]\n"
                            "template<int N> struct DifferentNonTypeTemplateDefinition {\n"
                            "   int values[N];\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "template<int N> struct DifferentNonTypeTemplateDefinition {\n"
                            "   long values[N];\n"
                            "};\n", output);
        }
    },
    {"TU34-033: Same external-linkage inline function name and signature, identical body. Expected ODR violation: NO.", []
        {
            const auto& [violations, output] = RunTest( "inline int IdenticalInlineFunction(int x) { return x + 33; }"
                                                    ,   "inline int IdenticalInlineFunction(int x) { return x + 33; }");
            Assert::AreEqual(0, violations, "there should be 0 ODR violation");
            Assert::AreEqual("", output);
        }
    },
    {"TU34-034: Same external-linkage inline function name and signature, different body. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "inline int DifferentInlineFunctionBody(int x) { return x + 34; }"
                                                    ,   "inline int DifferentInlineFunctionBody(int x) { return x + 340; }");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentInlineFunctionBody(int)\n"
                            "[tu3.cpp]\n"
                            "inline int __cdecl DifferentInlineFunctionBody(int x) { return x + 34; }\n"
                            "[tu4.cpp]\n"
                            "inline int __cdecl DifferentInlineFunctionBody(int x) { return x + 340; }\n", output);
        }
    },
    {"TU34-035: Same internal-linkage static function name, different body. Expected ODR violation: NO.", []
        {
            const auto& [violations, output] = RunTest( "static int SameStaticFunctionNameDifferentBody(int x) { return x + 35; }"
                                                    ,   "static int SameStaticFunctionNameDifferentBody(int x) { return x + 350; }");
            Assert::AreEqual(0, violations, "there should be no ODR violation");
            Assert::AreEqual("", output);
        }
    },
    {"TU34-036: Same anonymous-namespace function name, different body. Expected ODR violation: NO.", []
        {
            const auto& [violations, output] = RunTest("namespace { int SameAnonymousNamespaceFunctionNameDifferentBody(int x) { return x + 36; } }"
                                                    ,  "namespace { int SameAnonymousNamespaceFunctionNameDifferentBody(int x) { return x + 360; } }");
            Assert::AreEqual(0, violations, "there should be no ODR violation");
            Assert::AreEqual("", output);
        }
    },
    {"TU34-037: Same anonymous-namespace type name at namespace scope, different layout. Expected ODR violation: NO.", []
        {
            const auto& [violations, output] = RunTest("namespace { struct SameAnonymousNamespaceTypeNameDifferentLayout { int x; }; }"
                                                    ,  "namespace { struct SameAnonymousNamespaceTypeNameDifferentLayout { double y; }; }");
            Assert::AreEqual(0, violations, "there should be no ODR violation");
            Assert::AreEqual("", output);
        }
    },
    {"TU34-038: Anonymous-namespace type embedded in external-linkage type, different layout. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "namespace { struct InternalPartForExternalCarrier {          int x; }; }"
                                                        "struct ExternalCarrierOfInternalType { const InternalPartForExternalCarrier part; };"
                                                    ,   "namespace { struct InternalPartForExternalCarrier { unsigned int y; }; }"
                                                        "struct ExternalCarrierOfInternalType { const InternalPartForExternalCarrier part; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: ExternalCarrierOfInternalType\n"
                            "[tu3.cpp]\n"
                            "struct ExternalCarrierOfInternalType { // sizeof=4\n"
                            "   const struct (anonymous namespace)::InternalPartForExternalCarrier { // sizeof=4\n"
                            "      int x;\n"
                            "   } part;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct ExternalCarrierOfInternalType { // sizeof=4\n"
                            "   const struct (anonymous namespace)::InternalPartForExternalCarrier { // sizeof=4\n"
                            "      unsigned int y;\n"
                            "   } part;\n"
                            "};\n", output);
        }
    },
    {"TU34-039: Anonymous-namespace type used only by another internal-linkage type, different layout. Expected ODR violation: NO.", []
        {
            const auto& [violations, output] = RunTest( "namespace { struct InternalOnlyPart { int    x; }; struct InternalOnlyCarrier { InternalOnlyPart part; }; }"
                                                    ,   "namespace { struct InternalOnlyPart { double y; }; struct InternalOnlyCarrier { InternalOnlyPart part; }; }");

            Assert::AreEqual(0, violations, "there should be 0 ODR violation");
            Assert::AreEqual("", output);
        }
    },
    {"TU34-040: Same external-linkage function pointer member type, pointee signature differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentFunctionPointerMember {  int (*callback)( int); }; int  CallbackForTU3( int x) { return x + 40;  }"
                                                    ,   "struct DifferentFunctionPointerMember { long (*callback)(long); }; long CallbackForTU4(long x) { return x + 40L; }");

            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentFunctionPointerMember\n"
                            "[tu3.cpp]\n"
                            "struct DifferentFunctionPointerMember { // sizeof=8\n"
                            "   int (*callback)(int);\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentFunctionPointerMember { // sizeof=8\n"
                            "   long (*callback)(long);\n"
                            "};\n", output);
        }
    },
    {"TU34-041: Same external-linkage pointer-to-member type, member type differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct MemberPointerTarget {  int value; }; struct DifferentPointerToMember {  int MemberPointerTarget::* member; };"
                                                    ,   "struct MemberPointerTarget { long value; }; struct DifferentPointerToMember { long MemberPointerTarget::* member; };");

            Assert::AreEqual(2, violations, "there should be 2 ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentPointerToMember\n"
                            "[tu3.cpp]\n"
                            "struct DifferentPointerToMember { // sizeof=4\n"
                            "   int MemberPointerTarget::*member;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentPointerToMember { // sizeof=4\n"
                            "   long MemberPointerTarget::*member;\n"
                            "};\n"
                            "\n"
                            "ODR VIOLATION: MemberPointerTarget\n"
                            "[tu3.cpp]\n"
                            "struct __single_inheritance MemberPointerTarget { // sizeof=4\n"
                            "   int value;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct __single_inheritance MemberPointerTarget { // sizeof=4\n"
                            "   long value;\n"
                            "};\n", output);
        }
    },
    {"TU34-042: Same external-linkage array member type, bound differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentArrayBound { int values[3]; };"
                                                    ,   "struct DifferentArrayBound { int values[4]; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentArrayBound\n"
                            "[tu3.cpp]\n"
                            "struct DifferentArrayBound { // sizeof=12\n"
                            "   int values[3];\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentArrayBound { // sizeof=16\n"
                            "   int values[4];\n"
                            "};\n", output);
        }
    },
    { "TU34-043: Same external-linkage volatile/const qualification on member differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest ("struct DifferentCvQualifiedMember {    const int value; };"
                                                    ,   "struct DifferentCvQualifiedMember { volatile int value; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentCvQualifiedMember\n"
                            "[tu3.cpp]\n"
                            "struct DifferentCvQualifiedMember { // sizeof=4\n"
                            "   const int value;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentCvQualifiedMember { // sizeof=4\n"
                            "   volatile int value;\n"
                            "};\n", output);
        }
    },
    { "TU34-044: Same external-linkage nested class name, nested member differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest ("struct DifferentNestedClass { struct Nested {    int x; }; Nested nested; };"
                                                    ,   "struct DifferentNestedClass { struct Nested { double y; }; Nested nested; };");
            Assert::AreEqual(2, violations, "there should be 2 ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentNestedClass\n"
                            "[tu3.cpp]\n"
                            "struct DifferentNestedClass { // sizeof=4\n"
                            "   struct DifferentNestedClass::Nested { // sizeof=4\n"
                            "      int x;\n"
                            "   };\n"
                            "   Nested nested;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentNestedClass { // sizeof=8\n"
                            "   struct DifferentNestedClass::Nested { // sizeof=8\n"
                            "      double y;\n"
                            "   };\n"
                            "   Nested nested;\n"
                            "};\n"
                            "\n"
                            "ODR VIOLATION: DifferentNestedClass::Nested\n"
                            "[tu3.cpp]\n"
                            "struct DifferentNestedClass::Nested { // sizeof=4\n"
                            "   int x;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentNestedClass::Nested { // sizeof=8\n"
                            "   double y;\n"
                            "};\n", output);
        }
    },
    {"TU34-045: Same external-linkage nested enum name, nested enumerators differ. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest ("struct DifferentNestedEnum { enum Kind { alpha = 1, beta = 2 }; Kind kind; };"
                                                    ,   "struct DifferentNestedEnum { enum Kind { alpha = 1, gamma = 3 }; Kind kind; };");
            Assert::AreEqual(2, violations, "there should be 2 ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentNestedEnum::Kind\n"
                            "[tu3.cpp]\n"
                            "enum DifferentNestedEnum::Kind { alpha=1, beta=2 };\n"
                            "[tu4.cpp]\n"
                            "enum DifferentNestedEnum::Kind { alpha=1, gamma=3 };\n"
                            "\n"
                            "ODR VIOLATION: DifferentNestedEnum\n"
                            "[tu3.cpp]\n"
                            "struct DifferentNestedEnum { // sizeof=4\n"
                            "enum DifferentNestedEnum::Kind { alpha=1, beta=2 };   Kind kind;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentNestedEnum { // sizeof=4\n"
                            "enum DifferentNestedEnum::Kind { alpha=1, gamma=3 };   Kind kind;\n"
                            "};\n", output);
        }
    },
    {"TU34-046: Same external-linkage empty class definition. Expected ODR violation: NO.", []
        {
            const auto& [violations, output] = RunTest ("struct IdenticalEmpty {};"
                                                    ,   "struct IdenticalEmpty {};");
            Assert::AreEqual(0, violations, "there should be 0 ODR violations");
            Assert::AreEqual("", output);
        }
    },
    {"TU34-047: Same external-linkage empty class vs non-empty class. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct EmptyVsNonEmpty {};"
                                                    ,   "struct EmptyVsNonEmpty { int value; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: EmptyVsNonEmpty\n"
                            "[tu3.cpp]\n"
                            "struct EmptyVsNonEmpty { // sizeof=1\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct EmptyVsNonEmpty { // sizeof=4\n"
                            "   int value;\n"
                            "};\n", output);
        }
    },
    {"TU34-048: Same external-linkage class name, default constructor member initializer differs. Expected ODR violation: YES if constructor bodies/debug records are compared.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentConstructorInitializer { int value; DifferentConstructorInitializer() : value(48) {} };"
                                                    ,   "struct DifferentConstructorInitializer { int value; DifferentConstructorInitializer() : value(480) {} };");
            Assert::AreEqual(2, violations, "there should be 2 ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentConstructorInitializer::DifferentConstructorInitializer()\n"
                            "[tu3.cpp]\n"
                            "void __cdecl DifferentConstructorInitializer::DifferentConstructorInitializer() : value(48) {}\n"
                            "[tu4.cpp]\n"
                            "void __cdecl DifferentConstructorInitializer::DifferentConstructorInitializer() : value(480) {}\n"
                            "\n"
                            "ODR VIOLATION: DifferentConstructorInitializer\n"
                            "[tu3.cpp]\n"
                            "struct DifferentConstructorInitializer { // sizeof=4\n"
                            "   int value;\n"
                            "   void __cdecl DifferentConstructorInitializer() : value(48) {}\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentConstructorInitializer { // sizeof=4\n"
                            "   int value;\n"
                            "   void __cdecl DifferentConstructorInitializer() : value(480) {}\n"
                            "};\n", output);
        }
    },
    {"TU34-049: Same external-linkage lambda closure use through an inline function, identical shape. Expected ODR violation: NO.", []
        {
            const auto& [violations, output] = RunTest( "inline int IdenticalLambdaUser(int x) { auto lambda = [](int y) { return y + 49; }; return lambda(x); }"
                                                    ,   "inline int IdenticalLambdaUser(int x) { auto lambda = [](int y) { return y + 49; }; return lambda(x); }");
            Assert::AreEqual(0, violations, "there should be 0 ODR violation(s)");
            Assert::AreEqual("", output);
        }
    },
    {"TU34-050: Same external-linkage lambda closure use through an inline function, different body. Expected ODR violation: YES if function/lambda bodies are compared.", []
        {
            const auto& [violations, output] = RunTest( "inline int DifferentLambdaUser(int x) { auto lambda = [](int y) { return y + 50; }; return lambda(x); }"
                                                    ,   "inline int DifferentLambdaUser(int x) { auto lambda = [](int y) { return y + 500; }; return lambda(x); }");
            Assert::AreEqual(1, violations, "wrong number of ODR violation(s)");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentLambdaUser(int)\n"
                            "[tu3.cpp]\n"
                            "inline int __cdecl DifferentLambdaUser(int x) {\n"
                            "    auto lambda = [](int y) {\n"
                            "        return y + 50;\n"
                            "    };\n"
                            "    return lambda(x);\n"
                            "}\n"
                            "[tu4.cpp]\n"
                            "inline int __cdecl DifferentLambdaUser(int x) {\n"
                            "    auto lambda = [](int y) {\n"
                            "        return y + 500;\n"
                            "    };\n"
                            "    return lambda(x);\n"
                            "}\n", output);
        }
    },
};


Test ComprehensiveTests2[] = // TU1, TU2 tests
{
    {"Same class but different default member initializers", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentDefaultMemberInitializer { int a = 1; };"
                                                    ,   "struct DifferentDefaultMemberInitializer { int a = 2; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation(s)");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentDefaultMemberInitializer\n"
                            "[tu3.cpp]\n"
                            "struct DifferentDefaultMemberInitializer { // sizeof=4\n"
                            "   int a=1;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentDefaultMemberInitializer { // sizeof=4\n"
                            "   int a=2;\n"
                            "};\n", output);
        }
    },
    {"Same class but different constexpr values", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentConstexprValue { static constexpr int v = 1; };"
                                                    ,   "struct DifferentConstexprValue { static constexpr int v = 2; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation(s)");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentConstexprValue\n"
                            "[tu3.cpp]\n"
                            "struct DifferentConstexprValue { // sizeof=1\n"
                            "   constexpr static const int v=1;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentConstexprValue { // sizeof=1\n"
                            "   constexpr static const int v=2;\n"
                            "};\n", output);
        }
    },
    {"Same class but different constexpr / consteval / constinit. These affect linkage and initialization.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentConstInit { static constinit int x; };"
                                                    ,   "struct DifferentConstInit { static           int x; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation(s)");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentConstInit\n"
                            "[tu3.cpp]\n"
                            "struct DifferentConstInit { // sizeof=1\n"
                            "   constinit static int x;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentConstInit { // sizeof=1\n"
                            "   static int x;\n"
                            "};\n", output);
        }
    },
    {"Same constexpr function but different bodies", []
        {
            const auto& [violations, output] = RunTest ("constexpr int SameConstexprFunctionDifferentBody() { return 1; }"
                                                    ,   "constexpr int SameConstexprFunctionDifferentBody() { return 2; }");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation(s)");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameConstexprFunctionDifferentBody()\n"
                            "[tu3.cpp]\n"
                            "constexpr int __cdecl SameConstexprFunctionDifferentBody() { return 1; }\n"
                            "[tu4.cpp]\n"
                            "constexpr int __cdecl SameConstexprFunctionDifferentBody() { return 2; }\n"
                          , output);
        }
    },
    {"Static const vs static constexpr", []
        {
            const auto& [violations, output] = RunTest ("struct DataMemberIsStaticConstOrStaticConstexpr { static const     int a = 1; };"
                                                    ,   "struct DataMemberIsStaticConstOrStaticConstexpr { static constexpr int a = 1; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation(s)");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DataMemberIsStaticConstOrStaticConstexpr\n"
                            "[tu3.cpp]\n"
                            "struct DataMemberIsStaticConstOrStaticConstexpr { // sizeof=1\n"
                            "   static const int a=1;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DataMemberIsStaticConstOrStaticConstexpr { // sizeof=1\n"
                            "   constexpr static const int a=1;\n"
                            "};\n", output);
        }
    },
    {"Same template but different default template arguments", []
        {
            const auto& [violations, output] = RunTest ("template<typename T = char> struct SameTemplateDifferentDefaultTemplateArguments { T value; };"
                                                    ,   "template<typename T = long> struct SameTemplateDifferentDefaultTemplateArguments { T value; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation(s)");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameTemplateDifferentDefaultTemplateArguments<>\n"
                            "[tu3.cpp]\n"
                            "template<typename T=char> struct SameTemplateDifferentDefaultTemplateArguments {\n"
                            "   T value;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "template<typename T=long> struct SameTemplateDifferentDefaultTemplateArguments {\n"
                            "   T value;\n"
                            "};\n", output);
        }
    },
    {"Same template but different default non-type template arguments", []
        {
            const auto& [violations, output] = RunTest ("template<int N=1> struct SameTemplateDifferentDefaultNTTP { int value; };"
                                                       ,"template<int N=2> struct SameTemplateDifferentDefaultNTTP { int value; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation(s)");
            Assert::AreEqual("\n"
                             "ODR VIOLATION: SameTemplateDifferentDefaultNTTP<>\n"
                             "[tu3.cpp]\n"
                             "template<int N=1> struct SameTemplateDifferentDefaultNTTP {\n"
                             "   int value;\n"
                             "};\n"
                             "[tu4.cpp]\n"
                             "template<int N=2> struct SameTemplateDifferentDefaultNTTP {\n"
                             "   int value;\n"
                             "};\n",
                             output);
        }
    },
    {"Same template but different default template-template parameters", []
        {
            const auto& [violations, output] = RunTest ("template<class> struct T1 {};"
                                                        "template<template<class> class TT = T1>"
                                                        "struct SameTemplateDifferentDefaultTTP { TT<int> value; };"
                                                      , "template<class> struct T2 {};"
                                                        "template<template<class> class TT = T2>"
                                                        "struct SameTemplateDifferentDefaultTTP { TT<int> value; };");

            Assert::AreEqual(1, violations, "there should be 1 ODR violation(s)");
            Assert::AreEqual("\n"
                             "ODR VIOLATION: SameTemplateDifferentDefaultTTP<>\n"
                             "[tu3.cpp]\n"
                             "template<template<class> class TT=T1> struct SameTemplateDifferentDefaultTTP {\n"
                             "   TT<int> value;\n"
                             "};\n"
                             "[tu4.cpp]\n"
                             "template<template<class> class TT=T2> struct SameTemplateDifferentDefaultTTP {\n"
                             "   TT<int> value;\n"
                             "};\n", output);
        }
    },
    {"Same class but different inline-ness on method", []
        {
            const auto& [violations, output] = RunTest ("struct SameClassDifferentInlinenessOnFunction {              inline  void InlineOrNot() {} };"
                                                      , "struct SameClassDifferentInlinenessOnFunction { __declspec(noinline) void InlineOrNot() {} };");
            Assert::AreEqual(2, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameClassDifferentInlinenessOnFunction::InlineOrNot()\n"
                            "[tu3.cpp]\n"
                            "inline void __cdecl SameClassDifferentInlinenessOnFunction::InlineOrNot() {}\n"
                            "[tu4.cpp]\n"
                            "__declspec(noinline) void __cdecl SameClassDifferentInlinenessOnFunction::InlineOrNot() {}\n"
                            "\n"
                            "ODR VIOLATION: SameClassDifferentInlinenessOnFunction\n"
                            "[tu3.cpp]\n"
                            "struct SameClassDifferentInlinenessOnFunction { // sizeof=1\n"
                            "   inline void __cdecl InlineOrNot() {}\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct SameClassDifferentInlinenessOnFunction { // sizeof=1\n"
                            "   __declspec(noinline) void __cdecl InlineOrNot() {}\n"
                            "};\n", output);
        }
    },
    {"Same class but with different constexpr-ness on method", []
        {
            const auto& [violations, output] = RunTest ("struct SameClassDifferentConstexpressOnFunction { constexpr void ConstexpreOrNot() {} };"
                                                      , "struct SameClassDifferentConstexpressOnFunction {           void ConstexpreOrNot() {} };");
            Assert::AreEqual(2, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameClassDifferentConstexpressOnFunction::ConstexpreOrNot()\n"
                            "[tu3.cpp]\n"
                            "constexpr void __cdecl SameClassDifferentConstexpressOnFunction::ConstexpreOrNot() {}\n"
                            "[tu4.cpp]\n"
                            "void __cdecl SameClassDifferentConstexpressOnFunction::ConstexpreOrNot() {}\n"
                            "\n"
                            "ODR VIOLATION: SameClassDifferentConstexpressOnFunction\n"
                            "[tu3.cpp]\n"
                            "struct SameClassDifferentConstexpressOnFunction { // sizeof=1\n"
                            "   constexpr void __cdecl ConstexpreOrNot() {}\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct SameClassDifferentConstexpressOnFunction { // sizeof=1\n"
                            "   void __cdecl ConstexpreOrNot() {}\n"
                            "};\n", output);
        }
    },
    {"Same class but different final/override usage. These change the virtual table.", []
        {
            const auto& [violations, output] = RunTest ("struct BaseForOverride { virtual void OverrideOrNot() {} };"
                                                        "struct SameClassDifferentOverrideSpecifier : BaseForOverride { void OverrideOrNot() override {} };"
                                                      , "struct BaseForOverride { virtual void OverrideOrNot() {} };"
                                                        "struct SameClassDifferentOverrideSpecifier : BaseForOverride { void OverrideOrNot() {} };");
            Assert::AreEqual(2, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameClassDifferentOverrideSpecifier::OverrideOrNot()\n"
                            "[tu3.cpp]\n"
                            "void __cdecl SameClassDifferentOverrideSpecifier::OverrideOrNot() override override {}\n"
                            "[tu4.cpp]\n"
                            "void __cdecl SameClassDifferentOverrideSpecifier::OverrideOrNot() {}\n"
                            "\n"
                            "ODR VIOLATION: SameClassDifferentOverrideSpecifier\n"
                            "[tu3.cpp]\n"
                            "struct SameClassDifferentOverrideSpecifier : public BaseForOverride { // sizeof=8\n"
                            "   void __cdecl OverrideOrNot() override override {}\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct SameClassDifferentOverrideSpecifier : public BaseForOverride { // sizeof=8\n"
                            "   void __cdecl OverrideOrNot() {}\n"
                            "};\n", output);
        }
    },
    {"Same class but different noexcept on member functions", []
        {
            const auto& [violations, output] = RunTest ("struct SameClassDifferentNoExceptOnMethod { void NoExceptMethod() noexcept {} };"
                                                      , "struct SameClassDifferentNoExceptOnMethod { void NoExceptMethod()          {} };");
            Assert::AreEqual(2, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameClassDifferentNoExceptOnMethod::NoExceptMethod()\n"
                            "[tu3.cpp]\n"
                            "void __cdecl SameClassDifferentNoExceptOnMethod::NoExceptMethod() noexcept {}\n"
                            "[tu4.cpp]\n"
                            "void __cdecl SameClassDifferentNoExceptOnMethod::NoExceptMethod() {}\n"
                            "\n"
                            "ODR VIOLATION: SameClassDifferentNoExceptOnMethod\n"
                            "[tu3.cpp]\n"
                            "struct SameClassDifferentNoExceptOnMethod { // sizeof=1\n"
                            "   void __cdecl NoExceptMethod() noexcept {}\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct SameClassDifferentNoExceptOnMethod { // sizeof=1\n"
                            "   void __cdecl NoExceptMethod() {}\n"
                            "};\n", output);
        }
    },
    {"Same class name but differently sized data-member", []
        {
            const auto& [violations, output] = RunTest ("struct DifferentSizedMember { char a; };"
                                                      , "struct DifferentSizedMember { wchar_t* a; };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentSizedMember\n"
                            "[tu3.cpp]\n"
                            "struct DifferentSizedMember { // sizeof=1\n"
                            "   char a;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentSizedMember { // sizeof=8\n"
                            "   wchar_t *a;\n"
                            "};\n", output);
        }
    },
    {"Same class name but extra data-member", []
        {
            const auto& [violations, output] = RunTest ("struct DifferentDataMembers { int a; };"
                                                      , "struct DifferentDataMembers { int a; int b; };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentDataMembers\n"
                            "[tu3.cpp]\n"
                            "struct DifferentDataMembers { // sizeof=4\n"
                            "   int a;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentDataMembers { // sizeof=8\n"
                            "   int a;\n"
                            "   int b;\n"
                            "};\n", output);
        }
    },
    {"Same class name but data-members in different order", []
        {
            const auto& [violations, output] = RunTest ("struct DifferentOrderOfDataMembers { int a, b; };"
                                                      , "struct DifferentOrderOfDataMembers { int b, a; };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentOrderOfDataMembers\n"
                            "[tu3.cpp]\n"
                            "struct DifferentOrderOfDataMembers { // sizeof=8\n"
                            "   int a;\n"
                            "   int b;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentOrderOfDataMembers { // sizeof=8\n"
                            "   int b;\n"
                            "   int a;\n"
                            "};\n", output);
        }
    },
    {"Same class name but different type of data-members", []
        {
            const auto& [violations, output] = RunTest ("struct DifferentTypeOfDataMembers {   signed a; };"
                                                      , "struct DifferentTypeOfDataMembers { unsigned a; };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentTypeOfDataMembers\n"
                            "[tu3.cpp]\n"
                            "struct DifferentTypeOfDataMembers { // sizeof=4\n"
                            "   int a;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentTypeOfDataMembers { // sizeof=4\n"
                            "   unsigned int a;\n"
                            "};\n", output);
        }
    },
    {"Same class name but data-members are pointers to different types", []
        {
            const auto& [violations, output] = RunTest ("struct StructContainingPointerToDifferentTypes { char * ptr; };"
                                                      , "struct StructContainingPointerToDifferentTypes { int  * ptr; };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: StructContainingPointerToDifferentTypes\n"
                            "[tu3.cpp]\n"
                            "struct StructContainingPointerToDifferentTypes { // sizeof=8\n"
                            "   char *ptr;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct StructContainingPointerToDifferentTypes { // sizeof=8\n"
                            "   int *ptr;\n"
                            "};\n", output);
        }
    },
    {"Same class but different data-member access specifiers", []
        {
            const auto& [violations, output] = RunTest ("struct SameClassDifferentDataMemberAccessSpecifier { public:  int a; };"
                                                      , "struct SameClassDifferentDataMemberAccessSpecifier { private: int a; };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameClassDifferentDataMemberAccessSpecifier\n"
                            "[tu3.cpp]\n"
                            "struct SameClassDifferentDataMemberAccessSpecifier { // sizeof=4\n"
                            "public:\n"
                            "   int a;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct SameClassDifferentDataMemberAccessSpecifier { // sizeof=4\n"
                            "private:\n"
                            "   int a;\n"
                            "};\n", output);
        }
    },
    {"Same class name but different base", []
        {
            const auto& [violations, output] = RunTest ("struct Base1 {}; struct DifferentBases : Base1 {};"
                                                      , "struct Base2 {}; struct DifferentBases : Base2 {};");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentBases\n"
                            "[tu3.cpp]\n"
                            "struct DifferentBases : public Base1 { // sizeof=1\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentBases : public Base2 { // sizeof=1\n"
                            "};\n", output);
        }
    },
    {"Same class name but base classes in different order", []
        {
            const auto& [violations, output] = RunTest ("struct Base1{}; struct Base2{}; struct BaseClassesInDifferentOrder : Base1, Base2 {};"
                                                      , "struct Base1{}; struct Base2{}; struct BaseClassesInDifferentOrder : Base2, Base1 {};");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: BaseClassesInDifferentOrder\n"
                            "[tu3.cpp]\n"
                            "struct BaseClassesInDifferentOrder : public Base1, public Base2 { // sizeof=1\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct BaseClassesInDifferentOrder : public Base2, public Base1 { // sizeof=1\n"
                            "};\n", output);
        }
    },
    {"Same class name but base is either virtual or not", []
        {
            const auto& [violations, output] = RunTest ("struct Base1{}; struct BaseClassVirtualOrNot : virtual Base1 {};"
                                                      , "struct Base1{}; struct BaseClassVirtualOrNot :         Base1 {};");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: BaseClassVirtualOrNot\n"
                            "[tu3.cpp]\n"
                            "struct BaseClassVirtualOrNot : public virtual Base1 { // sizeof=8\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct BaseClassVirtualOrNot : public Base1 { // sizeof=1\n"
                            "};\n", output);
        }
    },
    {"Same class name but different Access specifiers on base class", []
        {
            const auto& [violations, output] = RunTest ("struct Base1 {}; struct DifferentAccessSpecifiersOnBaseClass : public  Base1 {};"
                                                      , "struct Base1 {}; struct DifferentAccessSpecifiersOnBaseClass : private Base1 {};");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentAccessSpecifiersOnBaseClass\n"
                            "[tu3.cpp]\n"
                            "struct DifferentAccessSpecifiersOnBaseClass : public Base1 { // sizeof=1\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentAccessSpecifiersOnBaseClass : private Base1 { // sizeof=1\n"
                            "};\n", output);
        }
    },
    {"Same name but different access specifiers on method", []
        {
            const auto& [violations, output] = RunTest ("struct DifferentAccessSpecifiersOnMethod { public:  void Foo() {} };"
                                                      , "struct DifferentAccessSpecifiersOnMethod { private: void Foo() {} };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentAccessSpecifiersOnMethod\n"
                            "[tu3.cpp]\n"
                            "struct DifferentAccessSpecifiersOnMethod { // sizeof=1\n"
                            "public:\n"
                            "   void __cdecl Foo() {}\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentAccessSpecifiersOnMethod { // sizeof=1\n"
                            "private:\n"
                            "   void __cdecl Foo() {}\n"
                            "};\n", output);
        }
    },
    {"Same class but different member types", []
        {
            const auto& [violations, output] = RunTest ("struct DifferentDataMemberType { char a; };"
                                                      , "struct DifferentDataMemberType { long a; };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentDataMemberType\n"
                            "[tu3.cpp]\n"
                            "struct DifferentDataMemberType { // sizeof=1\n"
                            "   char a;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentDataMemberType { // sizeof=4\n"
                            "   long a;\n"
                            "};\n", output);
        }
    },
    {"Same class but different member order", []
        {
            const auto& [violations, output] = RunTest ("struct SameMemberTypesDifferentOrder { int a; char b; };"
                                                      , "struct SameMemberTypesDifferentOrder { char b; int a; };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameMemberTypesDifferentOrder\n"
                            "[tu3.cpp]\n"
                            "struct SameMemberTypesDifferentOrder { // sizeof=8\n"
                            "   int a;\n"
                            "   char b;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct SameMemberTypesDifferentOrder { // sizeof=8\n"
                            "   char b;\n"
                            "   int a;\n"
                            "};\n", output);
        }
    },
    {"Same class but different base classes", []
        {
            const auto& [violations, output] = RunTest ("struct Base1{}; struct DifferentBaseClass : Base1 {};"
                                                      , "                struct DifferentBaseClass         {};");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentBaseClass\n"
                            "[tu3.cpp]\n"
                            "struct DifferentBaseClass : public Base1 { // sizeof=1\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentBaseClass { // sizeof=1\n"
                            "};\n", output);
        }
    },
    {"Same class but different const-ness on data-member", []
        {
            const auto& [violations, output] = RunTest ("struct DifferentConstDataMember { const int a{}; };"
                                                      , "struct DifferentConstDataMember {       int a{}; };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentConstDataMember\n"
                            "[tu3.cpp]\n"
                            "struct DifferentConstDataMember { // sizeof=4\n"
                            "   const int a{};\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentConstDataMember { // sizeof=4\n"
                            "   int a{};\n"
                            "};\n", output);
        }
    },
    {"Same class name but different Volatility on data-member", []
        {
            const auto& [violations, output] = RunTest ("struct DifferentVolatileDataMember { volatile int a{}; }; "
                                                      , "struct DifferentVolatileDataMember {          int a{}; }; ");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentVolatileDataMember\n"
                            "[tu3.cpp]\n"
                            "struct DifferentVolatileDataMember { // sizeof=4\n"
                            "   volatile int a{};\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentVolatileDataMember { // sizeof=4\n"
                            "   int a{};\n"
                            "};\n", output);
        }
    },
    {"Same name but different kind: class vs. struct", []
        {
            const auto& [violations, output] = RunTest ("struct StructVsClass {};"
                                                      , "class  StructVsClass {};");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: StructVsClass\n"
                            "[tu3.cpp]\n"
                            "struct StructVsClass { // sizeof=1\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "class StructVsClass { // sizeof=1\n"
                            "};\n", output);
        }
    },
    {"Same inline function but different bodies. Inline functions must be bit‑for‑bit identical across TUs.", []
        {
            const auto& [violations, output] = RunTest ("inline int FunctionsMustBeBitwiseIdentical() { return 1; }"
                                                      , "inline int FunctionsMustBeBitwiseIdentical() { return 2; }");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: FunctionsMustBeBitwiseIdentical()\n"
                            "[tu3.cpp]\n"
                            "inline int __cdecl FunctionsMustBeBitwiseIdentical() { return 1; }\n"
                            "[tu4.cpp]\n"
                            "inline int __cdecl FunctionsMustBeBitwiseIdentical() { return 2; }\n"
                          , output);
        }
    },
    {"Same template specialization but different definitions", []
        {
            const auto& [violations, output] = RunTest ("template<typename T> inline T   SameFunctionTemplateSpecializationDifferentDefinitions();"
                                                        "template<          > inline int SameFunctionTemplateSpecializationDifferentDefinitions<int>() { return 1; }"
                                                      , "template<typename T> inline T   SameFunctionTemplateSpecializationDifferentDefinitions();"
                                                        "template<          > inline int SameFunctionTemplateSpecializationDifferentDefinitions<int>() { return 2; }");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameFunctionTemplateSpecializationDifferentDefinitions<int>()\n"
                            "[tu3.cpp]\n"
                            "inline int __cdecl SameFunctionTemplateSpecializationDifferentDefinitions<int>() { return 1; }\n"
                            "[tu4.cpp]\n"
                            "inline int __cdecl SameFunctionTemplateSpecializationDifferentDefinitions<int>() { return 2; }\n"
                          , output);
        }
    },
    {"Same enum name but different values on enumerators", []
        {
            const auto& [violations, output] = RunTest ("enum SameEnumButDifferentValues { A = 1, B = 2 };"
                                                      , "enum SameEnumButDifferentValues { A = 1, B = 3 };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameEnumButDifferentValues\n"
                            "[tu3.cpp]\n"
                            "enum SameEnumButDifferentValues { A=1, B=2 };\n"
                            "[tu4.cpp]\n"
                            "enum SameEnumButDifferentValues { A=1, B=3 };\n"
                          , output);
        }
    },
    {"Same class name but different alignment", []
        {
            const auto& [violations, output] = RunTest ("#pragma warning(push)\n"
                                                        "#pragma warning(disable: 4324)\n"
                                                        "struct alignas(4) SameClassDifferentAlignment { int a; };\n"
                                                        "#pragma warning(pop)\n"
                                                      , "#pragma warning(push)\n"
                                                        "#pragma warning(disable: 4324)\n"
                                                        "struct alignas(8) SameClassDifferentAlignment { int a; };\n"
                                                        "#pragma warning(pop)\n");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameClassDifferentAlignment\n"
                            "[tu3.cpp]\n"
                            "struct alignas(4) SameClassDifferentAlignment { // sizeof=4\n"
                            "   int a;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct alignas(8) SameClassDifferentAlignment { // sizeof=8\n"
                            "   int a;\n"
                            "};\n", output);
        }
    },
    {"Same class but different virtual function table shape", []
        {
            const auto& [violations, output] = RunTest ("struct SameClassDifferentVirtualFunctionTableShape { virtual void f() {} };"
                                                      , "struct SameClassDifferentVirtualFunctionTableShape {};");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameClassDifferentVirtualFunctionTableShape\n"
                            "[tu3.cpp]\n"
                            "struct SameClassDifferentVirtualFunctionTableShape { // sizeof=8\n"
                            "   virtual void __cdecl f() {}\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct SameClassDifferentVirtualFunctionTableShape { // sizeof=1\n"
                            "};\n", output);
        }
    },
    {"Same class name but different virtual function names", []
        {
            const auto& [violations, output] = RunTest ("struct SameClassDifferentVirtualFunctionNames { virtual void Foo() {} };"
                                                      , "struct SameClassDifferentVirtualFunctionNames { virtual void Bar() {} };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameClassDifferentVirtualFunctionNames\n"
                            "[tu3.cpp]\n"
                            "struct SameClassDifferentVirtualFunctionNames { // sizeof=8\n"
                            "   virtual void __cdecl Foo() {}\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct SameClassDifferentVirtualFunctionNames { // sizeof=8\n"
                            "   virtual void __cdecl Bar() {}\n"
                            "};\n", output);
        }
    },
    {"Same class name but different virtualness on method", []
        {
            const auto& [violations, output] = RunTest ("struct SameClassDifferentVirtualnessOnFunction { virtual void VirtualOrNot() {} };"
                                                      , "struct SameClassDifferentVirtualnessOnFunction {         void VirtualOrNot() {} };");
            Assert::AreEqual(2, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameClassDifferentVirtualnessOnFunction::VirtualOrNot()\n"
                            "[tu3.cpp]\n"
                            "virtual void __cdecl SameClassDifferentVirtualnessOnFunction::VirtualOrNot() {}\n"
                            "[tu4.cpp]\n"
                            "void __cdecl SameClassDifferentVirtualnessOnFunction::VirtualOrNot() {}\n"
                            "\n"
                            "ODR VIOLATION: SameClassDifferentVirtualnessOnFunction\n"
                            "[tu3.cpp]\n"
                            "struct SameClassDifferentVirtualnessOnFunction { // sizeof=8\n"
                            "   virtual void __cdecl VirtualOrNot() {}\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct SameClassDifferentVirtualnessOnFunction { // sizeof=1\n"
                            "   void __cdecl VirtualOrNot() {}\n"
                            "};\n", output);
        }
    },
    {"Same class name but one has static method, the other does not", []
        {
            const auto& [violations, output] = RunTest ("struct StaticFunctionOrMethod { static void Foo() {} };"
                                                      , "struct StaticFunctionOrMethod {        void Foo() {} };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: StaticFunctionOrMethod\n"
                            "[tu3.cpp]\n"
                            "struct StaticFunctionOrMethod { // sizeof=1\n"
                            "   static void __cdecl Foo() {}\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct StaticFunctionOrMethod { // sizeof=1\n"
                            "   void __cdecl Foo() {}\n"
                            "};\n", output);
        }
    },
    {"Same class but one member is static vs non‑static", []
        {
            const auto& [violations, output] = RunTest ("struct SameClassDifferentStaticOnDataMember { static int a; };"
                                                      , "struct SameClassDifferentStaticOnDataMember {        int a; };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameClassDifferentStaticOnDataMember\n"
                            "[tu3.cpp]\n"
                            "struct SameClassDifferentStaticOnDataMember { // sizeof=1\n"
                            "   static int a;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct SameClassDifferentStaticOnDataMember { // sizeof=4\n"
                            "   int a;\n"
                            "};\n", output);
        }
    },
    {"Same class name but one member is static const, the other only const", []
        {
            const auto& [violations, output] = RunTest ("struct SameClassDifferentStaticConstOnDataMember { static const int a; };"
                                                      , "struct SameClassDifferentStaticConstOnDataMember { static       int a; };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameClassDifferentStaticConstOnDataMember\n"
                            "[tu3.cpp]\n"
                            "struct SameClassDifferentStaticConstOnDataMember { // sizeof=1\n"
                            "   static const int a;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct SameClassDifferentStaticConstOnDataMember { // sizeof=1\n"
                            "   static int a;\n"
                            "};\n", output);
        }
    },
    {"Same class name but one member is static volatile, the other only static", []
        {
            const auto& [violations, output] = RunTest ("struct SameClassDifferentStaticVolatileOnDataMember { static volatile int a; };"
                                                      , "struct SameClassDifferentStaticVolatileOnDataMember { static          int a; };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameClassDifferentStaticVolatileOnDataMember\n"
                            "[tu3.cpp]\n"
                            "struct SameClassDifferentStaticVolatileOnDataMember { // sizeof=1\n"
                            "   static volatile int a;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct SameClassDifferentStaticVolatileOnDataMember { // sizeof=1\n"
                            "   static int a;\n"
                            "};\n", output);
        }
    },
    {"Same class but different bitfield layout", []
        {
            const auto& [violations, output] = RunTest ("struct SameClassDifferentBitfieldLayout { unsigned a : 3; unsigned b : 5; };"
                                                      , "struct SameClassDifferentBitfieldLayout { unsigned a : 4; unsigned b : 4; };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameClassDifferentBitfieldLayout\n"
                            "[tu3.cpp]\n"
                            "struct SameClassDifferentBitfieldLayout { // sizeof=4\n"
                            "   unsigned int a : 3;\n"
                            "   unsigned int b : 5;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct SameClassDifferentBitfieldLayout { // sizeof=4\n"
                            "   unsigned int a : 4;\n"
                            "   unsigned int b : 4;\n"
                            "};\n", output);
        }
    },
    {"Same class but different member order inside anonymous struct/union", []
        {
            const auto& [violations, output] = RunTest ("#pragma warning(push)\n"
                                                        "#pragma warning(disable: 4201)\n"
                                                        "struct SameClassButDifferentMemberOrderInsideAnonmousStructAndUnion { union { struct { int a; int b; }; int x; }; };\n"
                                                        "#pragma warning(pop)\n"
                                                      , "#pragma warning(push)\n"
                                                        "#pragma warning(disable: 4201)\n"
                                                        "struct SameClassButDifferentMemberOrderInsideAnonmousStructAndUnion { union { struct { int b; int a; }; int x; }; };\n"
                                                        "#pragma warning(pop)\n");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameClassButDifferentMemberOrderInsideAnonmousStructAndUnion\n"
                            "[tu3.cpp]\n"
                            "struct SameClassButDifferentMemberOrderInsideAnonmousStructAndUnion { // sizeof=8\n"
                            "   union  { // sizeof=8\n"
                            "      struct  { // sizeof=8\n"
                            "         int a;\n"
                            "         int b;\n"
                            "      };\n"
                            "      int x;\n"
                            "   };\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct SameClassButDifferentMemberOrderInsideAnonmousStructAndUnion { // sizeof=8\n"
                            "   union  { // sizeof=8\n"
                            "      struct  { // sizeof=8\n"
                            "         int b;\n"
                            "         int a;\n"
                            "      };\n"
                            "      int x;\n"
                            "   };\n"
                            "};\n", output);
        }
    },
    {"Same class but different presence/absence of anonymous members", []
        {
            const auto& [violations, output] = RunTest ("#pragma warning(push)\n"
                                                        "#pragma warning(disable: 4201)\n"
                                                        "struct SameClassDifferentPresenceOfAnonymousMembers { union { int a; struct { int b; }; }; };\n"
                                                        "#pragma warning(pop)\n"
                                                      , "#pragma warning(push)\n"
                                                        "#pragma warning(disable: 4201)\n"
                                                        "struct SameClassDifferentPresenceOfAnonymousMembers { union { int a;                    }; };\n"
                                                        "#pragma warning(pop)\n");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameClassDifferentPresenceOfAnonymousMembers\n"
                            "[tu3.cpp]\n"
                            "struct SameClassDifferentPresenceOfAnonymousMembers { // sizeof=4\n"
                            "   union  { // sizeof=4\n"
                            "      int a;\n"
                            "      struct  { // sizeof=4\n"
                            "         int b;\n"
                            "      };\n"
                            "   };\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct SameClassDifferentPresenceOfAnonymousMembers { // sizeof=4\n"
                            "   union  { // sizeof=4\n"
                            "      int a;\n"
                            "   };\n"
                            "};\n", output);
        }
    },
    {"Same typedef or using but different underlying type", []
        {
            const auto& [violations, output] = RunTest ("struct SomeStructForTypedefTesting1 { int x1; }; struct EnclosingTypedefDefinition { typedef SomeStructForTypedefTesting1 SameTypedefDifferentUnderlyingType; };"
                                                      , "struct SomeStructForTypedefTesting2 { int x2; }; struct EnclosingTypedefDefinition { typedef SomeStructForTypedefTesting2 SameTypedefDifferentUnderlyingType; };");
            Assert::AreEqual(2, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: EnclosingTypedefDefinition::SameTypedefDifferentUnderlyingType\n"
                            "[tu3.cpp]\n"
                            "using EnclosingTypedefDefinition::SameTypedefDifferentUnderlyingType = SomeStructForTypedefTesting1; // typedef SomeStructForTypedefTesting1 EnclosingTypedefDefinition::SameTypedefDifferentUnderlyingType;\n"
                            "[tu4.cpp]\n"
                            "using EnclosingTypedefDefinition::SameTypedefDifferentUnderlyingType = SomeStructForTypedefTesting2; // typedef SomeStructForTypedefTesting2 EnclosingTypedefDefinition::SameTypedefDifferentUnderlyingType;\n"
                            "\n"
                            "ODR VIOLATION: EnclosingTypedefDefinition\n"
                            "[tu3.cpp]\n"
                            "struct EnclosingTypedefDefinition { // sizeof=1\n"
                            "   typedef SomeStructForTypedefTesting1 SameTypedefDifferentUnderlyingType;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct EnclosingTypedefDefinition { // sizeof=1\n"
                            "   typedef SomeStructForTypedefTesting2 SameTypedefDifferentUnderlyingType;\n"
                            "};\n", output);
        }
    },
    {"templated enum inside another template works", []
        {
            const auto& [violations, output] = RunTest ("template<typename Outer> struct NamelessEnum { template<typename T> struct Inner { enum { value = sizeof(T) == sizeof(char) ? 1 : 0 }; }; };"
                                                        "NamelessEnum<int>::Inner<char> g_1_instance;"
                                                      , "template<typename Outer> struct NamelessEnum { template<typename T> struct Inner { enum { value = sizeof(T) == sizeof(char) ? 1 : 0 }; }; };"
                                                        "NamelessEnum<int>::Inner<long> g_2_instance;");
            Assert::AreEqual(0, violations, "wrong number of ODR violations");
            Assert::AreEqual("", output);
        }
    },
    {"Same function name with same parameter struct but it's actually different", []
        {
            const auto& [violations, output] = RunTest ("struct DifferentDataMembers { int a;        }; inline void FunctionUsing_DifferentDataMembers(DifferentDataMembers) {}"
                                                      , "struct DifferentDataMembers { int a; int b; }; inline void FunctionUsing_DifferentDataMembers(DifferentDataMembers) {}");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentDataMembers\n"
                            "[tu3.cpp]\n"
                            "struct DifferentDataMembers { // sizeof=4\n"
                            "   int a;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct DifferentDataMembers { // sizeof=8\n"
                            "   int a;\n"
                            "   int b;\n"
                            "};\n", output);
        }
    },
    {"Same function name with same parameter enum but it's actually different", []
        {
            const auto& [violations, output] = RunTest ("enum SameEnumNameDifferentEnumerators { sendeA         }; inline void FunctionUsing_SameEnumNameDifferentEnumerators(SameEnumNameDifferentEnumerators) {}"
                                                      , "enum SameEnumNameDifferentEnumerators { sendeA, sendeB }; inline void FunctionUsing_SameEnumNameDifferentEnumerators(SameEnumNameDifferentEnumerators) {}");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameEnumNameDifferentEnumerators\n"
                            "[tu3.cpp]\n"
                            "enum SameEnumNameDifferentEnumerators { sendeA=0 };\n"
                            "[tu4.cpp]\n"
                            "enum SameEnumNameDifferentEnumerators { sendeA=0, sendeB=1 };\n"
                           , output);
        }
    },
    {"Same function name with same parameter strucct template but it's actually different", []
        {
            const auto& [violations, output] = RunTest ("template<class T> struct SameTemplateDifferentDefinition { int a;         }; inline void FunctionUsing_SameTemplateDifferentDefinition(SameTemplateDifferentDefinition<int>) {}"
                                                      , "template<class T> struct SameTemplateDifferentDefinition { int a;  int b; }; inline void FunctionUsing_SameTemplateDifferentDefinition(SameTemplateDifferentDefinition<int>) {}");
            Assert::AreEqual(2, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameTemplateDifferentDefinition<>\n"
                            "[tu3.cpp]\n"
                            "template<class T> struct SameTemplateDifferentDefinition {\n"
                            "   int a;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "template<class T> struct SameTemplateDifferentDefinition {\n"
                            "   int a;\n"
                            "   int b;\n"
                            "};\n"
                            "\n"
                            "ODR VIOLATION: SameTemplateDifferentDefinition<int>\n"
                            "[tu3.cpp]\n"
                            "template<> struct SameTemplateDifferentDefinition<int> { // sizeof=4\n"
                            "   int a;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "template<> struct SameTemplateDifferentDefinition<int> { // sizeof=8\n"
                            "   int a;\n"
                            "   int b;\n"
                            "};\n", output);
        }
    },
    {"Inline function using inline with different bodies", []
        {
            const auto& [violations, output] = RunTest ("inline int InlinedBody() { return 1; } inline void FunctionUsing_InlinedBody() { InlinedBody(); }"
                                                      , "inline int InlinedBody() { return 2; } inline void FunctionUsing_InlinedBody() { InlinedBody(); }");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: InlinedBody()\n"
                            "[tu3.cpp]\n"
                            "inline int __cdecl InlinedBody() { return 1; }\n"
                            "[tu4.cpp]\n"
                            "inline int __cdecl InlinedBody() { return 2; }\n"
                          , output);
        }
    },
    {"Same function name with same parameter union but it's actually different",[]
        {
            const auto& [violations, output] = RunTest ("union SameUnionNameDifferentElements { int i; float f;           }; inline void FunctionUsing_SameUnionNameDifferentElements(SameUnionNameDifferentElements) {}"
                                                      , "union SameUnionNameDifferentElements { int i; float f; double d; }; inline void FunctionUsing_SameUnionNameDifferentElements(SameUnionNameDifferentElements) {}");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SameUnionNameDifferentElements\n"
                            "[tu3.cpp]\n"
                            "union SameUnionNameDifferentElements { // sizeof=4\n"
                            "   int i;\n"
                            "   float f;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "union SameUnionNameDifferentElements { // sizeof=8\n"
                            "   int i;\n"
                            "   float f;\n"
                            "   double d;\n"
                            "};\n", output);
        }
    },
    {"Same struct method name but bodies differ", []
        {
            const auto& [violations, output] = RunTest ("struct StaticMethodsBodiesDiffer { static int Foo() { return 1; } };"
                                                      , "struct StaticMethodsBodiesDiffer { static int Foo() { return 2; } };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: StaticMethodsBodiesDiffer\n"
                            "[tu3.cpp]\n"
                            "struct StaticMethodsBodiesDiffer { // sizeof=1\n"
                            "   static int __cdecl Foo() { return 1; }\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct StaticMethodsBodiesDiffer { // sizeof=1\n"
                            "   static int __cdecl Foo() { return 2; }\n"
                            "};\n", output);
        }
    },
    {"a friend function vs. non-friend method", []
        {
            const auto& [violations, output] = RunTest ("struct ClassForFriend { friend int TheFriendFunction(ClassForFriend) { return 2; } };"
                                                      , "struct ClassForFriend {        int TheFriendFunction(ClassForFriend) { return 2; } };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: ClassForFriend\n"
                            "[tu3.cpp]\n"
                            "struct ClassForFriend { // sizeof=1\n"
                            "   friend int __cdecl TheFriendFunction(ClassForFriend) { return 2; }\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct ClassForFriend { // sizeof=1\n"
                            "   int __cdecl TheFriendFunction(ClassForFriend) { return 2; }\n"
                            "};\n", output);
        }
    },
    {"a friend function template vs. non-friend method template", []
        {
            const auto& [violations, output] = RunTest ("struct ClassForFriend { template<typename T> friend int TheFriendFunction(ClassForFriend, T) { return 2; } };"
                                                      , "struct ClassForFriend { template<typename T>        int TheFriendFunction(ClassForFriend, T) { return 2; } };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: ClassForFriend\n"
                            "[tu3.cpp]\n"
                            "struct ClassForFriend { // sizeof=1\n"
                            "   template <typename T> friend int __cdecl TheFriendFunction(ClassForFriend, T) { return 2; }\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct ClassForFriend { // sizeof=1\n"
                            "   template <typename T> int __cdecl TheFriendFunction(ClassForFriend, T) { return 2; }\n"
                            "};\n", output);
        }
    },
    {"a friend class template vs. non-friend class template", []
        {
            const auto& [violations, output] = RunTest ("struct ClassForFriend { template<typename T> friend class TheFriendClass; };"
                                                      , "struct ClassForFriend { template<typename T>        class TheFriendClass; };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: ClassForFriend\n"
                            "[tu3.cpp]\n"
                            "struct ClassForFriend { // sizeof=1\n"
                            "   template<typename T> friend class TheFriendClass;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct ClassForFriend { // sizeof=1\n"
                            "   template<typename T> class TheFriendClass;\n"
                            "};\n", output);
        }
    },
    {"a nested friend class declaration vs. non-friend class declarations", []
        {
            const auto& [violations, output] = RunTest ("struct ClassForFriend { friend class TheFriendClass; };"
                                                      , "struct ClassForFriend {        class TheFriendClass{}; };");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: ClassForFriend\n"
                            "[tu3.cpp]\n"
                            "struct ClassForFriend { // sizeof=1\n"
                            "   friend class TheFriendClass;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct ClassForFriend { // sizeof=1\n"
                            "   class ClassForFriend::TheFriendClass { // sizeof=1\n"
                            "   };\n"
                            "};\n", output);
        }
    },
    {"test to make sure relocs have been applied to bodies", []
        {
            const auto& [violations, output] = RunTest ("inline int g_aGlobal1 = 1; struct MakeSureRelocs { int HaveBeenAppliedToBodies() { return g_aGlobal1; } };"
                                                      , "inline int g_aGlobal2 = 2; struct MakeSureRelocs { int HaveBeenAppliedToBodies() { return g_aGlobal2; } };");
            Assert::AreEqual(2, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: MakeSureRelocs::HaveBeenAppliedToBodies()\n"
                            "[tu3.cpp]\n"
                            "int __cdecl MakeSureRelocs::HaveBeenAppliedToBodies() { return g_aGlobal1; }\n"
                            "[tu4.cpp]\n"
                            "int __cdecl MakeSureRelocs::HaveBeenAppliedToBodies() { return g_aGlobal2; }\n"
                            "\n"
                            "ODR VIOLATION: MakeSureRelocs\n"
                            "[tu3.cpp]\n"
                            "struct MakeSureRelocs { // sizeof=1\n"
                            "   int __cdecl HaveBeenAppliedToBodies() { return g_aGlobal1; }\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct MakeSureRelocs { // sizeof=1\n"
                            "   int __cdecl HaveBeenAppliedToBodies() { return g_aGlobal2; }\n"
                            "};\n", output);
        }
    },
    {"test overloads in different TUs", []
        {
            const auto& [violations, output] = RunTest ("int AnOverloadInTU1(void) { return 0; } int AnOverloadInTU1(char a) { return sizeof(a); } "
                                                      , "int AnOverloadInTU2(void) { return 0; } int AnOverloadInTU2(char a) { return sizeof(a); }");
            Assert::AreEqual(0, violations, "wrong number of ODR violations");
            Assert::AreEqual("", output);
        }
    },
    {"A lambda closure type has no linkage and no cross-TU identity.", []
        {
            const auto& [violations, output] = RunTest ("size_t SizeOfInt() { return [](){ return sizeof(int); }(); }"
                                                      , "size_t SizeOfInt() { return [](){ return sizeof(int); }(); }");
            Assert::AreEqual(0, violations, "wrong number of ODR violations");
            Assert::AreEqual("", output);
        }
    },
    {"A lambda closure type has no linkage and no cross-TU identity but the enclosing inline function can have an ODR violation", []
        {
            const auto& [violations, output] = RunTest ("size_t SizeOfInt() { return [](){ return sizeof(char); }(); }"
                                                      , "size_t SizeOfInt() { return [](){ return sizeof(long); }(); }");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: SizeOfInt()\n"
                            "[tu3.cpp]\n"
                            "unsigned long long __cdecl SizeOfInt() {\n"
                            "    return []() {\n"
                            "        return sizeof(char);\n"
                            "    }();\n"
                            "}\n"
                            "[tu4.cpp]\n"
                            "unsigned long long __cdecl SizeOfInt() {\n"
                            "    return []() {\n"
                            "        return sizeof(long);\n"
                            "    }();\n"
                            "}\n", output);
        }
    },
    {"A lambda initializing an inline global int variable", []
        {
            const auto& [violations, output] = RunTest ("inline constexpr auto x = [] { return 1; }();"
                                                      , "inline constexpr auto x = [] { return 2; }();");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: x\n"
                            "[tu3.cpp]\n"
                            "inline constexpr int x = [] { return 1; }();\n"
                            "[tu4.cpp]\n"
                            "inline constexpr int x = [] { return 2; }();\n"
                          , output);
        }
    },
    {"Brace initialization works when assigning return value of a lambda initializing an inline global int variable", []
        {
            const auto& [violations, output] = RunTest ("inline constexpr auto x{[] { return 1; }()};"
                                                      , "inline constexpr auto x{[] { return 2; }()};");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: x\n"
                            "[tu3.cpp]\n"
                            "inline constexpr int x{[] { return 1; }()};\n"
                            "[tu4.cpp]\n"
                            "inline constexpr int x{[] { return 2; }()};\n"
                          , output);
        }
    },
    {"Brace initialization inside a namespace works when initializing an inline global int variable", []
        {
            const auto& [violations, output] = RunTest ("namespace Hi { inline constexpr auto x{1}; }"
                                                      , "namespace Hi { inline constexpr auto x{2}; }");
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: Hi::x\n"
                            "[tu3.cpp]\n"
                            "inline constexpr int Hi::x{1};\n"
                            "[tu4.cpp]\n"
                            "inline constexpr int Hi::x{2};\n"
                          , output);
        }
    },
    {"Brace initialization works when initializing an inline global std::pair variable", []
        {
            const auto& [violations, output] = RunTest ("#include <utility>\ninline std::pair<int, int> x{1, 2};"
                                                      , "#include <utility>\ninline std::pair<int, int> x{2, 1};");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: x\n"
                            "[tu3.cpp]\n"
                            "inline std::pair<int, int> x{1, 2};\n"
                            "[tu4.cpp]\n"
                            "inline std::pair<int, int> x{2, 1};\n"
                          , output);
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
        }
    },
    {"Brace initialization works when initializing an inline global std::pair variable, take 2", []
        {
            const auto& [violations, output] = RunTest ("#include <utility>\ninline std::pair<int, int> x{1,2};"
                                                      , "#include <utility>\ninline std::pair<int, int> x{1, 2};"); // spaces are not an ODR violation
            Assert::AreEqual("", output);
            Assert::AreEqual(0, violations, "wrong number of ODR violations");
        }
    },
    {"Assigning a lambda to an inline global lambda variable",[]
        {
            const auto& [violations, output] = RunTest ("inline constexpr auto x = [] { return 1; };"
                                                      , "inline constexpr auto x = [] { return 2; };");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: x\n"
                            "[tu3.cpp]\n"
                            "inline constexpr auto x = [] { return 1; };\n"
                            "[tu4.cpp]\n"
                            "inline constexpr auto x = [] { return 2; };\n"
                          , output);
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
        }
    },
    {"Non-static data-member initialization from a lambda", []
        {
            const auto& [violations, output] = RunTest ("struct S { int x = [] { return 1; }(); };"
                                                      , "struct S { int x = [] { return 2; }(); };");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: S\n"
                            "[tu3.cpp]\n"
                            "struct S { // sizeof=4\n"
                            "   int x=[] { return 1; }();\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct S { // sizeof=4\n"
                            "   int x=[] { return 2; }();\n"
                            "};\n", output);
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
        }
    },
    {"Template instantiations using lambda assigning to global inline int", []
        {
            const auto& [violations, output] = RunTest ("template<class T> inline int g() { return [] { return sizeof(T)  ; }(); } inline int x = g<int>();"
                                                      , "template<class T> inline int g() { return [] { return sizeof(T)+1; }(); } inline int x = g<int>();");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: g<class>()\n"
                            "[tu3.cpp]\n"
                            "template <class T> inline int __cdecl g() {\n"
                            "    return [] {\n"
                            "        return sizeof(T);\n"
                            "    }();\n"
                            "}\n"
                            "[tu4.cpp]\n"
                            "template <class T> inline int __cdecl g() {\n"
                            "    return [] {\n"
                            "        return sizeof(T) + 1;\n"
                            "    }();\n"
                            "}\n"
                            "\n"
                            "ODR VIOLATION: g<int>()\n"
                            "[tu3.cpp]\n"
                            "inline int __cdecl g<int>() {\n"
                            "    return [] {\n"
                            "        return sizeof(int);\n"
                            "    }();\n"
                            "}\n"
                            "[tu4.cpp]\n"
                            "inline int __cdecl g<int>() {\n"
                            "    return [] {\n"
                            "        return sizeof(int) + 1;\n"
                            "    }();\n"
                            "}\n", output);
            Assert::AreEqual(2, violations, "wrong number of ODR violations");
        }
    },
    {"testing lambdas, auto and template instantiations", []
        {
            const auto& [violations, output] = RunTest ("template<class T> auto f() { return [](T* /*p*/) { /* ... */ }; }"
                                                        "auto g_1_f_int   = f<int  >();"
                                                        "auto g_1_f_char  = f<char >();"
                                                        "auto g_1_f_short = f<short>();"
                                                        "auto g_1_f_long  = f<long >();"
                                                      , "template<class T> auto f() { return [](T* /*p*/) { /* ... */ }; }"
                                                        "auto g_2_f_int   = f<int  >();"
                                                        "auto g_2_f_char  = f<char >();"
                                                        "auto g_2_f_short = f<short>();"
                                                        "auto g_2_f_long  = f<long >();"
                                                      );
            Assert::AreEqual("", output);
            Assert::AreEqual(0, violations, "wrong number of ODR violations");
        }
    },
    //{"Mocking callable works", [] // no, it doesn't; I get:  "tu3.cpp:2:64: error: cannot mangle this pack expansion yet" which is a real bummer, because I use that a lot for TBCI
    //    {
    //        const auto& [violations, output] = RunTest ("#include <utility>\n"
    //                              "template<auto Fn> struct Callable { template<typename... Args> decltype(auto) operator()(Args&&... args) const { return Fn(std::forward<Args>(args)...); } };"
    //                                                  , "#include <utility>\n"
    //                              "template<auto Fn> struct Callable { template<typename... Args> decltype(auto) operator()(Args&&... args) const { return Fn(std::forward<Args>(args)...); } };");
    //        Assert::AreEqual("", output);
    //        Assert::AreEqual(0, violations, "wrong number of ODR violations");
    //    }
    //},
    {"Test 1 - Top-level anonymous type, different layouts. OdrCop2 SHOULD flag", [] // this is a change; Codex convinced me that the standard is tricky here and I decided to be conservative
        {
            const auto& [violations, output] = RunTest ("namespace Tests { namespace T1 { namespace { struct Empty { int x;    }; } Empty t1_instance; } }"
                                                      , "namespace Tests { namespace T1 { namespace { struct Empty { double y; }; } Empty t1_instance; } }");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: Tests::T1::t1_instance\n"
                            "[tu3.cpp]\n"
                            "struct Tests::T1::(anonymous namespace)::Empty { // sizeof=4\n"
                            "   int x;\n"
                            "} Tests::T1::t1_instance;\n"
                            "[tu4.cpp]\n"
                            "struct Tests::T1::(anonymous namespace)::Empty { // sizeof=8\n"
                            "   double y;\n"
                            "} Tests::T1::t1_instance;\n"
                          , output);
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
        }
    },
    {"Test 2 - Anonymous type inside external-linkage struct, different layouts. OdrCop2 SHOULD flag", []
        {
            const auto& [violations, output] = RunTest ("namespace T2 { namespace { struct Helper {          int x; ~Helper() {} }; } struct Public { Helper h; }; }"
                                                      , "namespace T2 { namespace { struct Helper { unsigned int y;              }; } struct Public { Helper h; }; }");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: T2::Public\n"
                            "[tu3.cpp]\n"
                            "struct T2::Public { // sizeof=4\n"
                            "   struct T2::(anonymous namespace)::Helper { // sizeof=4\n"
                            "      int x;\n"
                            "      void __cdecl ~Helper() {}\n"
                            "   } h;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct T2::Public { // sizeof=4\n"
                            "   struct T2::(anonymous namespace)::Helper { // sizeof=4\n"
                            "      unsigned int y;\n"
                            "   } h;\n"
                            "};\n", output);
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
        }
    },
    {"Test 3 - Anonymous type inside external-linkage struct, identical layouts. OdrCop should NOT flag", []
        {
            const auto& [violations, output] = RunTest ("namespace T3 { namespace { struct Helper { int x; }; } struct Public { Helper h; }; }"
                                                      , "namespace T3 { namespace { struct Helper { int x; }; } struct Public { Helper h; }; }");
            Assert::AreEqual("", output);
            Assert::AreEqual(0, violations, "wrong number of ODR violations");
        }
    },
    {"Test 4 - Anonymous type unused by external-linkage struct. OdrCop should NOT flag", []
        {
            const auto& [violations, output] = RunTest ("namespace T4 { namespace { struct Helper { int x; void f();    }; } struct Public { int a; }; Helper g_1_t4_helper_instance; Public g_1_t4_public_instance; }"
                                                      , "namespace T4 { namespace { struct Helper { int x; void f(int); }; } struct Public { int a; }; Helper g_2_t4_helper_instance; Public g_2_t4_public_instance; }");
            Assert::AreEqual("", output);
            Assert::AreEqual(0, violations, "wrong number of ODR violations");
        }
    },
    {"Test 5 - Anonymous type used only inside inline function. OdrCop should SHOULD flag", []
        {
            const auto& [violations, output] = RunTest ("namespace T5 {"
                                                        "    namespace { struct Local { int    x; }; }"
                                                        "    inline int f() { Local l{1}; return l.x; }"
                                                        "    Local g_1_t5_instance;"
                                                        "}"
                                                      , "namespace T5 {"
                                                        "    namespace { struct Local { double y; }; }"
                                                        "    inline int f() { Local l{1.0}; return (int)l.y; }"
                                                        "    Local g_2_t5_instance;"
                                                        "}");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: T5::f()\n"
                            "[tu3.cpp]\n"
                            "inline int __cdecl T5::f() {\n"
                            "    Local l{1};\n"
                            "    return l.x;\n"
                            "}\n"
                            "[tu4.cpp]\n"
                            "inline int __cdecl T5::f() {\n"
                            "    Local l{1.};\n"
                            "    return (int)l.y;\n"
                            "}\n", output);
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
        }
    },
    {"Test 6 - Anonymous type as template argument, external-linkage instantiation, template type is in anonymous namespace. OdrCop should NOT flag", []
        {
            { // without instantiation
                const auto& [violations, output] = RunTest ("namespace T6 {"
                                                            "   namespace { struct Tag { int x; }; }"
                                                            "   template<typename T> struct Wrapper { T t; };"
                                                         // "   inline Wrapper<Tag> f1() { return {}; }"
                                                            "}"
                                                          , "namespace T6 {"
                                                            "   namespace { struct Tag { double y; }; }"
                                                            "   template<typename T> struct Wrapper { T t; };"
                                                         // "   inline Wrapper<Tag> f2() { return {}; }"
                                                            "}");
                Assert::AreEqual("", output);
                Assert::AreEqual(0, violations, "wrong number of ODR violations");
            }

            { // with instantiation
                const auto& [violations, output] = RunTest ("namespace T6 {"
                                                            "   namespace { struct Tag { int x; }; }"
                                                            "   template<typename T> struct Wrapper { T t; };"
                                                            "   inline Wrapper<Tag> f1() { return {}; }"
                                                            "}"
                                                          , "namespace T6 {"
                                                            "   namespace { struct Tag { double y; }; }"
                                                            "   template<typename T> struct Wrapper { T t; };"
                                                            "   inline Wrapper<Tag> f2() { return {}; }"
                                                            "}");
                Assert::AreEqual("", output);
                Assert::AreEqual(0, violations, "wrong number of ODR violations");
            }
        }
    },
    {"Test 7 - Anonymous type as base class of external-linkage struct, different layouts. OdrCop SHOULD flag", []
        {
            { // different sizes:  easy
                const auto& [violations, output] = RunTest ("namespace T7 { namespace { struct Base { int    x; }; } struct Public : Base {}; };"
                                                          , "namespace T7 { namespace { struct Base { double y; }; } struct Public : Base {}; };");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: T7::Public\n"
                                "[tu3.cpp]\n"
                                "struct T7::Public : public struct T7::(anonymous namespace)::Base { // sizeof=4\n"
                                "                              int x;\n"
                                "                           } { // sizeof=4\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "struct T7::Public : public struct T7::(anonymous namespace)::Base { // sizeof=8\n"
                                "                              double y;\n"
                                "                           } { // sizeof=8\n"
                                "};\n", output);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
            }
            {
                const auto& [violations, output] = RunTest ("namespace T7 { namespace { struct Base { int x; }; } struct Public : Base {}; };"
                                                          , "namespace T7 { namespace { struct Base { int y; }; } struct Public : Base {}; };");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: T7::Public\n"
                                "[tu3.cpp]\n"
                                "struct T7::Public : public struct T7::(anonymous namespace)::Base { // sizeof=4\n"
                                "                              int x;\n"
                                "                           } { // sizeof=4\n"
                                "};\n"
                                "[tu4.cpp]\n"
                                "struct T7::Public : public struct T7::(anonymous namespace)::Base { // sizeof=4\n"
                                "                              int y;\n"
                                "                           } { // sizeof=4\n"
                                "};\n", output);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
            }
        }
    },
    {"Test 8 - Anonymous type as base class of external-linkage struct, identical layouts. OdrCop should NOT flag", []
        {
            const auto& [violations, output] = RunTest ("namespace T8 { namespace { struct Base { int x; }; } struct Public : Base {}; }"
                                                      , "namespace T8 { namespace { struct Base { int x; }; } struct Public : Base {}; }");
            Assert::AreEqual("", output);
            Assert::AreEqual(0, violations, "wrong number of ODR violations");
        }
    },
    {"Test 9 - Anonymous type as parameter of external-linkage function, different layouts. OdrCop should NOT flag", []
        {
            OdrCop2::AllMaps maps;
            tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), "namespace T9 { namespace { struct Arg { int    x; }; } void function9(Arg a) { (void)a; } }", { "-x", "c++", "-std=c++23" }, "tu3.cpp");
            tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), "namespace T9 { namespace { struct Arg { double y; }; } void function9(Arg a) { (void)a; } }", { "-x", "c++", "-std=c++23" }, "tu4.cpp");

            Assert::AreEqual(2, maps.functionMap.size());

            auto it = maps.functionMap.begin();
            Assert::AreEqual("void __cdecl T9::function9(struct T9::(anonymous namespace)::Arg { // sizeof=4\n"
                             "                              int x;\n"
                             "                           } a) { (void)a; }"
                             , it->second[0].fullyQualified);
            ++it;
            Assert::AreEqual("void __cdecl T9::function9(struct T9::(anonymous namespace)::Arg { // sizeof=8\n"
                "                              double y;\n"
                "                           } a) { (void)a; }"
                , it->second[0].fullyQualified);


            std::ostringstream oss;
            int violations = OdrCop2::OdrViolationReporter::ReportOdrViolations(maps, oss);
            Assert::AreEqual(0, violations);
            Assert::AreEqual("", oss.str());
        }
    },
    {"Test 9a - Pointer to Anonymous type as parameter of external-linkage function", []
        {
            { // pointer anonymous namespace arg
                OdrCop2::AllMaps maps;
                tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), "namespace T9 { namespace { struct Arg { int x; }; } void function9(Arg* a) { (void)a; } }", { "-x", "c++", "-std=c++23" }, "tu3.cpp");

                Assert::AreEqual(1, maps.functionMap.size());
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void __cdecl T9::function9(struct T9::(anonymous namespace)::Arg { // sizeof=4\n"
                                 "                              int x;\n"
                                 "                           } * a) { (void)a; }"
                                 , it->second[0].fullyQualified);
            }
            { // pointer arg
                OdrCop2::AllMaps maps;
                tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), "namespace T9 { struct Arg { int x; }; void function9(const Arg* a) { (void)a; } }", { "-x", "c++", "-std=c++23" }, "tu3.cpp");

                Assert::AreEqual(1, maps.functionMap.size());
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void __cdecl T9::function9(const Arg * a) { (void)a; }", it->second[0].fullyQualified);
            }
            { // reference to const arg
                OdrCop2::AllMaps maps;
                tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), "namespace T9 { struct Arg { int x; }; void function9(const Arg& a) { (void)a; } }", { "-x", "c++", "-std=c++23" }, "tu3.cpp");

                Assert::AreEqual(1, maps.functionMap.size());
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void __cdecl T9::function9(const Arg & a) { (void)a; }", it->second[0].fullyQualified);
            }
            { // pointer to pointer arg
                OdrCop2::AllMaps maps;
                tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), "namespace T9 { struct Arg { int x; }; void function9(Arg** a) { (void)a; } }", { "-x", "c++", "-std=c++23" }, "tu3.cpp");

                Assert::AreEqual(1, maps.functionMap.size());
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void __cdecl T9::function9(Arg ** a) { (void)a; }", it->second[0].fullyQualified);
            }
            { // pointer to pointer to const& arg
                OdrCop2::AllMaps maps;
                tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), "namespace T9 { struct Arg { int x; }; void function9(Arg** const& a) { (void)a; } }", { "-x", "c++", "-std=c++23" }, "tu3.cpp");

                Assert::AreEqual(1, maps.functionMap.size());
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void __cdecl T9::function9(Arg **const & a) { (void)a; }", it->second[0].fullyQualified);
            }

            { // const reference to anonymous namespace arg
                OdrCop2::AllMaps maps;
                tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), "namespace T9 { namespace { struct Arg { int x; }; } void function9(const Arg& a) { (void)a; } }", { "-x", "c++", "-std=c++23" }, "tu3.cpp");

                Assert::AreEqual(1, maps.functionMap.size());
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void __cdecl T9::function9(const struct T9::(anonymous namespace)::Arg { // sizeof=4\n"
                                 "                                    int x;\n"
                                 "                                 } & a) { (void)a; }"
                                 , it->second[0].fullyQualified);
            }
            { // moveable anonymous namespace arg
                OdrCop2::AllMaps maps;
                tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), "namespace T9 { namespace { struct Arg { int x; }; } void function9(Arg&& a) { (void)a; } }", { "-x", "c++", "-std=c++23" }, "tu3.cpp");

                Assert::AreEqual(1, maps.functionMap.size());
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void __cdecl T9::function9(struct T9::(anonymous namespace)::Arg { // sizeof=4\n"
                                 "                              int x;\n"
                                 "                           } && a) { (void)a; }"
                                 , it->second[0].fullyQualified);
            }
            { // pointer to pointer to const& anonymous namespace arg
                OdrCop2::AllMaps maps;
                tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), "namespace T9 { namespace { struct Arg { int x; }; } void function9(Arg** const& a) { (void)a; } }", { "-x", "c++", "-std=c++23" }, "tu3.cpp");

                Assert::AreEqual(1, maps.functionMap.size());
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void __cdecl T9::function9(struct T9::(anonymous namespace)::Arg { // sizeof=4\n"
                                 "                              int x;\n"
                                 "                           } **const & a) { (void)a; }"
                                 , it->second[0].fullyQualified);
            }
        }
    },
    {"Test 9b - Field is pointer to Anonymous namespace type", []
        {
            { // pointer anonymous namespace arg
                OdrCop2::AllMaps maps;
                tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), "namespace T9 { namespace { struct Arg { int x; }; } struct Foo { Arg* a; }; }", { "-x", "c++", "-std=c++23" }, "tu3.cpp");

                Assert::AreEqual(1, maps.udtMap.size());
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct T9::Foo { // sizeof=8\n"
                                 "   struct T9::(anonymous namespace)::Arg { // sizeof=4\n"
                                 "      int x;\n"
                                 "   } * a;\n"
                                 "};"
                               , it->second[0].fullyQualified);
            }
            { // reference to const anonymous namespace arg
                OdrCop2::AllMaps maps;
                tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), "namespace T9 { namespace { struct Arg { int x; }; } struct Foo { const Arg& a; }; }", { "-x", "c++", "-std=c++23" }, "tu3.cpp");

                Assert::AreEqual(1, maps.udtMap.size());
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct T9::Foo { // sizeof=8\n"
                                 "   const struct T9::(anonymous namespace)::Arg { // sizeof=4\n"
                                 "      int x;\n"
                                 "   } & a;\n"
                                 "};"
                               , it->second[0].fullyQualified);
            }
            { // reference to pointer to volatile anonymous namespace arg
                OdrCop2::AllMaps maps;
                tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), "namespace T9 { namespace { struct Arg { int x; }; } struct Foo { volatile Arg* & a; }; }", { "-x", "c++", "-std=c++23" });

                Assert::AreEqual(1, maps.udtMap.size());
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct T9::Foo { // sizeof=8\n"
                                 "   volatile struct T9::(anonymous namespace)::Arg { // sizeof=4\n"
                                 "      int x;\n"
                                 "   } *& a;\n"
                                 "};"
                               , it->second[0].fullyQualified);
            }
        }
    },
    {"Test 9c - Return type that is an Anonymous namespace type", []
        {
            { // anonymous namespace return value
                OdrCop2::AllMaps maps;
                tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), "namespace T9 { namespace { struct Arg { int x; }; } Arg Foo() { return Arg{}; } }", { "-x", "c++", "-std=c++23" });

                Assert::AreEqual(1, maps.functionMap.size());
                auto it = maps.functionMap.begin();
                Assert::AreEqual("struct T9::(anonymous namespace)::Arg { // sizeof=4\n"
                                 "   int x;\n"
                                 "} __cdecl T9::Foo() { return Arg{}; }"
                               , it->second[0].fullyQualified);
            }
            { // pointer to a const anonymous namespace return value
                OdrCop2::AllMaps maps;
                tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), "namespace T9 { namespace { struct Arg { int x; }; } const Arg* Foo() { return new Arg{}; } }", { "-x", "c++", "-std=c++23" });

                Assert::AreEqual(1, maps.functionMap.size());
                auto it = maps.functionMap.begin();
                Assert::AreEqual("const struct T9::(anonymous namespace)::Arg { // sizeof=4\n"
                                 "         int x;\n"
                                 "      } * __cdecl T9::Foo() { return new Arg{}; }"
                               , it->second[0].fullyQualified);
            }
        }
    },
    {"Test 10 - Anonymous type as return type of external-linkage function, different layouts. OdrCop SHOULD flag", []
        {
            {
                const auto& [violations, output] = RunTest ("namespace T10 { namespace { struct Result { int    x; }; } Result ReturnAnAnonymousType() { return {}; }; }"
                                                          , "namespace T10 { namespace { struct Result { double y; }; } Result ReturnAnAnonymousType() { return {}; }; }");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: T10::ReturnAnAnonymousType()\n"
                                "[tu3.cpp]\n"
                                "struct T10::(anonymous namespace)::Result { // sizeof=4\n"
                                "   int x;\n"
                                "} __cdecl T10::ReturnAnAnonymousType() { return {}; }\n"
                                "[tu4.cpp]\n"
                                "struct T10::(anonymous namespace)::Result { // sizeof=8\n"
                                "   double y;\n"
                                "} __cdecl T10::ReturnAnAnonymousType() { return {}; }\n"
                              , output);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
            }

            { // return short vs long
                const auto& [violations, output] = RunTest ("namespace T10 { short ReturnBasicType() { return 0; }; }"
                                                          , "namespace T10 { long  ReturnBasicType() { return 0; }; }");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: T10::ReturnBasicType()\n"
                                "[tu3.cpp]\n"
                                "short __cdecl T10::ReturnBasicType() { return 0; }\n"
                                "[tu4.cpp]\n"
                                "long __cdecl T10::ReturnBasicType() { return 0; }\n"
                              , output);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
            }
            { // empty struct
                const auto& [violations, output] = RunTest ("namespace T10 { struct Foo{}; Foo ReturnBasicType() { return {}; }; }"
                                                          , "namespace T10 { struct Bar{}; Bar ReturnBasicType() { return {}; }; }");
                Assert::AreEqual("\n"
                                "ODR VIOLATION: T10::ReturnBasicType()\n"
                                "[tu3.cpp]\n"
                                "T10::Foo __cdecl T10::ReturnBasicType() { return {}; }\n"
                                "[tu4.cpp]\n"
                                "T10::Bar __cdecl T10::ReturnBasicType() { return {}; }\n"
                              , output);
                Assert::AreEqual(1, violations, "wrong number of ODR violations");
            }
        }
    },
    {"Test 10a - Anonymous enum as return type of external-linkage function, different layouts. OdrCop SHOULD flag", []
        {
            const auto& [violations, output] = RunTest ("namespace T10a { namespace { enum Result { Zero, One      }; } Result ReturnAnAnonymousEnum() { return Zero; } }"
                                                      , "namespace T10a { namespace { enum Result { Zero, One, Two }; } Result ReturnAnAnonymousEnum() { return Zero; } }");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: T10a::ReturnAnAnonymousEnum()\n"
                            "[tu3.cpp]\n"
                            "enum (anonymous namespace)::Result { Zero=0, One=1 } __cdecl T10a::ReturnAnAnonymousEnum() { return Zero; }\n"
                            "[tu4.cpp]\n"
                            "enum (anonymous namespace)::Result { Zero=0, One=1, Two=2 } __cdecl T10a::ReturnAnAnonymousEnum() { return Zero; }\n"
                          , output);
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
        }
    },
    {"Test - operator long() ODR violation: differing return values", []
        {
            const auto& [violations, output] = RunTest ("struct Converter { operator long() const noexcept { return 42L; } };"
                                                      , "struct Converter { operator long() const noexcept { return 99L; } };");
            Assert::AreEqual("\n"

                            "ODR VIOLATION: Converter::operator long() const\n"
                            "[tu3.cpp]\n"
                            "long __cdecl Converter::operator long() const noexcept { return 42L; }\n"
                            "[tu4.cpp]\n"
                            "long __cdecl Converter::operator long() const noexcept { return 99L; }\n"
                            "\n"
                            "ODR VIOLATION: Converter\n"
                            "[tu3.cpp]\n"
                            "struct Converter { // sizeof=1\n"
                            "   long __cdecl operator long() const noexcept { return 42L; }\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct Converter { // sizeof=1\n"
                            "   long __cdecl operator long() const noexcept { return 99L; }\n"
                            "};\n", output);
            Assert::AreEqual(2, violations, "wrong number of ODR violations");
        }
    },
    {"Test - explicit operator T() ODR violation on class template: differing return values", []
        {
            const auto& [violations, output] = RunTest ("template<typename T> struct Converter { explicit operator T() const { return T(42); } };"
                                                      , "template<typename T> struct Converter { explicit operator T() const { return T(99); } };");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: Converter::operator T() const\n"
                            "[tu3.cpp]\n"
                            "explicit T __cdecl Converter::operator T() const { return T(42); }\n"
                            "[tu4.cpp]\n"
                            "explicit T __cdecl Converter::operator T() const { return T(99); }\n"
                            "\n"
                            "ODR VIOLATION: Converter<>\n"
                            "[tu3.cpp]\n"
                            "template<typename T> struct Converter {\n"
                            "   explicit T __cdecl operator T() const { return T(42); }\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "template<typename T> struct Converter {\n"
                            "   explicit T __cdecl operator T() const { return T(99); }\n"
                            "};\n", output);
            Assert::AreEqual(2, violations, "wrong number of ODR violations");
        }
    },
    {"Test - explicit operator T() as function template on non-template class: ODR violation", []
        {
            const auto& [violations, output] = RunTest ("struct Converter { template<typename T> explicit operator T() const { return T(42); } };"
                                                      , "struct Converter { template<typename T> explicit operator T() const { return T(99); } };");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: Converter::operator T<T>() const\n"
                            "[tu3.cpp]\n"
                            "template <typename T> explicit T __cdecl Converter::operator T() const { return T(42); }\n"
                            "[tu4.cpp]\n"
                            "template <typename T> explicit T __cdecl Converter::operator T() const { return T(99); }\n"
                            "\n"
                            "ODR VIOLATION: Converter\n"
                            "[tu3.cpp]\n"
                            "struct Converter { // sizeof=1\n"
                            "   template <typename T> explicit T __cdecl operator T() const { return T(42); }\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct Converter { // sizeof=1\n"
                            "   template <typename T> explicit T __cdecl operator T() const { return T(99); }\n"
                            "};\n"
                          , output);
            Assert::AreEqual(2, violations, "wrong number of ODR violations");
        }
    },
    {"Test 11 - Doubly-nested anonymous namespace, different layouts. OdrCop should NOT flag", []
        {
            const auto& [violations, output] = RunTest ("namespace T11 { namespace { namespace { struct Empty { int    x; }; } } }"
                                                      , "namespace T11 { namespace { namespace { struct Empty { double y; }; } } }");
            Assert::AreEqual("", output);
            Assert::AreEqual(0, violations, "wrong number of ODR violations");
        }
    },
    {"Test 12 - Anonymous Empty as template argument, different layouts. OdrCop should NOT flag", []
        {
            const auto& [violations, output] = RunTest ("namespace T12 { namespace { struct Empty {              }; } template<typename T> struct ClassUnderTest { T t; }; }"
                                                      , "namespace T12 { namespace { struct Empty { int payload; }; } template<typename T> struct ClassUnderTest { T t; }; }");
            Assert::AreEqual("", output);
            Assert::AreEqual(0, violations, "wrong number of ODR violations");
        }
    },
    {"Test 13 - a typedef of a type inside an anonymous namespace. OdrCop SHOULD flag", []
        {
            const auto& [violations, output] = RunTest ("namespace { struct SomeStructForTypedefTesting { int x1; }; }"
                                                        "namespace T13 { struct AnonymousTypedefDefinition { typedef SomeStructForTypedefTesting SameTypedefDifferentUnderlyingType; }; }"
                                                      , "namespace { struct SomeStructForTypedefTesting { int x2; }; }"
                                                        "namespace T13 { struct AnonymousTypedefDefinition { typedef SomeStructForTypedefTesting SameTypedefDifferentUnderlyingType; }; }");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: T13::AnonymousTypedefDefinition\n"
                            "[tu3.cpp]\n"
                            "struct T13::AnonymousTypedefDefinition { // sizeof=1\n"
                            "   typedef struct (anonymous namespace)::SomeStructForTypedefTesting { // sizeof=4\n"
                            "              int x1;\n"
                            "           } SameTypedefDifferentUnderlyingType;\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct T13::AnonymousTypedefDefinition { // sizeof=1\n"
                            "   typedef struct (anonymous namespace)::SomeStructForTypedefTesting { // sizeof=4\n"
                            "              int x2;\n"
                            "           } SameTypedefDifferentUnderlyingType;\n"
                            "};\n", output);
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
        }
    },
    {"Test 13a - A using alias of a type inside an anonymous namespace. OdrCop SHOULD flag", []
        {
            const auto& [violations, output] = RunTest ("namespace { struct SomeStructForAliasTesting { int x1; }; }"
                                                        "namespace T13 { struct AnonymousAliasDefinition { using SameAliasDifferentUnderlyingType = SomeStructForAliasTesting; }; }"
                                                      , "namespace { struct SomeStructForAliasTesting { int x2; }; }"
                                                        "namespace T13 { struct AnonymousAliasDefinition { using SameAliasDifferentUnderlyingType = SomeStructForAliasTesting; }; }");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: T13::AnonymousAliasDefinition\n"
                            "[tu3.cpp]\n"
                            "struct T13::AnonymousAliasDefinition { // sizeof=1\n"
                            "   using SameAliasDifferentUnderlyingType = struct (anonymous namespace)::SomeStructForAliasTesting { // sizeof=4\n"
                            "                                               int x1;\n"
                            "                                            };\n"
                            "};\n"
                            "[tu4.cpp]\n"
                            "struct T13::AnonymousAliasDefinition { // sizeof=1\n"
                            "   using SameAliasDifferentUnderlyingType = struct (anonymous namespace)::SomeStructForAliasTesting { // sizeof=4\n"
                            "                                               int x2;\n"
                            "                                            };\n"
                            "};\n", output);
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
        }
    },








};
#ifdef KEEP
    {"
        
        
        ", []
        {
            const auto& [violations, output] = RunTest ("
                
                
                "
                                                      , "
                
                
                ");
            Assert::AreEqual("\n"





                            "};\n", output);
            Assert::AreEqual(1, violations, "wrong number of ODR violations");
        }
    },

#endif