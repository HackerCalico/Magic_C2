#pragma once

#include "StackSpoofer.h"

HANDLE hHeap = NULL;
HANDLE hEvent = NULL;
SYSTEMTIME st = {};
CRITICAL_SECTION cs;
unsigned int seed = 0;

int InitCLite() {
    hHeap = SPOOF(GetProcessHeap, 0);
    if (hHeap == NULL) {
        return 0;
    }
    SPOOF(GetLocalTime, 1, &st);
    SPOOF(InitializeCriticalSection, 1, &cs);
    seed = st.wMilliseconds;
    hEvent = SPOOF(CreateEventA, 4, NULL, TRUE, FALSE, NULL);
    return 1;
}

extern "C" int GetRand(int range) {
    seed = seed * 1103515245 + 12345;
    return (seed / 65536) % range;
}

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

char* StrStr(char* str1, char* str2) {
    if (str1 == NULL || str2 == NULL) {
        return NULL;
    }
    size_t s1Len = StrLen(str1);
    size_t s2Len = StrLen(str2);
    for (size_t i = 0; i <= s1Len - s2Len; i++) {
        char end = str1[i + s2Len];
        str1[i + s2Len] = '\0';
        if (IsEqual(str1 + i, str2)) {
            str1[i + s2Len] = end;
            return str1 + i;
        }
        str1[i + s2Len] = end;
    }
    return NULL;
}

// 统计全部堆管理情况
int allHeapNum = 0;

extern "C" PVOID SafeAlloc(size_t size) {
    PVOID p = SPOOF(HeapAlloc, 3, hHeap, HEAP_ZERO_MEMORY, size + 10);
    if (p != NULL) {
        allHeapNum++;
    }
    return p;
}

extern "C" PVOID SafeReAlloc(PVOID p, size_t size) {
    return SPOOF(HeapReAlloc, 4, hHeap, HEAP_ZERO_MEMORY, p, size + 10);
}

extern "C" BOOL SafeFree(PVOID p) {
    if (p != NULL) {
        allHeapNum--;
    }
    return SPOOF(HeapFree, 3, hHeap, 0, p);
}

extern "C" char* WStrToStr(wchar_t* wStr, size_t* pLen) {
    *pLen = SPOOF(WideCharToMultiByte, 8, CP_UTF8, 0, wStr, -1, NULL, 0, NULL, NULL);
    if (!*pLen) {
        return NULL;
    }
    char* str = (char*)SafeAlloc(*pLen);
    if (str == NULL) {
        return NULL;
    }
    if (!SPOOF(WideCharToMultiByte, 8, CP_UTF8, 0, wStr, -1, str, *pLen, NULL, NULL)) {
        SafeFree(str);
        return NULL;
    }
    *pLen--;
    return str;
}

extern "C" wchar_t* StrToWStr(char* str, size_t* pLen) {
    *pLen = SPOOF(MultiByteToWideChar, 6, CP_UTF8, 0, str, -1, NULL, 0);
    if (!*pLen) {
        return NULL;
    }
    wchar_t* wStr = (wchar_t*)SafeAlloc(*pLen * sizeof(wchar_t));
    if (wStr == NULL) {
        return NULL;
    }
    if (!SPOOF(MultiByteToWideChar, 6, CP_UTF8, 0, str, -1, wStr, *pLen)) {
        SafeFree(wStr);
        return NULL;
    }
    *pLen--;
    return wStr;
}

extern "C" char* Base64Encode(char* plaintext, size_t plaintextLen, DWORD* pLen) {
    if (!SPOOF(CryptBinaryToStringA, 5, (PBYTE)plaintext, plaintextLen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, pLen)) {
        return NULL;
    }
    char* ciphertext = (char*)SafeAlloc(*pLen);
    if (ciphertext == NULL) {
        return NULL;
    }
    if (!SPOOF(CryptBinaryToStringA, 5, (PBYTE)plaintext, plaintextLen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, ciphertext, pLen)) {
        SafeFree(ciphertext);
        return NULL;
    }
    return ciphertext;
}

