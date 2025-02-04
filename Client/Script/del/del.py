name = 'del'
_type = 'native'
help = 'del [filePath]'

def Run(paras, t):
    if paras == '-h' or not paras:
        t.AddContent(help)
    else:
        if t.sessionInfo['os'] == 'windows':
            paras = paras.replace('"', '')
            path = t.GetFullPath(paras)
            t.AddCommand({'type': _type, 'func': name, 'paras': path})