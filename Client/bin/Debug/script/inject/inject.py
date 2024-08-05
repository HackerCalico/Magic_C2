def Run(scriptPara, scriptsPath):
    if scriptPara['scriptPara'] == 'help':
        return {'display': 'inject [pid]', 'help': ''}
    return {'display': '', 'paraHex': (scriptPara['scriptPara'].encode() + b'\x00').hex()}