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
#include <clang\AST\RecordLayout.h>
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
            if (funcDecl->isImplicit())
                return true;

            if (context->getSourceManager().isInSystemHeader(funcDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            if (funcDecl->getStorageClass() == clang::SC_Static || funcDecl->isInAnonymousNamespace())
                return true; // if the function has internal-linkage, skip it

            if (funcDecl->isThisDeclarationADefinition())
            {
                auto mangledName = getMSVCMangledName(funcDecl);
                maps.functionMap[mangledName].push_back({TU, mangledName, ConstructFunctionSignature(funcDecl)});
            }
            return true;
        }

        bool shouldVisitTemplateInstantiations() const { return true; }
        bool shouldVisitImplicitCode          () const { return true; }
        bool VisitCXXRecordDecl(CXXRecordDecl* recordDecl)
        {
            if (context->getSourceManager().isInSystemHeader(recordDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            if (recordDecl->isInAnonymousNamespace())
                return true; // if the class/struc/union has internal-linkage, skip it

            if (recordDecl->isThisDeclarationADefinition())
            {
                std::string key = recordDecl->getQualifiedNameAsString();
                     if (recordDecl->getDescribedClassTemplate())                            key += "<>";
                else if (auto* CTSD = dyn_cast<ClassTemplateSpecializationDecl>(recordDecl)) key += TemplateArgsToString(CTSD->getTemplateArgs());

                maps.udtMap[key].push_back({TU, ConstructRecordSignature(recordDecl)});
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
            if (typedefDecl->isImplicit())
                return true;

            if (context->getSourceManager().isInSystemHeader(typedefDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            std::string aliasName    = typedefDecl->getQualifiedNameAsString();
            std::string resolvedType = typedefDecl->getUnderlyingType().getCanonicalType().getAsString(printPolicy);
            std::string fqtd         = aliasName + " = " + resolvedType;
            maps.typedefMap[aliasName].push_back({TU, fqtd});
            return true;
        }

    private:
        std::string TemplateArgsToString(const clang::TemplateArgumentList& args)
        {
            std::string out;
            out += "<";

            for (unsigned i = 0; i < args.size(); ++i)
            {
                if (i > 0)
                    out += ", ";

                const clang::TemplateArgument& arg = args[i];
                switch (arg.getKind())
                {
                case clang::TemplateArgument::Type:              out += arg.getAsType().getAsString();                                                                break;
                case clang::TemplateArgument::Integral:          out += llvm::toString(arg.getAsIntegral(), 10);                                                      break;
                case clang::TemplateArgument::NullPtr:           out += "nullptr";                                                                                    break;
                case clang::TemplateArgument::Declaration:       out += arg.getAsDecl()->getQualifiedNameAsString();                                                  break;
                case clang::TemplateArgument::Null:              out += "null";                                                                                       break;
                case clang::TemplateArgument::Template:          out += arg.getAsTemplate().getAsTemplateDecl()->getQualifiedNameAsString();                          break;
                case clang::TemplateArgument::TemplateExpansion: out += arg.getAsTemplateOrTemplatePattern().getAsTemplateDecl()->getQualifiedNameAsString() + "..."; break;
                case clang::TemplateArgument::Expression:
                {
                    std::string s;
                    llvm::raw_string_ostream os(s);
                    arg.getAsExpr()->printPretty(os, nullptr, context->getPrintingPolicy());
                    out += os.str();
                    break;
                }
                case clang::TemplateArgument::Pack:
                {
                    out += "{";
                    auto pack = arg.pack_elements();
                    for (unsigned j = 0; j < pack.size(); ++j) {
                        if (j > 0)
                            out += ", ";

                        // Inline handling for pack elements
                        const clang::TemplateArgument& pe = pack[j];
                        switch (pe.getKind())
                        {
                        case clang::TemplateArgument::Type:              out += pe.getAsType().getAsString();                                                                break;
                        case clang::TemplateArgument::Integral:          out += llvm::toString(pe.getAsIntegral(), 10);                                                      break;
                        case clang::TemplateArgument::NullPtr:           out += "nullptr";                                                                                   break;
                        case clang::TemplateArgument::Declaration:       out += pe.getAsDecl()->getQualifiedNameAsString();                                                  break;
                        case clang::TemplateArgument::Null:              out += "null";                                                                                      break;
                        case clang::TemplateArgument::Template:          out += pe.getAsTemplate().getAsTemplateDecl()->getQualifiedNameAsString();                          break;
                        case clang::TemplateArgument::TemplateExpansion: out += pe.getAsTemplateOrTemplatePattern().getAsTemplateDecl()->getQualifiedNameAsString() + "..."; break;
                        case clang::TemplateArgument::Expression:
                        {
                            std::string s;
                            llvm::raw_string_ostream os(s);
                            pe.getAsExpr()->printPretty(os, nullptr, context->getPrintingPolicy());
                            out += os.str();
                            break;
                        }
                        default:
                            out += "?"; break;
                        }
                    }
                    out += "}";
                    break;
                }
                default: out += "?"; break;
                }
            }
            out += ">";
            return out;
        }

        std::string ConstructFunctionSignature(const clang::FunctionDecl* funcDecl, bool wantFullyQualifiedMethodName=true)
        {
            std::string fqn;


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

                fqn += ConstructAttribute(attr);
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
            const auto* parentRecord = clang::dyn_cast<clang::CXXRecordDecl>(funcDecl->getParent());
            bool   isTemplateContext = funcDecl->getDescribedFunctionTemplate() != nullptr || (parentRecord && parentRecord->getDescribedClassTemplate() != nullptr);
            fqn += isTemplateContext ? funcDecl->getReturnType().getAsString(printPolicy)
                                     : funcDecl->getReturnType().getCanonicalType().getAsString(printPolicy);
            fqn += " ";

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
                    out += "<";
                    llvm::raw_string_ostream  os2(out);
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

                fqn += typeName;

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

                fqn += ConstructAttribute(attr);
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

            // if it's a ctor, get any initializers
            if (const auto* ctor = clang::dyn_cast<clang::CXXConstructorDecl>(funcDecl))
            {
                if (ctor->init_begin() != ctor->init_end())
                {
                    fqn += ": ";
                    bool first = true;
                    for (const clang::CXXCtorInitializer* init : ctor->inits())
                    {
                        if (!first)
                            fqn += ", ";
                        first = false;

                        if (init->isBaseInitializer())
                            fqn += clang::QualType(init->getBaseClass(), 0).getAsString(printPolicy);
                        else if (init->isMemberInitializer())
                            fqn += init->getMember()->getNameAsString();
                        else if (init->isIndirectMemberInitializer())
                            fqn += init->getIndirectMember()->getNameAsString();
                     // else if (init->isDelegatingInitializer())
                     //     ;

                        std::string argStr;
                        llvm::raw_string_ostream os(argStr);
                        init->getInit()->printPretty(os, nullptr, printPolicy);
                        os.flush();
                        fqn += "(" + argStr + ")";
                    }
                    fqn += " ";
                }
            }

            if (!(funcDecl->hasBody() && funcDecl->getBody()))
            { // no body:  end prototype with ';'
                if (fqn.ends_with(' '))
                    fqn = fqn.substr(0, fqn.size()-1);
                fqn += ";";
            }
            else
            { // append body
                std::string body;
                llvm::raw_string_ostream os(body);
                funcDecl->getBody()->printPretty(os, nullptr, printPolicy);
                os.flush();

                // post-process body

                // collapse to "{}" if there's no real content
                bool allWhitespace = true;
                for (char c : body) {
                    if ((c == '{') ||
                        (c == '}'))
                        continue;
                    if (!std::isspace(static_cast<unsigned char>(c))) {
                        allWhitespace = false;
                        break;
                    }
                }
                if (allWhitespace)
                    body = "{}";

                // strip "this->"
                const std::string target = "this->";
                size_t pos = 0;
                while ((pos = body.find(target, pos)) != std::string::npos)
                    body.erase(pos, target.length());

                // if it's a one-liner, remove all "\n   "
                int  semicolons = 0;
                for (char c : body) {
                    if (c == ';')
                        ++semicolons;
                }
                if (semicolons < 2) {
                    size_t pos = 0;
                    while ((pos = body.find("\n", pos)) != std::string::npos)
                        body.replace(pos, 1, " ");

                    pos = 0;
                    while ((pos = body.find("  ", pos)) != std::string::npos)
                        body.replace(pos, 2, " ");

                    while (body.ends_with(' '))
                        body = body.substr(0, body.size()-1); // strip off last ' '

                    body += "\n";
                }

                fqn += body;
                fqn = fqn.substr(0, fqn.size()-1); // strip off last '\n'
            }

            if (fqn.ends_with(' '))
                fqn = fqn.substr(0, fqn.size()-1);
            return fqn;
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

        std::string ConstructRecordSignature(const CXXRecordDecl* recordDecl)
        {
            std::string out;

            // template
            const clang::ClassTemplateDecl* ctd = recordDecl->getDescribedClassTemplate();
            if (ctd)
            {
                bool first = true;
                out        = "template<";
                const clang::TemplateParameterList* params = ctd->getTemplateParameters();
                for (const clang::NamedDecl* param : *params)
                {
                    if (!first)
                        out += ", ";
                    first = false;

                    if (const auto* ttp = clang::dyn_cast<clang::TemplateTypeParmDecl>(param))
                    {
                        out += ttp->wasDeclaredWithTypename() ? "typename" : "class";
                        if (!ttp->getName().empty())
                            out += " " + ttp->getName().str();
                    }
                    else if (const auto* nttp = clang::dyn_cast<clang::NonTypeTemplateParmDecl>(param))
                    {
                        out += nttp->getType().getAsString(printPolicy);
                        if (!nttp->getName().empty())
                            out += " " + nttp->getName().str();
                    }
                    else if (const auto* ttp2 = clang::dyn_cast<clang::TemplateTemplateParmDecl>(param))
                    {
                        out += "template<";
                        const clang::TemplateParameterList* innerParams = ttp2->getTemplateParameters();
                        bool first2 = true;
                        for (const clang::NamedDecl* innerParam : *innerParams)
                        {
                            if (!first2)
                                out += ", ";
                            first2 = false;

                            if (const auto* innerTtp = clang::dyn_cast<clang::TemplateTypeParmDecl>(innerParam))
                                out += innerTtp->wasDeclaredWithTypename() ? "typename" : "class";
                            else if (const auto* innerNttp = clang::dyn_cast<clang::NonTypeTemplateParmDecl>(innerParam))
                            {
                                out += innerNttp->getType().getAsString(printPolicy);
                                if (!innerNttp->getName().empty())
                                    out += " " + innerNttp->getName().str();
                            }
                        }
                        out += "> class";
                        if (!ttp2->getName().empty())
                            out += " " + ttp2->getName().str();
                    }
                }
                out += "> ";
            } else if (auto* CTSD = dyn_cast<ClassTemplateSpecializationDecl>(recordDecl))
                out += "template<> ";
            
            // struct/class/union keyword
            out += recordDecl->getKindName().str() + " ";

            // alignas/[[attributes]]/__declspecs
            out += ConstructAttributes(recordDecl);

            // name (if not nameless)
            if (!recordDecl->isAnonymousStructOrUnion())
                out += recordDecl->getQualifiedNameAsString(); // note: no space until after any <> stuff

            // if it's a template instantiation, add <arg> 
            if (auto* CTSD = dyn_cast<ClassTemplateSpecializationDecl>(recordDecl))
                out += TemplateArgsToString(CTSD->getTemplateArgs());

            out += " ";

            // base classes
            bool firstBase = true;
            for (const clang::CXXBaseSpecifier& base : recordDecl->bases())
            {
                if (firstBase)
                    out += ": ";
                else
                    out += ", ";
                firstBase = false;

                switch (base.getAccessSpecifier()) {
                case clang::AS_public:    out += "public ";    break;
                case clang::AS_protected: out += "protected "; break;
                case clang::AS_private:   out += "private ";   break;
                case clang::AS_none:      out += " ";          break; // depends on context
                }
                if (base.isVirtual())
                    out += "virtual ";

                out += base.getType().getAsString(printPolicy);
            }
            if (firstBase == false)
                out += " ";

            // final
            if (true == recordDecl->hasAttr<FinalAttr>())
                out += "final ";

            // sizeof as comment
            if (recordDecl->isCompleteDefinition() && !recordDecl->isDependentType())
                out += "{ // sizeof=" + std::to_string(context->getASTRecordLayout(recordDecl).getSize().getQuantity()) + "\n";
            else
                out += "{\n";

            // data-members and methods
            for (const clang::Decl* decl : recordDecl->decls())
            {
                if (clang::isa<clang::AccessSpecDecl>(decl))
                {
                    const auto* access = clang::dyn_cast<clang::AccessSpecDecl>(decl);
                    switch (access->getAccess())
                    {
                    case AS_public:    out += "public:\n";    break;
                    case AS_protected: out += "protected\n:"; break;
                    case AS_private:   out += "private:\n";   break;
                    default:                                  break;
                    }
                    continue;
                }
                if (const auto* field = clang::dyn_cast<clang::FieldDecl>(decl))
                {   // data members

                    if (field->isAnonymousStructOrUnion())
                        continue; // nameless unions/struct never have a variable name

                    out += "   ";

                    // attributes on data-members
                    out += ConstructAttributes(decl);

                    { // when a field is defined in an anonymous namespace, include the full definition here with the field.
                        const clang::Type* type = field->getType().getCanonicalType().getTypePtr();
                        const auto*  recordType = clang::dyn_cast<clang::RecordType>(type);
                        if (recordType && recordType->getDecl()->isInAnonymousNamespace())
                        {
                            clang::Qualifiers quals = field->getType().getQualifiers();
                            if (quals.hasConst())    out += "const ";
                            if (quals.hasVolatile()) out += "volatile ";

                            out += ConstructAttributes(field);
                            out += EmitAnonymousNamespaceRecord(recordType->getDecl());
                            out += " ";
                            out += field->getNameAsString();
                        }
                        else
                        { // field must be done this way to handle array fields as well.
                            std::string fieldStr;
                            llvm::raw_string_ostream os(fieldStr);
                            field->getType().print(os, printPolicy, field->getNameAsString());
                            os.flush();
                            out += fieldStr;
                        }
                    }

                    if (field->isBitField())
                    {
                        std::string bitWidth;
                        llvm::raw_string_ostream os(bitWidth);
                        field->getBitWidth()->printPretty(os, nullptr, printPolicy);
                        os.flush();
                        out += " : " + bitWidth;
                    }

                    if (field->hasInClassInitializer())
                    {
                        const Expr*      expr = field->getInClassInitializer();
                        llvm::StringRef  text = clang::Lexer::getSourceText(CharSourceRange::getTokenRange(expr->getSourceRange()), context->getSourceManager(), context->getLangOpts());
                        std::string      init = text.str();
                        if ((init.starts_with("{")) || init.starts_with("("))
                            out +=       init;
                        else
                            out += "=" + init;
                    }

                    out += ";\n";
                    continue;
                }
                if (const auto* var = clang::dyn_cast<clang::VarDecl>(decl))
                {   // static data members  (VarDecl inside a record == static member)
                    out += "   ";

                    // attributes on static data-members
                    out += ConstructAttributes(decl);

                    if (var->isConstexpr())
                        out += "constexpr ";
                    else if (var->isInline())
                        out += "inline ";

                    { // static field: use same type+name print trick as non-static
                        std::string fieldStr;
                        llvm::raw_string_ostream os(fieldStr);
                        var->getType().print(os, printPolicy, var->getNameAsString());
                        os.flush();
                        out += "static " + fieldStr;
                    }

                    if (var->hasInit())
                    {
                        const Expr* expr = var->getInit();
                        llvm::StringRef  text = clang::Lexer::getSourceText(CharSourceRange::getTokenRange(expr->getSourceRange()), context->getSourceManager(), context->getLangOpts());
                        std::string      init = text.str();
                        if ((init.starts_with("{")) || init.starts_with("("))
                            out += init;
                        else
                            out += "=" + init;
                    }
                    out += ";\n";
                    continue;
                }
                if (const auto* method = clang::dyn_cast<clang::CXXMethodDecl>(decl))
                {   // methods
                    if (method->isImplicit())
                        continue;

                    // recurse but indent
                    std::istringstream iss(ConstructFunctionSignature(method, false));
                    for (std::string line; std::getline(iss, line); )
                        out += "   " + line + "\n";

                    continue;
                }
                if (const auto* nested = clang::dyn_cast<clang::CXXRecordDecl>(decl))
                {
                    if (nested->isInjectedClassName())
                        continue;

                    // recurse but indent
                    std::istringstream iss(ConstructRecordSignature(nested));
                    for (std::string line; std::getline(iss, line); )
                        out += "   " + line + "\n";

                    continue;
                }
                if (clang::isa<clang::IndirectFieldDecl>(decl))
                    continue; // these are Clang's mechanism for exposing the members of an anonymous struct/union as if they were directly accessible in the enclosing class.

                { // unhandled
                    out += "unhandled clang::Decl ";
                    out += decl->getDeclKindName();
                    out += " ";
                    if (const auto* named = clang::dyn_cast<clang::NamedDecl>(decl))
                        out += named->getNameAsString();
                    out += "\n";
                }
            }

            out += "};";
            return out;
        }
        std::string EmitAnonymousNamespaceRecord(const clang::RecordDecl* innerDecl)
        {
            std::string out;

            out += innerDecl->getKindName().str() + " ";
            out += innerDecl->getQualifiedNameAsString();
            out += " { ";
            for (const clang::FieldDecl* innerField : innerDecl->fields())
            {
                const clang::Type* innerType = innerField->getType().getCanonicalType().getTypePtr();
                const auto*  innerRecordType = clang::dyn_cast<clang::RecordType>(innerType);
                if (innerRecordType && innerRecordType->getDecl()->isInAnonymousNamespace())
                {
                    clang::Qualifiers quals = innerField->getType().getQualifiers();
                    if (quals.hasConst())    out += "const ";
                    if (quals.hasVolatile()) out += "volatile ";

                    out += ConstructAttributes(innerField);
                    out += EmitAnonymousNamespaceRecord(innerRecordType->getDecl());
                    out += " ";
                    out += innerField->getNameAsString();
                }
                else
                {
                    std::string              innerFieldStr;
                    llvm::raw_string_ostream os(innerFieldStr);
                    innerField->getType().print(os, printPolicy, innerField->getNameAsString());
                    os.flush();
                    out += innerFieldStr;
                }

                if (innerField->isBitField())
                {
                    std::string              bitWidth;
                    llvm::raw_string_ostream os(bitWidth);
                    innerField->getBitWidth()->printPretty(os, nullptr, printPolicy);
                    os.flush();
                    out += " : " + bitWidth;
                }

                if (innerField->hasInClassInitializer())
                {
                    const Expr* expr = innerField->getInClassInitializer();
                    llvm::StringRef text = clang::Lexer::getSourceText(CharSourceRange::getTokenRange(expr->getSourceRange()), context->getSourceManager(), context->getLangOpts());
                    std::string     init = text.str();
                    if ((init.starts_with("{")) || init.starts_with("("))
                        out += init;
                    else
                        out += "=" + init;
                }

                out += "; ";
            }
            out += "}";
            return out;
        }
        std::string ConstructAttribute(const Attr* attr)
        {
            std::string out;

            if (const auto* alignedAttr = clang::dyn_cast<clang::AlignedAttr>(attr))
            {
                if (alignedAttr->isAlignmentExpr())
                {
                    const clang::Expr* expr = alignedAttr->getAlignmentExpr();
                    if (expr && expr->isIntegerConstantExpr(*context))
                    {
                        auto optInt = expr->getIntegerConstantExpr(*context);
                        if (optInt.has_value())
                        {
                            out += "alignas(" + std::to_string(optInt.value().getExtValue()) + ") ";
                            return out;
                        }
                    }
                }
                // alignment specified as a type: alignas(SomeType)
                out += "alignas(" + alignedAttr->getAlignmentType()->getType().getAsString() + ") ";
                return out;
            }
            if (const auto* nodiscard = clang::dyn_cast<clang::WarnUnusedResultAttr>(attr))
            {
                const llvm::StringRef msg = nodiscard->getMessage();
                out += msg.empty() ? "[[nodiscard]] " : ("[[nodiscard(\"" + msg.str() + "\")]] ");
                return out;
            }
            if (const auto* deprecated = clang::dyn_cast<clang::DeprecatedAttr>(attr))
            {
                const llvm::StringRef msg = deprecated->getMessage();
                out += msg.empty() ? "[[deprecated]] " : ("[[deprecated(\"" + msg.str() + "\")]] ");
                return out;
            }

            // filter out final (maybe more?)
            if (const auto* Final = clang::dyn_cast<clang::FinalAttr>(attr))
                return out;

            // attributes other than alignas, nodiscard and deprecated, nor final
            std::string raw;
            llvm::raw_string_ostream os(raw);
            attr->printPretty(os, printPolicy);
            os.flush();

            // strip off ("")
            constexpr std::string_view empty_parens = "(\"\")";
            if (auto pos = raw.find(empty_parens); pos != std::string::npos)
                raw.replace(pos, empty_parens.size(), "");
            out += raw + " ";

            return out;
        }
        std::string ConstructAttributes(const Decl* decl)
        {
            std::string out;
            for(const auto* attr : decl->attrs())
                out += ConstructAttribute(attr);
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


    struct OdrViolationReporter
    {
        template<typename Out> static int ReportOdrViolations(const AllMaps& maps, Out&& out)
        {
            return OdrCop2::OdrViolationReporter::ReportOdrViolations(maps.    enumMap, out)
                 + OdrCop2::OdrViolationReporter::ReportOdrViolations(maps.functionMap, out)
                 + OdrCop2::OdrViolationReporter::ReportOdrViolations(maps. typedefMap, out)
                 + OdrCop2::OdrViolationReporter::ReportOdrViolations(maps.     udtMap, out);
        }

        template<typename T, typename Out> static int ReportOdrViolations(const std::map<std::string,T>& map, Out&& out)
        {
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
                out << "\nODR VIOLATION: " << name << '\n';

                std::vector<bool> printed(items.size(), false);
                for (size_t i=0; i<items.size(); ++i)
                {
                    if (printed[i])
                        continue;

                    out << "[" << items[i].TU << "]\n";
                    out        << items[i].fullyQualified << '\n';
                    printed[i] = true;

                    for (size_t j=i+1; j<items.size(); ++j)
                    {
                        if (!printed[j] && (items[i] == items[j]))
                        {
                            out << "[" << items[j].TU << "] - same as above\n";
                            printed[j] = true;
                        }
                    }
                }
            }
            return violationCount;
        }
    };
}