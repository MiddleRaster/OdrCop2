#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"

std::pair<int,std::string> RunTest(const std::string& code1, const std::string& code2)
{
    OdrCop2::AllMaps maps;
    Assert::IsTrue(clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code1, { "-x", "c++" }, "tu3.cpp"));
    Assert::IsTrue(clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(maps), code2, { "-x", "c++" }, "tu4.cpp"));

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
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
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
                            "ODR VIOLATION: ?value@DifferentMethodNoexcept@@QEAAHXZ\n"
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
                            "ODR VIOLATION: ?value@DifferentMethodAttributes@@QEAAHXZ\n"
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
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentTypedefTarget::Alias\n"
                            "[tu3.cpp]\n"
                            "DifferentTypedefTarget::Alias = int\n"
                            "[tu4.cpp]\n"
                            "DifferentTypedefTarget::Alias = long\n"
                           , output);
        }
    },
    {"TU34-029: Same external-linkage struct name, using-alias target differs. Expected ODR violation: YES.", []
        {
            const auto& [violations, output] = RunTest( "struct DifferentUsingAliasTarget { using Alias = int ; Alias value; };"
                                                    ,   "struct DifferentUsingAliasTarget { using Alias = long; Alias value; };");
            Assert::AreEqual(1, violations, "there should be 1 ODR violation");
            Assert::AreEqual("\n"
                            "ODR VIOLATION: DifferentUsingAliasTarget::Alias\n"
                            "[tu3.cpp]\n"
                            "DifferentUsingAliasTarget::Alias = int\n"
                            "[tu4.cpp]\n"
                            "DifferentUsingAliasTarget::Alias = long\n"
                           , output);
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
                            "ODR VIOLATION: ?DifferentInlineFunctionBody@@YAHH@Z\n"
                            "[tu3.cpp]\n"
                            "inline int __cdecl DifferentInlineFunctionBody(int) { return x + 34; }\n"
                            "[tu4.cpp]\n"
                            "inline int __cdecl DifferentInlineFunctionBody(int) { return x + 340; }\n", output);
        }
    },
};
