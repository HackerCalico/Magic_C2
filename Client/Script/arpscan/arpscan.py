import re
import os
import base64
import hashlib

from Public.Obfuscator import Obfuscator

name = 'arpscan'
_type = 'exeLite'
help = 'arpscan scan [ip] [delay(ms)]\narpscan delay [delay(ms)]\narpscan pause\narpscan close\nReflectively load the ArpScan.exe into memory.\nDo not invoke Scan again until the current Scan is closed! ! !\nGlobal variables are not asynchronously protected. Although the error probability is low, avoid calling them frequently(Delay; Pause; Close).'

def Run(paras, t):
    if paras == '-h':
        t.AddContent(help, '')
        t.AddContent('[!] High-Risk Behavior: Frequent ARP requests.')
    else:
        if t.sessionInfo['os'] == 'windows' and t.sessionInfo['arch'] == 'x64':
            with open(f'{os.path.dirname(os.path.abspath(__file__))}\\ArpScan\\x64\\Release\\ArpScan_Base64.txt', 'r', encoding='UTF-8') as f:
                exe = bytearray(base64.b64decode(f.read().encode()))
            hash = hashlib.md5(exe).hexdigest()
            if hash in t.sessionInfo['hashInfo']:
                exe = b''
            elif not Obfuscator.OBFEXE(exe):
                return

            if paras == 'pause':
                t.AddCommand({'type': _type, 'hash': hash, 'bin': exe, 'func': 'SetPause', 'paras': '', 'async': 'false'})

            elif paras == 'close':
                t.AddCommand({'type': _type, 'hash': hash, 'bin': exe, 'func': 'SetClose', 'paras': '', 'async': 'false'})

            elif 'delay' in paras:
                delay = re.findall(r'^delay\s+(\d+)$', paras)
                if delay:
                    t.AddCommand({'type': _type, 'hash': hash, 'bin': exe, 'func': 'SetDelay', 'paras': delay[0], 'async': 'false'})
                else:
                    t.AddContent(help, '')
                    t.AddContent('[!] High-Risk Behavior: Frequent ARP requests.')

            else:
                paras = re.findall(r'^scan\s+(\d+\.\d+\.\d+\.)\d+\s+(\d+)$', paras)
                if paras:
                    ipPrefix = paras[0][0]
                    delay = paras[0][1]
                    t.AddCommand({'type': _type, 'hash': hash, 'bin': exe, 'func': 'ArpScan', 'paras': f'{ipPrefix}\x00{delay}\x00', 'async': 'true'})
                else:
                    t.AddContent(help, '')
                    t.AddContent('[!] High-Risk Behavior: Frequent ARP requests.')