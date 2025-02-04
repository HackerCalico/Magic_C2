name = 'sleep'
_type = 'native'
help = 'sleep [baseTime(ms)]\nRecommended to be over 1000ms.'

def Run(paras, t):
    if paras == '-h':
        t.AddContent(help)
    elif paras.isdigit():
        t.AddCommand({'type': _type, 'func': name, 'paras': paras})
    else:
        t.AddContent(help)