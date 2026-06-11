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

namespace OdrCop2
{
    struct FunctionInfo
    {
        const std::string TU;

        struct QualifiedAndCanonical
        {
            const std::string fullyQualifiedReturnValueTypeName;
            const std::string  canonicalizedReturnValuetypeName;
        };
        const std::string fullyQualifiedName;
        const std::string mangledName;
        const QualifiedAndCanonical returnType;

        // add more: args, static, inline, method/function, friend, noexcept, default, etc.
    };

    class FunctionVisitor : public RecursiveASTVisitor<FunctionVisitor>
    {
        const std::string TU;
        ASTContext* context;
        std::vector<FunctionInfo>& functionInfos;
    public:
        FunctionVisitor(ASTContext* context, std::vector<FunctionInfo>& functionInfos, const std::string& TU)
            : TU           (TU)
            , context      (context)
            , functionInfos(functionInfos)
        {}
        bool VisitFunctionDecl(FunctionDecl* funcDecl)
        {
            if (context->getSourceManager().isInSystemHeader(funcDecl->getLocation()))
                return true; // Skip anything not in the main file or a user header

            if (funcDecl->isThisDeclarationADefinition())
            {
                functionInfos.push_back({TU,
                                         funcDecl->getQualifiedNameAsString(),
                                         getMSVCMangledName(funcDecl, *context),
                                         {funcDecl->getReturnType().getAsString(), funcDecl->getReturnType().getCanonicalType().getAsString()}
                                        });
            }
            return true;
        }
    private:
        static std::string getMSVCMangledName(const clang::FunctionDecl* funcDecl, clang::ASTContext& Ctx)
        {
            std::unique_ptr<clang::MangleContext> mangleContext(Ctx.createMangleContext());

            if (!mangleContext->shouldMangleDeclName(funcDecl))
                return funcDecl->getNameAsString();

            std::string out;
            llvm::raw_string_ostream oStream(out);

            if (auto* ctorDecl = llvm::dyn_cast<clang::CXXConstructorDecl>(funcDecl))        // Constructors
                mangleContext->mangleName(clang::GlobalDecl(ctorDecl, clang::Ctor_Complete), oStream);
            else if (auto* dtorDecl = llvm::dyn_cast<clang::CXXDestructorDecl>(funcDecl))    // Destructors
                mangleContext->mangleName(clang::GlobalDecl(dtorDecl, clang::Dtor_Complete), oStream);
            else
                mangleContext->mangleCXXName(funcDecl, oStream);                             // Ordinary C++ functions
            return out;
        }
    };

    class VisitorConsumer : public ASTConsumer
    {
        FunctionVisitor visitor;
    public:
        VisitorConsumer(ASTContext* context, std::vector<FunctionInfo>& functionInfos, const std::string& TU) : visitor(context, functionInfos, TU) {}
        void HandleTranslationUnit(ASTContext& context) override
        {
            visitor.TraverseDecl(context.getTranslationUnitDecl());
        }
    };

    class VisitorAction : public ASTFrontendAction
    {
        std::vector<FunctionInfo>& functionInfos;
    public:
        explicit VisitorAction(std::vector<FunctionInfo>& functionInfos) : functionInfos(functionInfos) {}
        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& CI, llvm::StringRef InFile) override
        {
            return std::make_unique<VisitorConsumer>(&CI.getASTContext(), functionInfos, InFile.str());
        }
    };
    
    class VisitorActionFactory : public clang::tooling::FrontendActionFactory
    {
        std::vector<OdrCop2::FunctionInfo>& functionInfos;
    public:
        explicit VisitorActionFactory(std::vector<OdrCop2::FunctionInfo>& functionInfos) : functionInfos(functionInfos) {}
        std::unique_ptr<clang::FrontendAction> create() override { return std::make_unique<OdrCop2::VisitorAction>(functionInfos); }
    };
}