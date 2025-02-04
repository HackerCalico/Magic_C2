import re
import os
import base64
import importlib
from PyQt6.QtCore import QTimer
from PyQt6.QtWidgets import QWidget, QPlainTextEdit, QVBoxLayout
from PyQt6.QtGui import QColor, QTextOption, QTextCursor, QTextCharFormat

from Public.IM import IM
from Public.Layout import Layout

class Terminal(QWidget):
    currentPath = '.'
    sessionInfo = None

    def __init__(self):
        super().__init__()
        self.setStyleSheet('background-color: black;')
        if Layout.theme == 'Light':
            self.setStyleSheet('color: white; background-color: black;')
        allHorizontal_Layout = QVBoxLayout(self)
        self.terminal_Text = QPlainTextEdit()
        self.terminal_Text.setReadOnly(True)
        self.terminal_Text.setWordWrapMode(QTextOption.WrapMode.NoWrap)
        allHorizontal_Layout.addWidget(self.terminal_Text)
        self.command_Input = Layout.AddInput('help', allHorizontal_Layout)
        self.command_Input.returnPressed.connect(self.InvokeCommand_ReturnPressed)
        QTimer.singleShot(0, lambda: IM.Send({'obj': 'Session', 'func': 'GetTerminalContent', 'paras': {'sid': self.sessionInfo['sid']}}))

    def AddContent(self, content, end='\n', send=True):
        color_Format = QTextCharFormat()
        if '[+]' in content:
            color_Format.setForeground(QColor('lime'))
        elif '[-]' in content:
            color_Format.setForeground(QColor('red'))
        elif '[*]' in content:
            color_Format.setForeground(QColor('dodgerblue'))
        elif '[!]' in content:
            color_Format.setForeground(QColor('red'))
        cursor = self.terminal_Text.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.End)
        cursor.insertText(f'{content}\n{end}', color_Format)
        self.terminal_Text.moveCursor(QTextCursor.MoveOperation.End)
        if send:
            IM.Send({'obj': 'Session', 'func': 'UpdateTerminalInfo', 'paras': {'sid': self.sessionInfo['sid'], 'content': f'{content}\n{end}'.encode('UTF-8').hex()}})

    def InvokeCommand_ReturnPressed(self):
        command = self.command_Input.text().strip()
        if not command:
            return
        self.command_Input.clear()
        self.AddContent(f'{self.currentPath}> {command}', '')
        name, paras = re.findall(r'^(\S+)(?:\s+(\S.*))?$', command)[0]
        if name == 'help':
            for script in os.listdir('Script'):
                if script != '__pycache__':
                    self.AddContent(script, '')
            self.AddContent('Please try the -h flag for these commands.')
        else:
            module = None
            try:
                module = importlib.import_module(f'Script.{name}.{name}')
            except:
                self.AddContent('[-] Command not found.')
                return
            try:
                module.Run(paras, self)
            except Exception as e:
                self.AddContent(f'[-] {e}')

    def AddCommand(self, parasInfo, needBase64=False):
        parasInfo['sid'] = self.sessionInfo['sid']
        if parasInfo['type'] == 'exeLite':
            needBase64 = True
            parasInfo['bin'] = base64.b64encode(parasInfo['bin']).decode()
        elif parasInfo['type'] == 'shellcode':
            parasInfo['bin'] = base64.b64encode(parasInfo['bin']).decode()
        if needBase64:
            parasInfo['paras'] = base64.b64encode(parasInfo['paras'].encode('UTF-8')).decode()
        IM.Send({'obj': 'Session', 'func': 'AddCommand', 'paras': parasInfo})

    def GetMultiParas(self, paras):
        paras = paras.replace('\x00', '').replace('\\"', '\x00')
        paras = re.findall(r'"([^"]*)"', paras)
        return [para.replace('\x00', '"') for para in paras]

    def GetFullPath(self, path):
        if self.sessionInfo['os'] == 'windows':
            if len(path) > 1 and path[1] == ':':
                path = path[0].upper() + path[1:]
                return os.path.normpath(path)
            else:
                return os.path.normpath(f'{self.currentPath}\\{path}')