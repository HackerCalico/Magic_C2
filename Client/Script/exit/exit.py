name = 'exit'
_type = 'native'
help = 'Close RAT.'

def Run(paras, t):
    if paras == '-h':
        t.AddContent(help)
    else:
        t.AddCommand({'type': _type, 'func': name, 'paras': ''})