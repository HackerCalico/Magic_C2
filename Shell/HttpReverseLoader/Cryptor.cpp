#include <iostream>

using namespace std;

// 加密数据
void EncryptData(char* data, int dataLength) {
	for (int i = 0; i < dataLength - 1; i++) {
		*(data + i) ^= *(data + dataLength - 1);
	}
}

// 解密数据
void DecryptData(char* data, int dataLength) {
	for (int i = 0; i < dataLength - 1; i++) {
		*(data + i) ^= *(data + dataLength - 1);
	}
}