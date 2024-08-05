#include "pch.h"
#include <iostream>
#include <windows.h>

using namespace std;

typedef HMODULE(WINAPI* pLoadLibraryA)(LPCSTR);
typedef FARPROC(WINAPI* pGetProcAddress)(HMODULE, LPCSTR);
typedef LPVOID(WINAPI* pVirtualAlloc)(LPVOID, SIZE_T, DWORD, DWORD);
typedef BOOL(WINAPI* pVirtualProtect)(LPVOID, SIZE_T, DWORD, PDWORD);

#pragma code_seg(".loader")

void ReflectiveLoader(PBYTE jmpAddr) {
    // PE 结构信息
    PBYTE dllBase = jmpAddr + 5;
    PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)dllBase;
    PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(dllBase + pDos->e_lfanew);
    WORD numberOfSections = pNt->FileHeader.NumberOfSections;
    DWORD addressOfEntryPoint = pNt->OptionalHeader.AddressOfEntryPoint;
    DWORD_PTR imageBase = pNt->OptionalHeader.ImageBase;
    DWORD sizeOfImage = pNt->OptionalHeader.SizeOfImage;
    DWORD sizeOfHeaders = pNt->OptionalHeader.SizeOfHeaders;
    DWORD importDirRVA = ((PIMAGE_DATA_DIRECTORY) & (pNt->OptionalHeader.DataDirectory[1]))->VirtualAddress;
    DWORD relocDirRVA = ((PIMAGE_DATA_DIRECTORY) & (pNt->OptionalHeader.DataDirectory[5]))->VirtualAddress;

    // 获取 Kernel32 基址
    DWORD_PTR _PEB = __readgsqword(0x60);
    DWORD_PTR _PEB_LDR_DATA = *(PDWORD_PTR)(_PEB + 0x18);
    DWORD_PTR inLoadOrderModuleList = *(PDWORD_PTR)(_PEB_LDR_DATA + 0x20);
    DWORD_PTR ntdllLIST_ENTRY = *(PDWORD_PTR)inLoadOrderModuleList;
    DWORD_PTR kernel32LIST_ENTRY = *(PDWORD_PTR)ntdllLIST_ENTRY;
    DWORD_PTR kernel32Base = *(PDWORD_PTR)(kernel32LIST_ENTRY + 0x20);
    // 获取 Kernel32 导出表信息
    PIMAGE_DOS_HEADER pKernel32Dos = (PIMAGE_DOS_HEADER)kernel32Base;
    PIMAGE_NT_HEADERS pKernel32Nt = (PIMAGE_NT_HEADERS)(kernel32Base + pKernel32Dos->e_lfanew);
    PIMAGE_DATA_DIRECTORY pDataDir = &((pKernel32Nt->OptionalHeader.DataDirectory)[0]);
    PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)(kernel32Base + pDataDir->VirtualAddress);
    DWORD numberOfNames = pExportDir->NumberOfNames; // 以函数名导出的导出函数的总数
    PDWORD pAddressOfFunctions = (PDWORD)(kernel32Base + pExportDir->AddressOfFunctions); // 元素为导出函数的 RVA 的数组
    PDWORD pAddressOfNames = (PDWORD)(kernel32Base + pExportDir->AddressOfNames); // 元素为导出函数的名称的 RVA 的数组
    PWORD pAddressOfNameOrdinals = (PWORD)(kernel32Base + pExportDir->AddressOfNameOrdinals); // 元素为与 AddressOfNames 同下标的导出函数在 AddressOfFunctions 中的下标的数组
    // 获取 Kernel32 函数地址
    pLoadLibraryA funcLoadLibraryA = NULL;
    pGetProcAddress funcGetProcAddress = NULL;
    volatile char loadLibraryAName[] = { 'L', 'o', 'a', 'd', 'L', 'i', 'b', 'r', 'a', 'r', 'y', 'A', '\0' };
    volatile char getProcAddressName[] = { 'G', 'e', 't', 'P', 'r', 'o', 'c', 'A', 'd', 'd', 'r', 'e', 's', 's', '\0' };
    for (int i = 0; i < numberOfNames; i++) {
        char* funcName = (char*)(kernel32Base + pAddressOfNames[i]);
        int equal = 0;
        for (int j = 0;; j++) {
            if (funcGetProcAddress == NULL) {
                if (*(funcName + j) != *(getProcAddressName + j)) {
                    break;
                }
            }
            else {
                if (*(funcName + j) != *(loadLibraryAName + j)) {
                    break;
                }
            }
            if (*(funcName + j) == '\0') {
                equal = 1;
                break;
            }
        }
        if (equal) {
            if (funcGetProcAddress == NULL) {
                funcGetProcAddress = (pGetProcAddress)(kernel32Base + pAddressOfFunctions[pAddressOfNameOrdinals[i]]);
            }
            else {
                funcLoadLibraryA = (pLoadLibraryA)(kernel32Base + pAddressOfFunctions[pAddressOfNameOrdinals[i]]);
            }
            continue;
        }
    }
    volatile char virtualAllocName[] = { 'V', 'i', 'r', 't', 'u', 'a', 'l', 'A', 'l', 'l', 'o', 'c', '\0' };
    volatile char virtualProtectName[] = { 'V', 'i', 'r', 't', 'u', 'a', 'l', 'P', 'r', 'o', 't', 'e', 'c', 't', '\0' };
    pVirtualAlloc funcVirtualAlloc = (pVirtualAlloc)funcGetProcAddress((HMODULE)kernel32Base, (char*)virtualAllocName);
    pVirtualProtect funcVirtualProtect = (pVirtualProtect)funcGetProcAddress((HMODULE)kernel32Base, (char*)virtualProtectName);

    // 映射节到内存
    int executeSize; // .text + .loader
    PIMAGE_SECTION_HEADER pSectionDir = (PIMAGE_SECTION_HEADER)((DWORD_PTR) & (pNt->OptionalHeader) + pNt->FileHeader.SizeOfOptionalHeader);
    for (int i = 0; i < numberOfSections; i++) {
        if (*(pSectionDir[i].Name + 1) == 'l') {
            executeSize = pSectionDir[i].VirtualAddress + pSectionDir[i].SizeOfRawData - sizeOfHeaders;
            break;
        }
    }
    int pageDev = 64 * 1024 - executeSize % (64 * 1024); // 页大小为 64 * 1024 的整数倍，修改内存属性只能以页为单位。
    DWORD_PTR vtImageBase = (DWORD_PTR)funcVirtualAlloc(NULL, pageDev + sizeOfImage - sizeOfHeaders, MEM_COMMIT, PAGE_READWRITE) + pageDev - sizeOfHeaders;
    for (int i = 0; i < numberOfSections; i++) {
        for (int j = 0; j < pSectionDir[i].SizeOfRawData; j++) {
            *(PBYTE)(vtImageBase + pSectionDir[i].VirtualAddress + j) = *(PBYTE)(dllBase + pSectionDir[i].PointerToRawData + j);
        }
    }

    // 修改 IAT 表
    PIMAGE_IMPORT_DESCRIPTOR pImportDir = (PIMAGE_IMPORT_DESCRIPTOR)(vtImageBase + importDirRVA);
    while (pImportDir->FirstThunk) {
        char* dllName = (char*)(vtImageBase + pImportDir->Name);
        DWORD_PTR importDllBase = (DWORD_PTR)funcLoadLibraryA(dllName);
        PIMAGE_THUNK_DATA pIAT = (PIMAGE_THUNK_DATA)(vtImageBase + pImportDir->FirstThunk);
        PIMAGE_THUNK_DATA pOriginalFirstThunk = (PIMAGE_THUNK_DATA)(vtImageBase + pImportDir->OriginalFirstThunk);
        while (pOriginalFirstThunk->u1.AddressOfData) {
            if (IMAGE_SNAP_BY_ORDINAL(pOriginalFirstThunk->u1.Ordinal)) {
                *(PDWORD_PTR)pIAT = (DWORD_PTR)funcGetProcAddress((HMODULE)importDllBase, (char*)(*(PDWORD_PTR)pOriginalFirstThunk & 0xFFFF));
            }
            else {
                PIMAGE_IMPORT_BY_NAME pImportByName = (PIMAGE_IMPORT_BY_NAME)(vtImageBase + pOriginalFirstThunk->u1.AddressOfData);
                *(PDWORD_PTR)pIAT = (DWORD_PTR)funcGetProcAddress((HMODULE)importDllBase, pImportByName->Name);
            }
            pIAT++;
            pOriginalFirstThunk++;
        }
        pImportDir++;
    }

    // 地址重定位
    PIMAGE_BASE_RELOCATION pRelocDir = (PIMAGE_BASE_RELOCATION)(vtImageBase + relocDirRVA);
    while (pRelocDir->SizeOfBlock) {
        PWORD pOffset = (PWORD)((DWORD_PTR)pRelocDir + sizeof(IMAGE_BASE_RELOCATION));
        int offsetNum = (pRelocDir->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
        while (offsetNum--) {
            if (pOffset[offsetNum] >> 12 == IMAGE_REL_BASED_DIR64) {
                *(PDWORD_PTR)(vtImageBase + pRelocDir->VirtualAddress + (pOffset[offsetNum] & 0xFFF)) += vtImageBase - imageBase;
            }
        }
        pRelocDir = (PIMAGE_BASE_RELOCATION)((DWORD_PTR)pRelocDir + pRelocDir->SizeOfBlock);
    }

    DWORD oldProtect;
    funcVirtualProtect((PVOID)(vtImageBase + sizeOfHeaders - pageDev), pageDev + executeSize, PAGE_EXECUTE_READ, &oldProtect);

    // 调用 DLL 入口函数
    ((BOOL(*)(...))(vtImageBase + addressOfEntryPoint))(jmpAddr, DLL_PROCESS_ATTACH, 0);
}