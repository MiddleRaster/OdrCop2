#include "stdafx.h"

#include "ASTVisitor.h"

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2)
    {
        std::wcout << L"Usage: OdrCop2 <folder to compile_commands.json> ";
        return -1;
    }

    std::filesystem::path jsonFolder = argv[1];
    std::error_code ec;
    if (!std::filesystem::exists(jsonFolder, ec) || !std::filesystem::is_directory(jsonFolder, ec))
    {
        std::wcerr << L"Path not found or not a directory: " << jsonFolder.wstring() << L'\n';
        return -1;
    }

    std::string error;
    auto compilations = clang::tooling::CompilationDatabase::loadFromDirectory(jsonFolder.string(), error);
    std::vector<std::string> files = compilations->getAllFiles();
    clang::tooling::ClangTool tool(*compilations, files);

    std::vector<OdrCop2::FunctionInfo> functionInfos;
    OdrCop2::VisitorActionFactory factory(functionInfos);
    tool.run(&factory);

    std::wcout << L"functions found:\n";
    for (const auto& fi : functionInfos)
    {
        std::cout << fi.TU << ": " << fi.returnType.fullyQualifiedReturnValueTypeName << " " << fi.fullyQualifiedName;
        
        if (fi.args.size() == 0)
            std::cout << "();\n";
        else {
            for(size_t i=0; i<fi.args.size(); ++i)
            {
                if (i == 0)
                    std::cout << "(";
                std::cout << fi.args[i];
                if (i+1 != fi.args.size())
                    std::cout << ", ";
            }
            std::cout << ");\n";
        }
    }

    return 0;
}
