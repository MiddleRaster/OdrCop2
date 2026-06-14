#pragma once

#include <clang\AST\AST.h>
#include <clang\AST\RecursiveASTVisitor.h>
#include <clang\Frontend\FrontendActions.h>
#include <clang\Frontend\CompilerInstance.h>
#include <clang\Tooling\Tooling.h>
#include <clang\Tooling\CompilationDatabase.h>
#include <clang\AST\Mangle.h>
#include <clang\AST\Decl.h>
#include <clang\AST\GlobalDecl.h>
#include <llvm\Support\raw_ostream.h>
using namespace clang;

#include <vector>

#include <windows.h>

namespace OdrCop2
{
    struct FunctionInfo
    {
        const std::string TU;
        const std::string mangled;
        const std::string fullyQualified;
        bool operator==(const FunctionInfo& other) const { return fullyQualified == other.fullyQualified; }
    };
    struct EnumInfo
    {
        const std::string TU;
        const std::string fullyQualified; // eg: enum class Color : uint8_t { Red=1, Green=2, Blue=3, };
        bool operator==(const EnumInfo& other) const { return fullyQualified == other.fullyQualified; }
    };
    struct TypedefInfo
    {
        const std::string TU;
        const std::string fullyQualified; // eg: INT=int
        bool operator==(const TypedefInfo& other) const { return fullyQualified == other.fullyQualified; }
    };
    struct UdtInfo
    {
        const std::string TU;
        const std::string fullyQualified;
        bool operator==(const UdtInfo& other) const { return fullyQualified == other.fullyQualified; }
    };
    struct AllMaps
    {
        std::map<std::string,std::vector<     UdtInfo>>      udtMap;
        std::map<std::string,std::vector<    EnumInfo>>     enumMap;
        std::map<std::string,std::vector< TypedefInfo>>  typedefMap;
        std::map<std::string,std::vector<FunctionInfo>> functionMap;
    };