extern "C" PBYTE Base64Decode(char* ciphertext, size_t ciphertextLen, DWORD* pSize) {
    if (!SPOOF(CryptStringToBinaryA, 7, ciphertext, ciphertextLen, CRYPT_STRING_BASE64, NULL, pSize, NULL, NULL)) {
        return NULL;
    }
    PBYTE plaintext = (PBYTE)SafeAlloc(*pSize);
    if (plaintext == NULL) {
        return NULL;
    }
    if (!SPOOF(CryptStringToBinaryA, 7, ciphertext, ciphertextLen, CRYPT_STRING_BASE64, plaintext, pSize, NULL, NULL)) {
        SafeFree(plaintext);
        return NULL;
    }
    return plaintext;
}

#ifndef _DEBUG
extern "C" int _fltused = 0;

extern "C" void __chkstk() {}

int atexit(void(__cdecl* func)(void)) {
    return 0;
}

void* memset(void* dest, int c, size_t count) {
    if (dest != NULL) {
        for (size_t i = 0; i < count; i++) {
            ((PBYTE)dest)[i] = c;
        }
    }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count) {
    if (src != NULL && dest != NULL) {
        for (size_t i = 0; i < count; i++) {
            ((PBYTE)dest)[i] = ((PBYTE)src)[i];
        }
    }
    return dest;
}
#endif

char CheckEscape(char c) {
    switch (c)
    {
    case '\r':
        return 'r';
    case '\n':
        return 'n';
    case '\t':
        return 't';
    case '\b':
        return 'b';
    case '\f':
        return 'f';
    }
    return c;
}

// 统计对象堆管理情况
int mapHeapNum = 0;
int stringHeapNum = 0;

class stringObj {
public:
    char* str;
    size_t length;
    int autoDestroy;

    // stringObj s;
    stringObj() : str(NULL), length(0), autoDestroy(1) {}

    // stringObj s = "xxx";
    stringObj(char* initStr) : str(NULL), length(0), autoDestroy(1) {
        if (initStr != NULL) {
            size_t initStrLen = StrLen(initStr);
            str = (char*)SafeAlloc(initStrLen + 1);
            if (str != NULL) {
                stringHeapNum++;
                memcpy(str, initStr, initStrLen + 1);
                length = initStrLen;
            }
        }
    }

    // stringObj s1 = s2;
    stringObj(const stringObj& s) : str(NULL), length(0), autoDestroy(1) {
        CopyStr(s);
    }

    ~stringObj() {
        if (autoDestroy) {
            DestroyHeap();
        }
    }

    // s1 = s2;
    stringObj& operator=(const stringObj& s) {
        if (this != &s) {
            CopyStr(s);
        }
        return *this;
    }

    stringObj operator+(const stringObj& s) {
        if (str != NULL && s.str == NULL) {
            return *this;
        }
        if (str == NULL && s.str != NULL) {
            return s;
        }
        stringObj t;
        if (str != NULL && s.str != NULL) {
            t.str = (char*)SafeAlloc(length + s.length + 1);
            if (t.str != NULL) {
                stringHeapNum++;
                memcpy(t.str, str, length);
                memcpy(t.str + length, s.str, s.length + 1);
                t.length = length + s.length;
            }
        }
        return t;
    }

    int operator==(stringObj& s) {
        if (!length && !s.length) { // NULL == "" True
            return 1;
        }
        return IsEqual(str, s.str);
    }

    int operator!=(stringObj& s) {
        return !(*this == s);
    }

    void Xor() {
        if (str != NULL && length < 500) {
            XorData(str, str, length);
        }
    }

