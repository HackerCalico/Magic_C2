name = 'cmd'
_type = 'native'
help = 'cmd [command]'

def Run(paras, t):
    if paras == '-h' or not paras:
        t.AddContent(help, '')
        t.AddContent('[!] High-Risk Behavior: Creating a process, especially cmd.')
    else:
        t.AddCommand({'type': _type, 'func': name, 'paras': f'cmd /c {paras}'})