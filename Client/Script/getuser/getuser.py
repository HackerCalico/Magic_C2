name = 'getuser'
_type = 'native'
help = 'Prefix * indicates administrator privileges.'

def Run(paras, t):
    if paras == '-h':
        t.AddContent(help)
    else:
        t.AddCommand({'type': _type, 'func': name, 'paras': ''})