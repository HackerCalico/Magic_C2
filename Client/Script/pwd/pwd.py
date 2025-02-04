name = 'pwd'
_type = 'native'
help = '-re: Refresh\nGet the absolute path to check if the current path exists.'

def Run(paras, t):
    if paras == '-h':
        t.AddContent(help)
    else:
        if paras == '-re':
            t.AddCommand({'type': _type, 'func': name, 'paras': t.currentPath, 're': 'true'})
        else:
            t.AddCommand({'type': _type, 'func': name, 'paras': t.currentPath, 're': 'false'})