name = 'cd'
_type = None
help = 'cd [path]\nYou can navigate to any path, even a non-existent one, but use pwd to check if it exists.'

def Run(paras, t):
    if paras == '-h':
        t.AddContent(help)
    else:
        if t.sessionInfo['os'] == 'windows':
            paras = paras.replace('"', '')
            t.currentPath = t.GetFullPath(f'{paras}\\')
            t.AddContent(f'[*] {t.currentPath}')