    stringObj GetJson() {
        if (!length) {
            return OBFSTR("\"\"", "OBFSIG");
        }
        else {
            // 统计要添加的转义用的 \ 的字符数
            size_t escapeNum = 0;
            for (size_t i = 0; i < length; i++) {
                char c = str[i];
                if (c == '"' || c == '\\' || CheckEscape(c) != c) {
                    escapeNum++;
                }
            }
            // "len\\gth"\0
            stringObj t;
            t.str = (char*)SafeAlloc(length + escapeNum + 3);
            if (t.str != NULL) {
                stringHeapNum++;
                size_t index = 1;
                for (size_t i = 0; i < length; i++) {
                    char c = CheckEscape(str[i]);
                    if (c == '"' || c == '\\' || c != str[i]) {
                        t.str[index] = '\\';
                        index++;
                    }
                    t.str[index] = c;
                    index++;
                }
                t.str[0] = t.str[index] = '"';
                t.str[index + 1] = '\0';
                t.length = length + escapeNum + 2;
            }
            return t;
        }
    }

    void CopyStr(const stringObj& s) {
        DestroyHeap();
        if (str == NULL && s.str != NULL) {
            str = (char*)SafeAlloc(s.length + 1);
            if (str != NULL) {
                stringHeapNum++;
                memcpy(str, s.str, s.length + 1);
                length = s.length;
            }
        }
    }

    void DestroyHeap() {
        if (str != NULL) {
            SafeFree(str);
            stringHeapNum--;
            str = NULL;
            length = 0;
        }
    }
};

class pointer {
public:
    PBYTE p;
    int autoDestroy;

    // pointer p;
    pointer() : p(NULL), autoDestroy(1) {}

    // pointer p = pointer(pData);
    pointer(PBYTE pData) : p(pData), autoDestroy(1) {}

    // pointer p1 = p2;
    pointer(const pointer& _p) : p(_p.p), autoDestroy(1) {}

    // p1 = p2;
    pointer& operator=(const pointer& _p) {
        if (this != &_p) {
            p = _p.p;
        }
        return *this;
    }

    void Xor() {}

    stringObj GetJson() {
        return OBFSTR("\"\"", "OBFSIG");
    }

    void DestroyHeap() {}
};

template <typename V>
class mapObj {
public:
    struct Pairs {
        stringObj key;
        V value;
    };
    size_t index;
    size_t length;
    Pairs* pairsList;
    int autoDestroy;

    mapObj() : index(0), length(0), pairsList(NULL), autoDestroy(1) {}

    // mapObj m1 = m2;
    mapObj(const mapObj& m) : index(0), length(0), pairsList(NULL), autoDestroy(1) {
        CopyPairsList(m);
    }

    ~mapObj() {
        if (autoDestroy) {
            DestroyHeap();
        }
    }

    // m1 = m2;
    mapObj& operator=(const mapObj& m) {
        if (this != &m) {
            CopyPairsList(m);
        }
        return *this;
    }

    void operator+=(const mapObj& m) {
        if (m.pairsList != NULL) {
            for (size_t i = 0; i < m.index; i++) {
                this->Set(m.pairsList[i].key, m.pairsList[i].value);
            }
        }
    }

    // 从 index = 0 开始向 pairsList 添加 Pairs 元素
    void Set(stringObj key, V value) {
        if (pairsList == NULL) {
            pairsList = (Pairs*)SafeAlloc(50 * sizeof(Pairs));
            if (pairsList != NULL) {
                mapHeapNum++;
                length = 50;
            }
            else {
                return;
            }
        }
        else {
            // 键重复
            for (size_t i = 0; i < index; i++) {
                if (pairsList[i].key == key) {
                    pairsList[i].value = value;
                    return;
                }
            }
            // 扩容
            if (index >= length) {
                Pairs* rePairsList = (Pairs*)SafeReAlloc(pairsList, (length + 50) * sizeof(Pairs));
                if (rePairsList != NULL) {
                    pairsList = rePairsList;
                    length += 50;
                }
                else {
                    return;
                }
            }
        }
        Pairs pairs = { key , value };
        pairs.key.autoDestroy = 0; // 绕过自动销毁机制
        pairs.value.autoDestroy = 0;
        memcpy(&pairsList[index], &pairs, sizeof(Pairs));
        index++;
    }

