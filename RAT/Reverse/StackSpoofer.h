#pragma once

#include "Obfuscator.h"

#define UWOP_PUSH_NONVOL 0
#define UWOP_ALLOC_LARGE 1
#define UWOP_ALLOC_SMALL 2
#define UWOP_SET_FPREG 3
#define UWOP_SAVE_NONVOL 4
#define UWOP_SAVE_NONVOL_FAR 5
#define UWOP_SAVE_XMM128 8
#define UWOP_SAVE_XMM128_FAR 9
#define UWOP_PUSH_MACHFRAME 10

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef union _UNWIND_CODE {
    struct {
        unsigned char CodeOffset;
        unsigned char UnwindOp : 4;
        unsigned char OpInfo : 4;
    };
    USHORT FrameOffset;
} UNWIND_CODE, * PUNWIND_CODE;

typedef struct _UNWIND_INFO {
    unsigned char Version : 3;
    unsigned char Flags : 5;
    unsigned char SizeOfProlog;
    unsigned char CountOfCodes;
    unsigned char FrameRegister : 4;
    unsigned char FrameOffset : 4;
    UNWIND_CODE UnwindCode[1];
} UNWIND_INFO, * PUNWIND_INFO;

size_t StrLen(char* str);
int IsEqual(char* str1, char* str2);
PBYTE GetDllBase(char* dllName);
PBYTE FindExpFuncAddr(PBYTE pImageBase, char* funcName, WORD ordinal);

PBYTE pInitStack = NULL;
size_t initStackSize = 0;
PBYTE pGadget = NULL;
size_t gadgetStackSize = 0;

#ifdef _DEBUG
// Gadget 不存在或栈帧过小则不会进行栈欺骗调用
__declspec(dllexport) PVOID Gadget() {
    // 扩大栈帧存储函数参数
    volatile char buf[] = "12345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234";
    __asm {
        call rax
        mov rbx, qword ptr[rbx]
        jmp rbx
    }
}
#endif

size_t LocateSection(PBYTE pImageBase, char* sectionName, PBYTE& pSection) {
    PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pImageBase;
    PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(pImageBase + pDos->e_lfanew);
    WORD numberOfSections = pNt->FileHeader.NumberOfSections;
    PIMAGE_SECTION_HEADER pSectionTable = (PIMAGE_SECTION_HEADER)((DWORD_PTR) & (pNt->OptionalHeader) + pNt->FileHeader.SizeOfOptionalHeader);
    for (size_t i = 0; i < numberOfSections; i++) {
        if (IsEqual((char*)pSectionTable[i].Name, sectionName)) {
            pSection = pImageBase + pSectionTable[i].VirtualAddress;
            return pSectionTable[i].SizeOfRawData;
        }
    }
    return 0;
}

