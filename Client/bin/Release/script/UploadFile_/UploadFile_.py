def Run(scriptPara, scriptsPath):
    with open(scriptPara['localFilePath'], 'rb') as file:
        file.seek(int(scriptPara['readLength']))
        fileData = file.read(int(scriptPara['eachUploadSize']))
    return {'display': '', 'scriptInfo': str(len(fileData)), 'paraHex': (('0' if scriptPara['readLength'] == '0' else '1').encode() + scriptPara['targetFilePath'].encode('GBK') + b'\x00' + fileData).hex()}