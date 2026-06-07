#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "ASTVisitor.h"

// Clang libs
#pragma comment(lib, "clangTooling.lib")
#pragma comment(lib, "clangToolingCore.lib")
#pragma comment(lib, "clangFrontend.lib")
#pragma comment(lib, "clangDriver.lib")
#pragma comment(lib, "clangSerialization.lib")
#pragma comment(lib, "clangParse.lib")
#pragma comment(lib, "clangSema.lib")
#pragma comment(lib, "clangAnalysis.lib")
#pragma comment(lib, "clangAST.lib")
#pragma comment(lib, "clangASTMatchers.lib")
#pragma comment(lib, "clangEdit.lib")
#pragma comment(lib, "clangLex.lib")
#pragma comment(lib, "clangBasic.lib")
#pragma comment(lib, "clangRewrite.lib")
#pragma comment(lib, "clangSupport.lib")

// LLVM libs
#pragma comment(lib, "LLVMSupport.lib")
#pragma comment(lib, "LLVMCore.lib")
#pragma comment(lib, "LLVMBinaryFormat.lib")
#pragma comment(lib, "LLVMBitReader.lib")
#pragma comment(lib, "LLVMBitstreamReader.lib")
#pragma comment(lib, "LLVMMC.lib")
#pragma comment(lib, "LLVMMCParser.lib")
#pragma comment(lib, "LLVMOption.lib")
#pragma comment(lib, "LLVMProfileData.lib")
#pragma comment(lib, "LLVMRemarks.lib")
#pragma comment(lib, "LLVMFrontendOpenMP.lib")
#pragma comment(lib, "LLVMTargetParser.lib")
#pragma comment(lib, "LLVMTextAPI.lib")
#pragma comment(lib, "LLVMWindowsDriver.lib")
#pragma comment(lib, "LLVMWindowsManifest.lib")

// X86 backend
#pragma comment(lib, "LLVMX86CodeGen.lib")
#pragma comment(lib, "LLVMX86Desc.lib")
#pragma comment(lib, "LLVMX86Info.lib")

#pragma comment(lib, "clangAPINotes.lib")
#pragma comment(lib, "clangDependencyScanning.lib")
//#pragma comment(lib, "clangHLSL.lib")
//#pragma comment(lib, "clangSSAF.lib")

#pragma comment(lib, "LLVMOption.lib")
#pragma comment(lib, "LLVMObject.lib")
//#pragma comment(lib, "LLVMArchive.lib")
#pragma comment(lib, "LLVMDemangle.lib")
#pragma comment(lib, "LLVMXRay.lib")

// Codex's idea
#pragma comment(lib, "clangIndex.lib")
#pragma comment(lib, "clangBasic.lib")
#pragma comment(lib, "clangOptions.lib")
#pragma comment(lib, "clangAnalysisLifetimeSafety.lib")
#pragma comment(lib, "clangScalableStaticAnalysisFrameworkCore.lib")
#pragma comment(lib, "clangScalableStaticAnalysisFrameworkAnalyses.lib")
#pragma comment(lib, "clangScalableStaticAnalysisFrameworkFrontend.lib")
#pragma comment(lib, "LLVMPlugins.lib")
#pragma comment(lib, "LLVMFrontendHLSL.lib")
#pragma comment(lib, "LLVMFrontendOffloading.lib")
#pragma comment(lib, "LLVMDebugInfoDWARF.lib")
#pragma comment(lib, "LLVMIRReader.lib")
#pragma comment(lib, "LLVMAnalysis.lib")
#pragma comment(lib, "LLVMScalarOpts.lib")
#pragma comment(lib, "LLVMTransformUtils.lib")
#pragma comment(lib, "LLVMipo.lib")
#pragma comment(lib, "LLVMTarget.lib")
#pragma comment(lib, "LLVMDebugInfoDWARFLowLevel.lib")
#pragma comment(lib, "LLVMFrontendDirective.lib")
#pragma comment(lib, "LLVMFrontendAtomic.lib")
#pragma comment(lib, "LLVMAsmParser.lib")
#pragma comment(lib, "clangUnifiedSymbolResolution.lib")

// Windows system libs
#pragma comment(lib, "version.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Ntdll.lib")

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2)
    {
        std::wcout << L"Usage: OdrCop2 <folder of .i files> [more folders ...]";
        return -1;
    }

    std::vector<std::filesystem::path> preprocessedFiles;
    for (int i=1; i<argc; ++i)
    {
        std::filesystem::path root = argv[i];

        std::error_code ec;
        if (!std::filesystem::exists(root, ec) || !std::filesystem::is_directory(root, ec)) {
            std::wcerr << L"Path not found or not a directory: " << root.wstring() << L'\n';
            continue;
        }

        try
        {
            for (const auto& e : std::filesystem::recursive_directory_iterator(root, std::filesystem::directory_options::skip_permission_denied))
            {
                if (e.is_regular_file() && e.path().extension() == ".i") 
                    preprocessedFiles.push_back(e.path());
            }
        }
        catch (const std::filesystem::filesystem_error& ex)
        {
            std::wcerr << L"Filesystem error enumerating " << root.wstring() << L": " << ex.what() << L'\n';
            continue;
        }
    }
    if (preprocessedFiles.empty())
    {
        std::wcerr << L"No preprocessed .i files found\n";
        return -1;
    }

    for (auto& path : preprocessedFiles)
    {
        std::vector<OdrCop2::FunctionInfo> functionInfos;

        std::ifstream f(path, std::ios::binary);
        std::string code{std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};

        bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop2::VisitorAction>(functionInfos), code, { "-x", "c++-cpp-output" });
        if (ok == false)
            std::wcout << L"There were compiler errors in " << path.c_str() << L'\n';

        std::wcout << L"functions found in: " << path.c_str() << L"\n";
        for (const auto& fi : functionInfos)
        {
            std::cout << fi.fullyQualifiedName << '\n';
        }
    }

    return 0;
}