size_t GetStackSize(PBYTE pImageBase, PBYTE pFunc) {
    // 定位 .pdata
    PBYTE pPDATA = NULL;
    size_t pdataSize = LocateSection(pImageBase, OBFSTR(".pdata", "OBFSIG"), pPDATA);
    if (pPDATA == NULL) {
        return 0;
    }

    // 查找函数 RUNTIME_FUNCTION
    PRUNTIME_FUNCTION pRuntimeFunction = (PRUNTIME_FUNCTION)pPDATA;
    while (pRuntimeFunction->BeginAddress && (PBYTE)pRuntimeFunction + sizeof(RUNTIME_FUNCTION) <= pPDATA + pdataSize) {
        if (pImageBase + pRuntimeFunction->BeginAddress <= pFunc && pImageBase + pRuntimeFunction->EndAddress >= pFunc) {
            break;
        }
        pRuntimeFunction++;
    }
    if (!pRuntimeFunction->BeginAddress || (PBYTE)pRuntimeFunction + sizeof(RUNTIME_FUNCTION) > pPDATA + pdataSize) {
        return 0;
    }

    // 计算栈帧大小
    // 该算法在计算 RtlUserThreadStart BaseThreadInitThunk 以外的函数的栈帧大小时不保证准确, 尤其在函数存在特殊的栈操作时
    // 请在 Process Hacker 中检验欺骗情况
    size_t stackSize = 0;
    int UWOP_SET_FPREG_HIT = 0;
    PUNWIND_INFO pUnwindInfo = (PUNWIND_INFO)(pImageBase + pRuntimeFunction->UnwindData);
    for (size_t i = 0; i < pUnwindInfo->CountOfCodes; i++) {
        ULONG unwindOp = pUnwindInfo->UnwindCode[i].UnwindOp;
        ULONG opInfo = pUnwindInfo->UnwindCode[i].OpInfo;
        if (unwindOp == UWOP_PUSH_NONVOL) {
            if (opInfo == 4 && !UWOP_SET_FPREG_HIT) {
                return 0;
            }
            stackSize += 8;
        }
        else if (unwindOp == UWOP_ALLOC_LARGE) {
            i++;
            ULONG frameOffset = pUnwindInfo->UnwindCode[i].FrameOffset;
            if (!opInfo) {
                frameOffset *= 8;
            }
            else {
                i++;
                frameOffset += (pUnwindInfo->UnwindCode[i].FrameOffset << 16);
            }
            stackSize += frameOffset;
        }
        else if (unwindOp == UWOP_ALLOC_SMALL) {
            stackSize += (opInfo + 1) * 8;
        }
        else if (unwindOp == UWOP_SET_FPREG) {
            if ((pUnwindInfo->Flags & UNW_FLAG_EHANDLER) && (pUnwindInfo->Flags & UNW_FLAG_CHAININFO)) {
                return 0;
            }
            UWOP_SET_FPREG_HIT = 1;
            ULONG frameOffset = -0x10 * (pUnwindInfo->FrameOffset);
            stackSize += frameOffset;
        }
        else if (unwindOp == UWOP_SAVE_NONVOL) {
            if (opInfo == 4 || opInfo == 5) {
                return 0;
            }
            i++;
        }
        else if (unwindOp == UWOP_SAVE_NONVOL_FAR) {
            if (opInfo == 4 || opInfo == 5) {
                return 0;
            }
            i += 2;
        }
        else if (unwindOp == UWOP_SAVE_XMM128) {
            i++;
        }
        else if (unwindOp == UWOP_SAVE_XMM128_FAR) {
            i += 2;
        }
        else if (unwindOp == UWOP_PUSH_MACHFRAME) {
            if (!opInfo) {
                stackSize += 0x40;
            }
            else {
                stackSize += 0x48;
            }
        }
        else {
            return 0;
        }
    }
    if (pUnwindInfo->Flags & UNW_FLAG_CHAININFO) {
        return 0;
    }
    return stackSize + sizeof(DWORD_PTR);
}

void FindGadget(PBYTE pImageBase, size_t index) {
    PBYTE pTEXT = NULL;
    size_t textSize = LocateSection(pImageBase, OBFSTR(".text", "OBFSIG"), pTEXT);
    char* gadgetSig[] = { OBFSTR("\x48\x8B\x1B\xFF\xE3\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", "gadgetSigSIG"), OBFSTR("\xFF\x23\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", "spareGadgetSigSIG") };
    int gadgetSigLen[] = { OBFINT(5, "gadgetSigLenSIG"), OBFINT(2, "spareGadgetSigLenSIG") };
    if (pTEXT != NULL) {
        for (size_t i = 0; i < textSize - gadgetSigLen[index]; i++) {
            int isFind = 1;
            for (size_t j = 0; j < gadgetSigLen[index]; j++) {
                if (pTEXT[i + j] != (BYTE)gadgetSig[index][j]) { // char 有符号, BYTE 无符号, 不能直接比较
                    isFind = 0;
                    break;
                }
            }
            if (isFind) {
                size_t curGadgetStackSize = GetStackSize(pImageBase, pTEXT + i);
                if (curGadgetStackSize > gadgetStackSize && curGadgetStackSize < 220) {
                    pGadget = pTEXT + i;
                    gadgetStackSize = curGadgetStackSize;
                }
            }
        }
    }
}

