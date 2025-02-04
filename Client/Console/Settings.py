from PyQt6.QtWidgets import QLabel, QWidget, QCheckBox, QPushButton, QHBoxLayout, QVBoxLayout

from Public.IM import IM
from Public.SQL import SQL
from Public.Layout import Layout
from Public.Message import Message

class Settings(QWidget):
    def __init__(self):
        super().__init__()
        self.setFixedSize(550, 100)
        self.setWindowTitle('Settings')

        allVertical_Layout = QVBoxLayout(self)
        themeHorizontal_Layout = QHBoxLayout()
        proxyHorizontal_Layout = QHBoxLayout()
        allVertical_Layout.addLayout(themeHorizontal_Layout)
        allVertical_Layout.addLayout(proxyHorizontal_Layout)

        theme_Checkbox = QCheckBox()
        theme_Checkbox.setChecked(True if Layout.theme == 'Dark' else False)
        theme_Checkbox.stateChanged.connect(self.ChangeTheme_StateChanged)
        theme_Checkbox.setStyleSheet('QCheckBox::indicator { width: 20px; height: 20px; }')
        themeHorizontal_Layout.addWidget(QLabel('Dark Theme'))
        themeHorizontal_Layout.addWidget(theme_Checkbox)
        themeHorizontal_Layout.addStretch(0)

        proxyHorizontal_Layout.addWidget(QLabel('Proxy:'))
        self.proxyProtocol_Input = Layout.AddInput('protocol', proxyHorizontal_Layout)
        self.proxyProtocol_Input.setText(IM.proxyProtocol)
        self.proxyHost_Input = Layout.AddInput('host', proxyHorizontal_Layout)
        self.proxyHost_Input.setText(IM.proxyHost)
        self.proxyPort_Input = Layout.AddInput('port', proxyHorizontal_Layout)
        self.proxyPort_Input.setText(IM.proxyPort)
        proxyHorizontal_Layout.addWidget(QLabel('Open'))
        self.proxyOpen_Checkbox = QCheckBox()
        self.proxyOpen_Checkbox.setChecked(True if IM.proxyOpen == 'true' else False)
        self.proxyOpen_Checkbox.setStyleSheet('QCheckBox::indicator { width: 20px; height: 20px; }')
        proxyHorizontal_Layout.addWidget(self.proxyOpen_Checkbox)
        save_Button = QPushButton('Save')
        save_Button.clicked.connect(self.Save_Clicked)
        proxyHorizontal_Layout.addWidget(save_Button)

    @staticmethod
    def SetConfig(app):
        config = SQL.SqlExec('SELECT * FROM Config', [], True)[0]
        Layout.theme = config['theme']
        Layout.SetTheme(app)
        IM.proxyProtocol = config['proxyProtocol']
        IM.proxyHost = config['proxyHost']
        IM.proxyPort = config['proxyPort']
        IM.proxyOpen = config['proxyOpen']

    def ChangeTheme_StateChanged(self, state):
        if state == 2:
            Layout.theme = 'Dark'
        else:
            Layout.theme = 'Light'
        if SQL.SqlExec('UPDATE Config SET theme=?', [Layout.theme]):
            Message.MessageBox('info', 'ChangeTheme_StateChanged', 'Please restart the client.')

    def Save_Clicked(self):
        IM.proxyProtocol = self.proxyProtocol_Input.text()
        IM.proxyHost = self.proxyHost_Input.text()
        IM.proxyPort = self.proxyPort_Input.text()
        IM.proxyOpen = 'true' if self.proxyOpen_Checkbox.isChecked() else 'false'
        SQL.SqlExec('UPDATE Config SET proxyProtocol=?, proxyHost=?, proxyPort=?, proxyOpen=?', [IM.proxyProtocol, IM.proxyHost, IM.proxyPort, IM.proxyOpen])