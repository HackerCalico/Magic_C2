name = 'info'
_type = None
help = 'No flags.'

def Run(paras, t):
    if paras == '-h':
        t.AddContent(help)
    else:
        t.AddContent(f'ARCH: {t.sessionInfo["arch"]}\nPID: {t.sessionInfo["pid"]}\nFID: {t.sessionInfo["fid"]}\nFirst connection time: {t.sessionInfo["connectTime"]}')