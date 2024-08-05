def Run(scriptPara, scriptsPath):
    cutOrCopy = '0' if scriptPara['cutOrCopy'] == 'cut' else '1'
    return {'display': '', 'paraHex': (cutOrCopy.encode() + scriptPara['cutOrCopyFilePath'].encode('GBK') + b'\x00' + scriptPara['targetPath'].encode('GBK') + b'\x00').hex()}