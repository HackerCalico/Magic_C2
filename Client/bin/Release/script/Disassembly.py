import re
import binascii
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

def ParseShellCode(shellcode):
    asmByte = binascii.unhexlify(shellcode)
    cs = Cs(CS_ARCH_X86, CS_MODE_64)
    instructions = cs.disasm(asmByte, 0)

    asm = ''
    for instruction in instructions:
        mnemonic = instruction.mnemonic.replace('rep stosb', 'repStosb').replace('rep movsb', 'repMovsb')
        ops = re.sub(r'(-?\b[0-9a-fA-F]+\b)', r'0x\1', instruction.op_str) # 补全立即数 0x 前缀
        asm += hex(instruction.address) + '_' + mnemonic + '_' + ops + '\n'
    return asm