extern "C" int GetSpoofStack(size_t minGadgetStackSize) {
    if (pGadget != NULL) {
        if (gadgetStackSize < minGadgetStackSize) {
            return 0;
        }
        return 1;
    }
    // 获取 RtlUserThreadStart BaseThreadInitThunk 地址
    PBYTE pNtdll = GetDllBase(OBFSTR("ntdll.dll", "OBFSIG"));
    if (pNtdll == NULL) {
        pNtdll = GetDllBase(OBFSTR("NTDLL.DLL", "OBFSIG"));
    }
    PBYTE pKernel32 = GetDllBase(OBFSTR("KERNEL32.DLL", "OBFSIG"));
    if (pKernel32 == NULL) {
        pKernel32 = GetDllBase(OBFSTR("kernel32.dll", "OBFSIG"));
    }
    if (pNtdll == NULL || pKernel32 == NULL) {
        return 0;
    }
    PBYTE pRtlUserThreadStart = FindExpFuncAddr(pNtdll, OBFSTR("RtlUserThreadStart", "OBFSIG"), -1);
    PBYTE pBaseThreadInitThunk = FindExpFuncAddr(pKernel32, OBFSTR("BaseThreadInitThunk", "OBFSIG"), -1);
    if (pRtlUserThreadStart == NULL || pBaseThreadInitThunk == NULL) {
        return 0;
    }

    // 计算 RtlUserThreadStart BaseThreadInitThunk 栈帧大小
    size_t rutsStackSize = GetStackSize(pNtdll, pRtlUserThreadStart);
    size_t btitStackSize = GetStackSize(pKernel32, pBaseThreadInitThunk);
    if (!rutsStackSize || !btitStackSize) {
        return 0;
    }
    initStackSize = rutsStackSize + btitStackSize;

    // 查找 RtlUserThreadStart BaseThreadInitThunk 栈帧
    DWORD_PTR _TEB = __readgsqword(0x30);
    DWORD_PTR stackBase = *(PDWORD_PTR)(_TEB + 8);
    DWORD_PTR stackLimit = *(PDWORD_PTR)(_TEB + 16);
    DWORD_PTR stack = stackBase - sizeof(DWORD_PTR);
    while (stackLimit < stack - rutsStackSize - btitStackSize) {
        if (*(PDWORD_PTR)stack == 0x00) {
            DWORD_PTR addr = *(PDWORD_PTR)(stack - rutsStackSize);
            if (addr > (DWORD_PTR)pRtlUserThreadStart && addr < (DWORD_PTR)pRtlUserThreadStart + 0xFF) {
                addr = *(PDWORD_PTR)(stack - rutsStackSize - btitStackSize);
                if (addr > (DWORD_PTR)pBaseThreadInitThunk && addr < (DWORD_PTR)pBaseThreadInitThunk + 0xFF) {
                    pInitStack = (PBYTE)stack - rutsStackSize - btitStackSize;
                }
            }
        }
        stack -= sizeof(DWORD_PTR);
    }
    if (pInitStack == NULL) {
        return 0;
    }

    // 查找 Gadget
    DWORD_PTR _PEB = __readgsqword(0x60);
    DWORD_PTR imageBase = *(PDWORD_PTR)(_PEB + 0x10);
    FindGadget((PBYTE)imageBase, 0);
    // 查找备用 Gadget
    if (OBFINT(1, "useSpareGadgetSIG") && pGadget == NULL) {
        DWORD_PTR _PEB_LDR_DATA = *(PDWORD_PTR)(_PEB + 0x18);
        PLIST_ENTRY pInInitializationOrderModuleList = (PLIST_ENTRY) * (PDWORD_PTR)(_PEB_LDR_DATA + 0x30);
        PLIST_ENTRY pNode = pInInitializationOrderModuleList;
        do {
            FindGadget((PBYTE) * (PDWORD_PTR)((PBYTE)pNode + 0x10), 1);
            pNode = pNode->Flink;
        } while (pNode->Flink != pInInitializationOrderModuleList);
    }
    if (pGadget == NULL || gadgetStackSize < minGadgetStackSize) {
        return 0;
    }
    return 1;
}

