#pragma once

size_t StrLen(char* str) {
    size_t len = 0;
    if (str != NULL) {
        while (str[len] != '\0') len++;
    }
    return len;
}

int StrToInt(char* str) {
    int result = 0;
    if (str != NULL) {
        while (*str != '\0') {
            result = result * 10 + (*str - '0');
            str++;
        }
    }
    return result;
}

extern "C" __declspec(dllimport) int GetRand(int range);

extern "C" __declspec(dllimport) void MySleep(DWORD time);

extern "C" __declspec(dllimport) PVOID SafeAlloc(size_t size); // 返回值需通过 SafeFree 手动释放

extern "C" __declspec(dllimport) PVOID SafeReAlloc(PVOID p, size_t size); // 返回值需通过 SafeFree 手动释放

extern "C" __declspec(dllimport) BOOL SafeFree(PVOID p);

extern "C" __declspec(dllimport) char* WStrToStr(wchar_t* wStr, size_t* pLen); // 返回值需通过 SafeFree 手动释放

extern "C" __declspec(dllimport) wchar_t* StrToWStr(char* str, size_t* pLen); // 返回值需通过 SafeFree 手动释放

extern "C" __declspec(dllimport) char* Base64Encode(char* plaintext, size_t plaintextLen, DWORD* pLen); // 返回值需通过 SafeFree 手动释放

extern "C" __declspec(dllimport) PBYTE Base64Decode(char* ciphertext, size_t ciphertextLen, DWORD* pSize); // 返回值需通过 SafeFree 手动释放

extern "C" __declspec(dllimport) void AsyncCommandOutput(char* id, char* username, char* content, char* note);

#ifndef _DEBUG
void* memset(void* dest, int c, size_t count) {
    if (dest != NULL) {
        for (size_t i = 0; i < count; i++) {
            *((PBYTE)dest + i) = c;
        }
    }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count) {
    if (src != NULL && dest != NULL) {
        for (size_t i = 0; i < count; i++) {
            *((PBYTE)dest + i) = *((PBYTE)src + i);
        }
    }
    return dest;
}
#endif