    class TheVisitor : public RecursiveASTVisitor<TheVisitor>
    {
        const std::string TU;
        ASTContext* context;
        const PrintingPolicy printPolicy;
        AllMaps& maps;
    public:
        TheVisitor(ASTContext* context, AllMaps& maps, const std::string& TU)
            : TU           (TU)
            , context      (context)
            , printPolicy  (context->getLangOpts())
            , maps         (maps)
        {}
        bool VisitFunctionDecl(FunctionDecl* funcDecl)
        {
            if (context->getSourceManager().isInSystemHeader(funcDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            if (funcDecl->isThisDeclarationADefinition())
            {
                auto mangledName = getMSVCMangledName(funcDecl);
                maps.functionMap[mangledName].push_back({TU, mangledName, ConstructFunctionSignature(funcDecl)});
            }
            return true;
        }
        bool VisitCXXRecordDecl(CXXRecordDecl* recordDecl)
        {
            if (context->getSourceManager().isInSystemHeader(recordDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            if (recordDecl->isThisDeclarationADefinition())
            {
                maps.udtMap[recordDecl->getQualifiedNameAsString()].push_back({TU, ConstructRecordSignature(recordDecl)});
            }
            return true;
        }
        bool VisitEnumDecl(clang::EnumDecl* enumDecl)
        {
            if (context->getSourceManager().isInSystemHeader(enumDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            if (enumDecl->isThisDeclarationADefinition())
            {
                std::string underlyingType = enumDecl->getIntegerType().getCanonicalType().getAsString();
                bool              isScoped = enumDecl->isScoped();   // enum class vs enum
                bool               isFixed = enumDecl->isFixed();
                std::string       enumName = enumDecl->getNameAsString();
                std::string     prettyEnum = enumDecl->getQualifiedNameAsString();

                std::string fqe = (isScoped ? "enum class " : "enum ") + prettyEnum + (isFixed ? " : " + underlyingType : "") + " { ";
                for (const clang::EnumConstantDecl* enumeratorDecl : enumDecl->enumerators())
                {
                    std::string enumeratorName = enumeratorDecl->getName().str();
                    std::string val            = llvm::toString(enumeratorDecl->getInitVal(), 10);
                    fqe += enumeratorName  +  "="  +  val + ", ";
                }
                fqe = fqe.substr(0, fqe.size()-2) + " };";
                
                maps.enumMap[prettyEnum].push_back({TU, fqe});
            }
            return true;
        }
        bool VisitTypedefNameDecl(clang::TypedefNameDecl* typedefDecl)
        {
            if (context->getSourceManager().isInSystemHeader(typedefDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            std::string aliasName    = typedefDecl->getQualifiedNameAsString();
            std::string resolvedType = typedefDecl->getUnderlyingType().getCanonicalType().getAsString(printPolicy);
            std::string fqtd         = aliasName + " = " + resolvedType;
            maps.typedefMap[aliasName].push_back({TU, fqtd});
            return true;
        }

    private:
        std::string ConstructFunctionSignature(const clang::FunctionDecl* funcDecl, bool wantFullyQualifiedMethodName=true)
        {
            std::string fqn;

            // access specifier
            AccessSpecifier access = funcDecl->getAccess();
            switch (access)
            {
            case AS_public:    fqn +=    "public: "; break;
            case AS_protected: fqn += "protected: "; break;
            case AS_private:   fqn +=   "private: "; break;
            default:                                 break;
            }

            // if it's a template, that comes first
            if (const FunctionTemplateDecl* ftd = funcDecl->getDescribedFunctionTemplate())
            {
                std::string              templatePrefix;
                llvm::raw_string_ostream os(templatePrefix);
                ftd->getTemplateParameters()->print(os, *context, printPolicy);
                os.flush();
                fqn += templatePrefix;
            }

            // any attributes
            SourceLocation nameEnd = funcDecl->getNameInfo().getEndLoc();
            for (const Attr* attr : funcDecl->attrs())
            {
                SourceLocation attrLoc = attr->getLocation();
                if (attrLoc > nameEnd)
                    continue; // this is a trailing attribute: int f() [[attr]];

                std::string out;
                llvm::raw_string_ostream os(out);
                attr->printPretty(os, printPolicy);
                os.flush();
                fqn += out + ' ';
            }

            // friend keyword
            if (funcDecl->getFriendObjectKind() != Decl::FOK_None)
                fqn += "friend ";

            // storage class
            if (funcDecl->isStatic())
                fqn += "static ";
            if (funcDecl->getStorageClass() == SC_Extern)
                fqn += "extern ";

            // function specifiers: virtual/explicit/inline
            if (funcDecl->isVirtualAsWritten())
                fqn += "virtual ";
            if (const auto* ctor = dyn_cast<CXXConstructorDecl>(funcDecl)) {
                if (ctor->isExplicit())
                    fqn += "explicit ";
            } else if (const auto* conv = dyn_cast<CXXConversionDecl>(funcDecl)) {
                if (conv->isExplicit())
                    fqn += "explicit ";
            }
            if (funcDecl->isInlineSpecified())
                fqn += "inline ";

            // other specifiers: constexpr, consteval
            if (funcDecl->isConstexpr())
                fqn += "constexpr ";
            if (funcDecl->isConsteval())
                fqn += "consteval ";

            // return type
         // fqn += funcDecl->getReturnType().getCanonicalType().getAsString(printPolicy) + " ";
            if (funcDecl->getDescribedFunctionTemplate() != nullptr)
                fqn += funcDecl->getReturnType().getAsString(printPolicy) + " ";
            else
                fqn += funcDecl->getReturnType().getCanonicalType().getAsString(printPolicy) + " ";

            // calling convention
            switch (funcDecl->getType()->castAs<FunctionType>()->getCallConv())
            {
            case CC_C: if (!(funcDecl->isExternC() || funcDecl->isMSVCRTEntryPoint()))
                                   fqn += "__cdecl ";       break;
            case CC_X86StdCall:    fqn += "__stdcall ";     break;
            case CC_X86FastCall:   fqn += "__fastcall ";    break;
            case CC_X86ThisCall:   fqn += "__thiscall ";    break;
            case CC_X86VectorCall: fqn += "__vectorcall ";  break;
            case CC_Win64:         fqn += "__ms_abi ";      break;
            default:                                        break;
            }

            // function name
            {
                std::string out;
                llvm::raw_string_ostream os(out);
                if (const FunctionTemplateDecl* ftd = funcDecl->getDescribedFunctionTemplate())
                {
                    if (wantFullyQualifiedMethodName)
                        ftd->getTemplatedDecl()->printQualifiedName(os, printPolicy);
                    else
                        ftd->getTemplatedDecl()->getNameAsString();
                }
                else
                {
                    if (wantFullyQualifiedMethodName)
                        funcDecl->printQualifiedName(os, printPolicy);
                    else
                        out = funcDecl->getNameAsString();
                }
                os.flush();

                if (const auto* args = funcDecl->getTemplateSpecializationArgs())
                {
                    llvm::raw_string_ostream  os2(out);
                    out += "<";
                    bool first = true;
                    for (const TemplateArgument& arg : args->asArray())
                    {
                        if (!first)
                            out += ", ";
                        arg.print(printPolicy, os2, true);
                        os2.flush();
                        first = false;
                    }
                    out += ">";
                }

                fqn += out;
            }

            // args
            fqn += '(';
            for (const ParmVarDecl* param : funcDecl->parameters())
            {
                QualType    type     = param->getType();
                std::string typeName = type.getAsString(printPolicy);
             // std::string name     = param->getNameAsString();   // empty if unnamed

                fqn += typeName; // +" " + name;

                // Default argument, if any
                if (param->hasDefaultArg())
                {
                    std::string out;
                    llvm::raw_string_ostream os(out);
                    param->getDefaultArg()->printPretty(os, nullptr, printPolicy);
                    os.flush();
                    fqn += " = " + out;
                }
                fqn += ", ";
            }
            if (fqn.substr(fqn.size()-2) == ", ")   // if there are args
                fqn = fqn.substr(0, fqn.size()-2);  // strip off last ", "
            fqn += ") ";

            // cv
            if (const auto* method = dyn_cast<CXXMethodDecl>(funcDecl))
            {
                if (method->isConst())
                    fqn += "const ";
                if (method->isVolatile())
                    fqn += "volatile ";
            }
            // & and &&
            if (const auto* method = dyn_cast<CXXMethodDecl>(funcDecl))
            {
                switch (method->getRefQualifier())
                {
                default:
                case RQ_None  :               break;
                case RQ_LValue: fqn += "& ";  break;
                case RQ_RValue: fqn += "&& "; break;
                }
            }
            // __ptr64
            //if (isa<CXXMethodDecl>(funcDecl) && !cast<CXXMethodDecl>(funcDecl)->isStatic())
            //    fqn += "__ptr64 ";

            // noexcept/throw()
            {
                const auto* proto = funcDecl->getType()->getAs<FunctionProtoType>();
                if (proto != nullptr)
                {
                    switch (proto->getExceptionSpecType())
                    {
                    default:
                    case EST_None:                fqn += "";                 break;
                    case EST_NoexceptTrue:        fqn += "noexcept(true) ";  break;
                    case EST_NoexceptFalse:       fqn += "noexcept(false) "; break;
                    case EST_DynamicNone:         fqn += "throw() ";         break;
                    case EST_MSAny:               fqn += "throw(...) ";      break;

                    case EST_BasicNoexcept:
                        if (funcDecl->getExceptionSpecSourceRange().isValid()) // only if the user actually wrote this (i.e., not "inferred" by the compiler)
                            fqn += "noexcept ";
                        break;

                    case EST_DependentNoexcept:
                    {
                        std::string              exprStr;
                        llvm::raw_string_ostream os(exprStr);
                        proto->getNoexceptExpr()->printPretty(os, nullptr, printPolicy);
                        os.flush();
                        fqn += "noexcept(" + exprStr + ") ";
                        break;
                    }
                    case EST_Dynamic:
                    {
                        std::string result = "throw(";
                        bool        first = true;
                        for (QualType t : proto->exceptions())
                        {
                            if (!first)
                                result += ", ";
                            result += t.getAsString(printPolicy);
                            first = false;
                        }
                        result += ") ";
                        fqn += result;
                    }
                    }
                }
            }

            // trailing attributes
            for (const Attr* attr : funcDecl->attrs())
            {
                SourceLocation attrLoc = attr->getLocation();
                if (attrLoc < nameEnd)
                    continue; // this is a leading attribute: [[attr]] int f();

                std::string out;
                llvm::raw_string_ostream os(out);
                attr->printPretty(os, printPolicy);
                os.flush();
                fqn += out + ' ';
            }

            // override / final
            if (const auto* method = dyn_cast<CXXMethodDecl>(funcDecl))
            {
                if (method->hasAttr<OverrideAttr>())
                    fqn += "override ";
                if (method->hasAttr<FinalAttr>())
                    fqn += "final ";
            }

            // = 0
            if (const auto* method = dyn_cast<CXXMethodDecl>(funcDecl))
            {
                if (method->isPureVirtual())
                    fqn += "=0 ";
            }
            // = default
            if (funcDecl->isDefaulted())
                fqn += "=default ";
            // = delete
            if (funcDecl->isDeleted())
                fqn += "= delete ";

            // strip off the last " "
            return fqn.substr(0, fqn.size()-1);
        }
        std::string getMSVCMangledName(const clang::FunctionDecl* funcDecl)
        {
            std::unique_ptr<clang::MangleContext> mangleContext(context->createMangleContext());

            if (!mangleContext->shouldMangleDeclName(funcDecl))
            {   // C APIs (like DllMain and main) trigger this path.
                std::string display = funcDecl->getType().getAsString(printPolicy);
                return display.replace(display.find('('), 0, funcDecl->getNameAsString());
            }

            std::string out;
            llvm::raw_string_ostream oStream(out);

            if (auto* ctorDecl = llvm::dyn_cast<clang::CXXConstructorDecl>(funcDecl))        // Constructors
                mangleContext->mangleName(clang::GlobalDecl(ctorDecl, clang::Ctor_Complete), oStream);
            else if (auto* dtorDecl = llvm::dyn_cast<clang::CXXDestructorDecl>(funcDecl))    // Destructors
                mangleContext->mangleName(clang::GlobalDecl(dtorDecl, clang::Dtor_Base /* N.B: not Dtor_Complete */), oStream);
            else
                mangleContext->mangleCXXName(funcDecl, oStream);                             // Ordinary C++ functions
            return out;
        }

        std::string ConstructRecordSignature(CXXRecordDecl* recordDecl)
        {
            std::string out;

            // struct/class/union keyword + name
            out += recordDecl->getKindName().str() + " ";
            out += recordDecl->getQualifiedNameAsString();
            out += " {\n";

            // data members
            for (const FieldDecl* field : recordDecl->fields())
            {
                switch (field->getAccess())
                {
                case AS_public:    out += "public:    "; break;
                case AS_protected: out += "protected: "; break;
                case AS_private:   out += "private:   "; break;
                default:           out += "           "; break;
                }

                out += field->getType().getCanonicalType().getAsString(printPolicy) + " ";
                out += field->getNameAsString();

                // bitfield
                if (field->isBitField())
                {
                    std::string bitWidth;
                    llvm::raw_string_ostream os(bitWidth);
                    field->getBitWidth()->printPretty(os, nullptr, printPolicy);
                    os.flush();
                    out += " : " + bitWidth;
                }
                out += ";\n";
            }

            // methods
            for (const CXXMethodDecl* method : recordDecl->methods())
            {
                if (method->isImplicit())
                    continue;

                out += ConstructFunctionSignature(method, false);
                out += ";\n";
            }

            out += "}";
            return out;
        }

    };

    class VisitorConsumer : public ASTConsumer
    {
        TheVisitor visitor;
    public:
        VisitorConsumer(ASTContext* context, AllMaps& maps, const std::string& TU) : visitor(context, maps, TU) {}
        void HandleTranslationUnit(ASTContext& context) override
        {
            visitor.TraverseDecl(context.getTranslationUnitDecl());
        }
    };

    class VisitorAction : public ASTFrontendAction
    {
        AllMaps& maps;
    public:
        explicit VisitorAction(AllMaps& maps) : maps(maps) {}
        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& CI, llvm::StringRef InFile) override
        {
            return std::make_unique<VisitorConsumer>(&CI.getASTContext(), maps, InFile.str());
        }
    };

    class VisitorActionFactory : public clang::tooling::FrontendActionFactory
    {
        AllMaps& maps;
    public:
        explicit VisitorActionFactory(AllMaps& maps) : maps(maps) {}
        std::unique_ptr<clang::FrontendAction> create() override { return std::make_unique<VisitorAction>(maps); }
    };
}