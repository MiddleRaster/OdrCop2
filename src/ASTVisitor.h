#pragma once

#include <clang\AST\AST.h>
#include <clang\AST\RecursiveASTVisitor.h>
#include <clang\Frontend\FrontendActions.h>
#include <clang\Frontend\CompilerInstance.h>
#include <clang\Tooling\Tooling.h>
#include <clang\Tooling\CompilationDatabase.h>
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
            if (funcDecl->isThisDeclarationADefinition())
            {
                //  funcDecl->getReturnType() gives you a QualType.From that you can call
                //  .getAsString() for a human - readable form, or
                //  .getCanonicalType().getAsString() 
                //  to get the fully resolved type without typedefs.For ODR purposes you'll want canonical types.

                functionInfos.push_back({TU,
                                         funcDecl->getQualifiedNameAsString(),
                                            {funcDecl->getReturnType().getAsString(), funcDecl->getReturnType().getCanonicalType().getAsString()}
                                        });
            }
            return true;
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