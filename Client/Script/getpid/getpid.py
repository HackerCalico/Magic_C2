name = 'getpid'
_type = 'native'
help = 'No flags.'

def Run(paras, t):
    if paras == '-h':
        t.AddContent(help)
    else:
        t.AddCommand({'type': _type, 'func': name, 'paras': ''})