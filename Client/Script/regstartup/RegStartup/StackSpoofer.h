#pragma once

extern "C" __declspec(dllimport) int GetSpoofStack(size_t minGadgetStackSize);

template<typename T>
__attribute__((naked)) T RetValue() {
    __asm {
        ret
    }
}

extern "C" __declspec(dllimport) void JmpToFunc(...);

extern "C" __declspec(dllimport) void SetSpoofStack(PBYTE* ppSpoofStack);

__attribute__((naked)) void GetRetAddr() {
    __asm {
        mov rax, qword ptr[rsp]
        push rax
        ret
    }
}

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