template<typename T>
__attribute__((naked)) T RetValue() {
    __asm {
        ret
    }
}

__attribute__((naked)) void FixRSP(...) {
    __asm {
        add rsp, initStackSize
        add rsp, gadgetStackSize
        add rsp, 0x98
        ret
    }
}

PVOID pFixRSP = FixRSP;

extern "C" __attribute__((naked)) void JmpToFunc(...) {
    __asm {
        lea rbx, [pFixRSP]
        mov rax, pGadget
        mov qword ptr[rsp], rax
        sub rsp, 0x08
        ret
    }
}

extern "C" void SetSpoofStack(PBYTE* ppSpoofStack) {
    *ppSpoofStack -= (initStackSize + sizeof(DWORD_PTR) * 20);
    memcpy(*ppSpoofStack, pInitStack, initStackSize);
    *(PDWORD_PTR)(*ppSpoofStack + initStackSize) = 0x00;
    *ppSpoofStack -= gadgetStackSize;
}

__attribute__((naked)) void GetRetAddr() {
    __asm {
        mov rax, qword ptr[rsp]
        push rax
        ret
    }
}

// 计算参数总大小, 不能超过 15 个
#define SIZEOF0() 0
#define SIZEOF1(a) sizeof(a)
#define SIZEOF2(a, ...) sizeof(a) + SIZEOF1(__VA_ARGS__)
#define SIZEOF3(a, ...) sizeof(a) + SIZEOF2(__VA_ARGS__)
#define SIZEOF4(a, ...) sizeof(a) + SIZEOF3(__VA_ARGS__)
#define SIZEOF5(a, ...) sizeof(a) + SIZEOF4(__VA_ARGS__)
#define SIZEOF6(a, ...) sizeof(a) + SIZEOF5(__VA_ARGS__)
#define SIZEOF7(a, ...) sizeof(a) + SIZEOF6(__VA_ARGS__)
#define SIZEOF8(a, ...) sizeof(a) + SIZEOF7(__VA_ARGS__)
#define SIZEOF9(a, ...) sizeof(a) + SIZEOF8(__VA_ARGS__)
#define SIZEOF10(a, ...) sizeof(a) + SIZEOF9(__VA_ARGS__)
#define SIZEOF11(a, ...) sizeof(a) + SIZEOF10(__VA_ARGS__)
#define SIZEOF12(a, ...) sizeof(a) + SIZEOF11(__VA_ARGS__)
#define SIZEOF13(a, ...) sizeof(a) + SIZEOF12(__VA_ARGS__)
#define SIZEOF14(a, ...) sizeof(a) + SIZEOF13(__VA_ARGS__)
#define SIZEOF15(a, ...) sizeof(a) + SIZEOF14(__VA_ARGS__)

// 栈欺骗
// 参数中不能存在函数调用, 如果 Gadget 栈帧大小 < 参数总大小则直接调用函数
#define SPOOF(pFunc, parasNum, ...) ({ \
    size_t minGadgetStackSize = SIZEOF##parasNum(__VA_ARGS__); \
    int result = GetSpoofStack(minGadgetStackSize); \
    using retType = decltype(((decltype(pFunc)*)pFunc)(__VA_ARGS__)); \
    if (result) { \
        int getRetAddr = 1; \
        while (1) { \
            if (!getRetAddr) { \
                PBYTE pSpoofStack; \
                __asm { \
                    mov pSpoofStack, rsp \
                } \
                SetSpoofStack(&pSpoofStack); \
                __asm { \
                    mov rsp, pSpoofStack \
                } \
                __asm { \
                    lea rax, [pFunc] \
                } \
                __asm { \
                    mov qword ptr[rsp - 0x08], rax \
                } \
                __asm { \
                    pop rax \
                } \
                JmpToFunc(__VA_ARGS__); \
            } \
            __asm { \
                call GetRetAddr \
            } \
            if (!getRetAddr) { \
                break; \
            } \
            getRetAddr = 0; \
        } \
    } \
    if (!result) { \
        ((decltype(pFunc)*)pFunc)(__VA_ARGS__); \
    } \
    RetValue<retType>(); \
})