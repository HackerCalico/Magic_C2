#pragma once

void HttpRequest(char* host, int port, char* requestHeader, char* sessionInfo, int sessionInfoLength, PBYTE pAntiSandboxResult, PBYTE receiveXor1, PBYTE receiveXor2);