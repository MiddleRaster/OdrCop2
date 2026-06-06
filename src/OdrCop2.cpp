#include <vector>
#include <iostream>
#include <filesystem>

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

    return 0;
}
