def Run(scriptPara, scriptsPath):
    if scriptPara['scriptPara'] == 'help':
        return {'display': 'cmd [CMD命令]', 'help': ''}
    cmd = 'cmd /c ' + scriptPara['scriptPara']
    return {'display': '', 'paraHex': (cmd.encode('GBK') + b'\x00').hex()}