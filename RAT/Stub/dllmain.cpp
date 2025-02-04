#include "pch.h"
#include <iostream>
#include <windows.h>
#include <wincrypt.h>

#pragma comment(lib, "Crypt32.lib")

using namespace std;

// 一个模拟 RAT 内部函数的 DLL, 用于 ExeLite 的编译与测试
// 将 Stub.dll 和 Stub.lib 复制到 ExeLite 插件的 .sln 目录

extern "C" __declspec(dllexport) int GetRand(int range) {
    return rand() % range;
}

extern "C" __declspec(dllexport) void MySleep(DWORD time) {
    Sleep(time);
}

extern "C" __declspec(dllexport) PVOID SafeAlloc(size_t size) {
    return malloc(size);
}

extern "C" __declspec(dllexport) PVOID SafeReAlloc(PVOID p, size_t size) {
    return realloc(p, size);
}

extern "C" __declspec(dllexport) BOOL SafeFree(PVOID p) {
    free(p);
    return 1;
}

extern "C" __declspec(dllexport) char* WStrToStr(wchar_t* wStr, size_t* pLen) {
    *pLen = WideCharToMultiByte(CP_UTF8, 0, wStr, -1, NULL, 0, NULL, NULL);
    if (!*pLen) {
        return NULL;
    }
    char* str = (char*)SafeAlloc(*pLen);
    if (str == NULL) {
        return NULL;
    }
    if (!WideCharToMultiByte(CP_UTF8, 0, wStr, -1, str, *pLen, NULL, NULL)) {
        SafeFree(str);
        return NULL;
    }
    *pLen--;
    return str;
}

extern "C" __declspec(dllexport) wchar_t* StrToWStr(char* str, size_t* pLen) {
    *pLen = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if (!*pLen) {
        return NULL;
    }
    wchar_t* wStr = (wchar_t*)SafeAlloc(*pLen * sizeof(wchar_t));
    if (wStr == NULL) {
        return NULL;
    }
    if (!MultiByteToWideChar(CP_UTF8, 0, str, -1, wStr, *pLen)) {
        SafeFree(wStr);
        return NULL;
    }
    *pLen--;
    return wStr;
}

extern "C" __declspec(dllexport) char* Base64Encode(char* plaintext, size_t plaintextLen, DWORD* pLen) {
    if (!CryptBinaryToStringA((PBYTE)plaintext, plaintextLen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, pLen)) {
        return NULL;
    }
    char* ciphertext = (char*)SafeAlloc(*pLen);
    if (ciphertext == NULL) {
        return NULL;
    }
    if (!CryptBinaryToStringA((PBYTE)plaintext, plaintextLen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, ciphertext, pLen)) {
        SafeFree(ciphertext);
        return NULL;
    }
    return ciphertext;
}

extern "C" __declspec(dllexport) PBYTE Base64Decode(char* ciphertext, size_t ciphertextLen, DWORD* pSize) {
    if (!CryptStringToBinaryA(ciphertext, ciphertextLen, CRYPT_STRING_BASE64, NULL, pSize, NULL, NULL)) {
        return NULL;
    }
    PBYTE plaintext = (PBYTE)SafeAlloc(*pSize);
    if (plaintext == NULL) {
        return NULL;
    }
    if (!CryptStringToBinaryA(ciphertext, ciphertextLen, CRYPT_STRING_BASE64, plaintext, pSize, NULL, NULL)) {
        SafeFree(plaintext);
        return NULL;
    }
    return plaintext;
}

extern "C" __declspec(dllexport) void AsyncCommandOutput(char* id, char* username, char* content, char* note) {
    printf("%s %s %s %s\n", id, username, content, note);
}

extern "C" __declspec(dllexport) void JmpToFunc(...) {}

extern "C" __declspec(dllexport) void SetSpoofStack(PBYTE* ppSpoofStack) {}

extern "C" __declspec(dllexport) int GetSpoofStack(size_t minGadgetStackSize) { return 0; }

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}