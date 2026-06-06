#pragma once

#include <clang\AST\AST.h>
#include <clang\AST\RecursiveASTVisitor.h>
#include <clang\Frontend\FrontendActions.h>
#include <clang\Frontend\CompilerInstance.h>
#include <clang\Tooling\Tooling.h>
using namespace clang;

#include <vector>


namespace OdrCop2
{
    struct FunctionInfo
    {
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
        ASTContext* context;
        std::vector<FunctionInfo>& functionInfos;

    public:
        FunctionVisitor(ASTContext* context, std::vector<FunctionInfo>& functionInfos)
            : context      (context)
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

                functionInfos.push_back({funcDecl->getQualifiedNameAsString(),
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
        VisitorConsumer(ASTContext* context, std::vector<FunctionInfo>& functionInfos) : visitor(context, functionInfos) {}
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
            return std::make_unique<VisitorConsumer>(&CI.getASTContext(), functionInfos);
        }
        bool BeginInvocation(CompilerInstance& CI) override
        {   // don't allow Clang/LLVM to write to screen when there's a compiler error
            struct SilentDiagConsumer : public clang::DiagnosticConsumer {
                void HandleDiagnostic(clang::DiagnosticsEngine::Level level, const clang::Diagnostic& info) override {
                    DiagnosticConsumer::HandleDiagnostic(level, info); // increments error count but don't print anything
                }
            };
            CI.getDiagnosticOpts().ShowCarets = false;
            CI.createDiagnostics(new SilentDiagConsumer(), true);
            return true;
        }
    };
}