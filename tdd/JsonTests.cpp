#include "stdafx.h"

#include "..\src\FindJson.h"

import std;
import tdd20;
using namespace TDD20;


Test JsonTests[] =
{
    {"Can find compile_commands.json file", []
        {
            Assert::AreNotEqual("", OdrCop2::Find::JsonFile(), "should be able to find compile_commands.json");
        }
    },
    {"Can read compile_commands.json file", []
        {
            auto fqfns = OdrCop2::Find::FullyQualifiedFileNames();
            Assert::AreEqual(2, fqfns.size(), "there should have been 2 fqfns found");
            Assert::AreEqual("x64\\\\Debug\\\\\\\\AnotherFile.obj", fqfns[0]);
            Assert::AreEqual("x64\\\\Debug\\\\\\\\demo.obj",        fqfns[1]);
        }
    },
    {"trying out CommandLineArgsW", []
        {
            std::string commandLine = "clang-cl.exe /nologo /W4 /WX /diagnostics:column /Od /D _DEBUG /D _CONSOLE /D _UNICODE /D UNICODE /EHsc /MDd /GS /fp:precise /Zc:wchar_t /Zc:forScope /Zc:inline /std:c++23preview /permissive- /Fox64\\Debug\\\\ \"C:/Users/Bill/source/repos/OdrCop2/demo/demo.cpp\"";
            auto fqfn = OdrCop2::Find::FQFNFromCommandLine(commandLine);
            Assert::AreEqual("x64\\Debug\\\\demo.obj", fqfn);
        }
    },
};