    V Get(stringObj key) { // 键不存在返回默认对象
        if (pairsList == NULL) {
            goto End;
        }
        for (size_t i = 0; i < index; i++) {
            if (pairsList[i].key == key) {
                return pairsList[i].value;
            }
        }
    End:
        V value;
        return value;
    }

    void Xor() {
        if (pairsList != NULL) {
            for (size_t i = 0; i < index; i++) {
                pairsList[i].key.Xor();
                pairsList[i].value.Xor();
            }
        }
    }

    stringObj GetJson() {
        if (pairsList == NULL) {
            return OBFSTR("{}", "OBFSIG");
        }
        else {
            stringObj json = OBFSTR("{", "OBFSIG");
            for (size_t i = 0; i < index; i++) {
                json = json + pairsList[i].key.GetJson() + OBFSTR(":", "OBFSIG") + pairsList[i].value.GetJson();
                if (i < index - 1) {
                    json = json + OBFSTR(",", "OBFSIG");
                }
            }
            return json + OBFSTR("}", "OBFSIG");
        }
    }

    void CopyPairsList(const mapObj& m) {
        DestroyHeap();
        if (pairsList == NULL && m.pairsList != NULL) {
            for (size_t i = 0; i < m.index; i++) {
                this->Set(m.pairsList[i].key, m.pairsList[i].value);
            }
        }
    }

    void DestroyHeap() {
        if (pairsList != NULL) {
            for (size_t i = 0; i < index; i++) {
                pairsList[i].key.DestroyHeap();
                pairsList[i].value.DestroyHeap();
            }
            SafeFree(pairsList);
            mapHeapNum--;
            pairsList = NULL;
            index = length = 0;
        }
    }
};

// "xxx"
int StrFromJson(char* json, stringObj& s) {
    if (json != NULL) {
        size_t jsonLen = StrLen(json);
        if (jsonLen > 1 && json[0] == '"' && json[jsonLen - 1] == '"') {
            // 统计 json 中用于转义的 \ 的字符数
            size_t escapeNum = 0;
            for (size_t i = 1; i < jsonLen - 1; i++) {
                if (json[i] == '\\') {
                    escapeNum++;
                    i++;
                }
            }
            // "len\\gth"\0
            size_t expectedSize = jsonLen - escapeNum - 1;
            if (expectedSize > 0) {
                stringObj t;
                t.str = (char*)SafeAlloc(expectedSize);
                if (t.str != NULL) {
                    stringHeapNum++;
                    size_t index = 0;
                    for (size_t i = 1; i < jsonLen - 1; i++) {
                        char c = json[i];
                        if (c == '\\') {
                            i++;
                            c = json[i];
                            switch (c)
                            {
                            case 'r':
                                c = '\r';
                                break;
                            case 'n':
                                c = '\n';
                                break;
                            case 't':
                                c = '\t';
                                break;
                            case 'b':
                                c = '\b';
                                break;
                            case 'f':
                                c = '\f';
                                break;
                            }
                        }
                        if (index >= expectedSize - 1) { // json 格式错误
                            return 0;
                        }
                        t.str[index] = c;
                        index++;
                    }
                    t.length = expectedSize - 1;
                    t.str[t.length] = '\0';
                    s = t;
                    return 1;
                }
            }
        }
    }
    return 0;
}

// 检查当前字符是否为字符串的开头或结尾的 "
int CheckStrHeadTail(char*& json, size_t i) {
    if (json[i] != '"') {
        return 0;
    }
    // 统计 " 前的 \ 是否为偶数个
    size_t count = 0;
    while (i > 1) {
        i--;
        if (json[i] == '\\') {
            count++;
        }
        else {
            break;
        }
    }
    return !(count % 2);
}

