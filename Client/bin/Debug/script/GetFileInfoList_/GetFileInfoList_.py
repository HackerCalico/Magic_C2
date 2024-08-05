def Run(scriptPara, scriptsPath):
    return {'display': '', 'paraHex': ((scriptPara['targetPath'] + '\\*').encode('GBK') + b'\x00').hex()}