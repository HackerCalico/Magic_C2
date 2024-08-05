import os

def Run(scriptPara, scriptsPath):
    help = '全部命令:\n'
    for fileName in os.listdir(scriptsPath):
        if '.' not in fileName and '_' not in fileName and fileName != 'help':
            help += fileName + '\n'
    return {'display': help + '[命令名称] help\nclose 关闭当前窗口', 'help': ''}