// {"xxx":"xxx"}
int StrMapFromJson(char* json, mapObj<stringObj>& m) {
    if (json != NULL) {
        size_t jsonLen = StrLen(json);
        if (jsonLen > 1 && json[0] == '{' && json[jsonLen - 1] == '}') {
            char* str = NULL;
            int inStr = 0;
            int findKey = 1;
            stringObj key;
            mapObj<stringObj> t;
            for (size_t i = 1; i < jsonLen - 1; i++) {
                if (CheckStrHeadTail(json, i)) { // 到达字符串的开头或结尾
                    if (!inStr) { // 到达字符串开头
                        if (json[i - 1] != '{' && json[i - 1] != ':' && json[i - 1] != ',') {
                            return 0;
                        }
                        str = json + i; // 当前要解析的字符串的指针
                        inStr = 1;
                    }
                    else { // 到达字符串结尾
                        char end = json[i + 1];
                        if (end != ':' && end != ',' && end != '}') {
                            return 0;
                        }
                        json[i + 1] = '\0';
                        if (findKey) { // 解析键
                            if (!StrFromJson(str, key)) {
                                return 0;
                            }
                            findKey = 0;
                        }
                        else { // 解析值
                            stringObj value;
                            if (!StrFromJson(str, value)) {
                                return 0;
                            }
                            t.Set(key, value);
                            findKey = 1;
                        }
                        json[i + 1] = end;
                        inStr = 0;
                    }
                }
            }
            if (inStr || !findKey) {
                return 0;
            }
            m = t;
            return 1;
        }
    }
    return 0;
}

// {"xxx":{"xxx":"xxx"}}
int MultiMapFromJson(char* json, mapObj<mapObj<stringObj>>& m) {
    if (json != NULL) {
        size_t jsonLen = StrLen(json);
        if (jsonLen > 1 && json[0] == '{' && json[jsonLen - 1] == '}') {
            char* str = NULL;
            int inStr = 0;
            int findKey = 1;
            stringObj key;
            mapObj<mapObj<stringObj>> t;
            for (size_t i = 1; i < jsonLen - 1; i++) {
                if (CheckStrHeadTail(json, i)) { // 到达字符串的开头或结尾
                    if (!inStr) { // 到达字符串开头
                        if (json[i - 1] != '{' && json[i - 1] != ':' && json[i - 1] != ',') {
                            return 0;
                        }
                        if (findKey) { // 当前要解析的键的指针
                            str = json + i;
                        }
                        inStr = 1;
                    }
                    else { // 到达字符串结尾
                        char end = json[i + 1];
                        if (end != ':' && end != ',' && end != '}') {
                            return 0;
                        }
                        if (findKey) { // 解析键
                            json[i + 1] = '\0';
                            if (!StrFromJson(str, key)) {
                                return 0;
                            }
                            json[i + 1] = end;
                            findKey = 0;
                        }
                        inStr = 0;
                    }
                }
                else if (json[i] == '{' && !inStr) { // 当前为字符串外部的 {
                    if (findKey || json[i - 1] != ':') {
                        return 0;
                    }
                    str = json + i; // 当前要解析的值的指针
                }
                else if (json[i] == '}' && !inStr) { // 当前为字符串外部的 }
                    char end = json[i + 1];
                    if (findKey || (end != ',' && end != '}')) {
                        return 0;
                    }
                    json[i + 1] = '\0';
                    mapObj<stringObj> value;
                    if (!StrMapFromJson(str, value)) { // 解析值
                        return 0;
                    }
                    t.Set(key, value);
                    json[i + 1] = end;
                    findKey = 1;
                }
            }
            if (inStr || !findKey) {
                return 0;
            }
            m = t;
            return 1;
        }
    }
    return 0;
}