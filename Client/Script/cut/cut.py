name = 'cut'
_type = 'native'
help = 'cut "srcFilePath" "destFilePath"'

def Run(paras, t):
    if paras == '-h':
        t.AddContent(help)
    else:
        paras = t.GetMultiParas(paras)
        if len(paras) == 2:
            src = t.GetFullPath(paras[0])
            dest = t.GetFullPath(paras[1])
            t.AddCommand({'type': _type, 'func': name, 'paras': f'{src}\x00{dest}\x00'}, True)
        else:
            t.AddContent(help)