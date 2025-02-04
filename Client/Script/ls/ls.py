import os

name = 'ls'
_type = 'native'
help = '-re: Refresh'

def Run(paras, t):
    if paras == '-h':
        t.AddContent(help)
    else:
        currentPath = None
        if t.sessionInfo['os'] == 'windows':
            currentPath = os.path.normpath(f'{t.currentPath}\\*')
        if paras == '-re':
            t.AddCommand({'type': _type, 'func': name, 'paras': currentPath, 're': 'true'})
        else:
            t.AddCommand({'type': _type, 'func': name, 'paras': currentPath, 're': 'false'})