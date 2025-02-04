from PyQt6.QtWidgets import QWidget, QFileDialog

name = 'shell'
_type = 'shellcode'
help = 'shell (-t)\n-t Create thread.\nA shellcode loader. If you want to test loading Magic C2 shellcode, please use -t.'

def Run(paras, t):
    if paras == '-h':
        t.AddContent(help)
    else:
        tFlag = 'false'
        if paras == '-t':
            tFlag = 'true'
        fileDialog = QWidget()
        shellcodePath, _ = QFileDialog.getOpenFileName(fileDialog, 'ShellCode Path', '', '')
        fileDialog.deleteLater()
        if shellcodePath:
            with open(shellcodePath, 'rb') as f:
                shellcode = f.read()
            t.AddCommand({'type': _type, 'bin': shellcode, 'paras': tFlag})