msbuild demo\demo.vcxproj /t:rebuild /bl:binlog.binlog
BL2OC\bin\Release\net8.0\BL2OC.exe .\binlog.binlog .
x64\Release\OdrCop2.exe .\