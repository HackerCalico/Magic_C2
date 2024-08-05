#pragma once

#include <iostream>

using namespace std;

void EncryptData(string& data);
void DecryptData(char* data, int dataLength);

char* HexToBin(string dataHex);
string BinToHex(char* data, int dataLength);

struct RefDllInfo {
	PBYTE jmpAddr;
	int dllSize;
	int dllXor;
};

RefDllInfo EncryptReflectiveDll(PBYTE jmpAddr);