import sys
import json
import base64
import importlib.util

from Imul import ProcessImul
from GenerateSelfAsm import FormatAsm
from Disassembly import ParseShellCode

def GetSelfAsmHash(selfAsm):
    hash = 0
    for byte in selfAsm:
        hash += ord(byte)
        hash = (hash << 8) - hash
    hash &= 0xffffffff  # 保留 32 位
    if hash & 0x80000000:  # 负数
        hash = -((~hash + 1) & 0xffffffff)
    return str(hash)

def SelfAsmConverter(shellcode):
    asm = ParseShellCode(shellcode)
    asm = ProcessImul(asm)
    return FormatAsm(asm)

if __name__ == '__main__':
    scriptName = sys.argv[1]
    scriptPara = json.loads(base64.b64decode(sys.argv[2]).decode())
    scriptPath = sys.argv[3] + '\\script\\' + scriptName + '\\' + scriptName + '.py'
    shellcodePath = sys.argv[3] + '\\script\\' + scriptName + '\\ShellCode.txt'

    try:
        # 调用命令脚本
        spec = importlib.util.spec_from_file_location('script', scriptPath)
        script = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(script)
        scriptOutput = script.Run(scriptPara, sys.argv[3] + '\\script')

        if 'help' not in scriptOutput:
            # ShellCode 转 自定义汇编指令
            with open(shellcodePath, 'r', encoding='UTF-8') as file:
                shellcode = file.read().replace(' ', '').replace('\n', '')
            selfAsm = SelfAsmConverter(shellcode)
            selfAsmHash = GetSelfAsmHash(selfAsm)

            scriptOutput['selfAsm'] = selfAsm
            scriptOutput['selfAsmHash'] = selfAsmHash

        print(scriptOutput)
    except Exception as e:
        print({'display': str(e)})

    sys.stdout.flush()