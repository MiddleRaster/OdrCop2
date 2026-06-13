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
        const bool isInline;
        // TODO:
        const bool isContexpr;
        const bool isConsteval;
        const bool isNoexcept;
        const bool isThrow;
        const bool isExternC; // ⁠extern "C"⁠ vs C++ Linkage // I don’t see how this one can work, as the extern “C” one doesn’t get mangled
        const std::vector<std::string> defaultArgValues;
        const std::vector<std::string> defaultTemplateArgTypes;
        bool operator==(const FunctionInfo& other) const
        {   // I prepended "inline", but if the others cannot be pre/appended, check for them here
            return fullyQualified == other.fullyQualified;
        }
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
    struct AllMaps
    {
        std::map<std::string,std::vector<FunctionInfo>> functionMap;
        std::map<std::string,std::vector<    EnumInfo>>     enumMap;
        std::map<std::string,std::vector< TypedefInfo>>  typedefMap;
        // udt
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
                auto mangledName = getMSVCMangledName(funcDecl, *context);

                std::string fqn;
                char buf[1024*16] = {'\0'};
                DWORD result = UnDecorateSymbolName(mangledName.c_str(), buf, static_cast<DWORD>(sizeof(buf)), UNDNAME_COMPLETE);
                if (result)
                    fqn = std::string(buf);
                else
                    fqn = mangledName;

                const FunctionTemplateDecl* funcTmpltDecl = funcDecl->getDescribedFunctionTemplate();
                if (funcTmpltDecl != nullptr && !funcDecl->getTemplateSpecializationInfo())
                {   // only uninstantiated templates go through this path
                    std::string out;
                    llvm::raw_string_ostream oStream(out);
                    funcTmpltDecl->print(oStream, printPolicy);
                    fqn = out;
                }

                bool isInlined = funcDecl->isInlined();
                if  (isInlined)
                {
                    struct Insert
                    {
                        static std::string Inline(const std::string& sig)
                        {
                            static const std::string accessSpecs[] =
                            {
                                "public: ",
                                "protected: ",
                                "private: ",
                            };
                            for (const auto& prefix : accessSpecs)
                                if (sig.starts_with(prefix))
                                    return std::string(prefix) + "inline " + sig.substr(prefix.size());

                            return "inline " + sig;
                        }
                    };
                    fqn = Insert::Inline(fqn);
                }

                maps.functionMap[mangledName].push_back({TU, getMSVCMangledName(funcDecl, *context), fqn, isInlined});
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
        std::string getMSVCMangledName(const clang::FunctionDecl* funcDecl, clang::ASTContext& Ctx)
        {
            std::unique_ptr<clang::MangleContext> mangleContext(Ctx.createMangleContext());

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