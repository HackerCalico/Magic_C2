import hashlib
from PyQt6.QtGui import QIcon
from PyQt6.QtCore import Qt, QSize
from PyQt6.QtWidgets import QMenu, QLabel, QWidget, QTextEdit, QListWidget, QPushButton, QHBoxLayout, QVBoxLayout, QListWidgetItem

from Public.IM import IM
from Public.SQL import SQL
from Public.Layout import Layout
from Console.Settings import Settings
from SessionController.SessionController import SessionController

class Login(QWidget):
    def __init__(self):
        super().__init__()
        self.setFixedSize(700, 400)
        self.setWindowTitle('Magic C2 v2.0 - 𝐇𝐚𝐩𝐩𝐲 𝐍𝐞𝐰 𝐘𝐞𝐚𝐫 𝟐𝟎𝟐𝟓! 🐍')
        self.sessionController = SessionController()
        self.sessionController.loginObj = self

        allVertical_Layout = QVBoxLayout(self)
        topHorizontal_Layout = QHBoxLayout()
        bottomHorizontal_Layout = QHBoxLayout()
        allVertical_Layout.addLayout(topHorizontal_Layout, 1)
        allVertical_Layout.addLayout(bottomHorizontal_Layout, 3)

        magicC2 = '''███╗   ███╗ █████╗  ██████╗ ██╗ ██████╗    ██████╗██████╗ 
████╗ ████║██╔══██╗██╔════╝ ██║██╔════╝   ██╔════╝╚════██╗
██╔████╔██║███████║██║  ███╗██║██║        ██║      █████╔╝
██║╚██╔╝██║██╔══██║██║   ██║██║██║        ██║     ██╔═══╝ 
██║ ╚═╝ ██║██║  ██║╚██████╔╝██║╚██████╗   ╚██████╗███████╗
╚═╝     ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═╝ ╚═════╝    ╚═════╝╚══════╝'''
        magicC2_Text = QTextEdit()
        magicC2_Text.setReadOnly(True)
        magicC2_Text.setHtml(f'<pre>{magicC2}</pre>')
        magicC2_Text.setStyleSheet('background-color: transparent;')
        icon_Label = QLabel()
        icon_Label.setPixmap(QIcon('Image\\author.png').pixmap(115, 115))
        topHorizontal_Layout.addWidget(magicC2_Text, 17)
        topHorizontal_Layout.addWidget(icon_Label, 4)

        # 获取登录信息列表
        self.loginInfoList = SQL.SqlExec('SELECT * FROM LoginInfo', [], True)
        # 登录信息列表控件
        self.loginInfoList_List = QListWidget()
        self.loginInfoList_List.itemClicked.connect(self.DisplayInfo_ItemClicked)
        self.loginInfoList_List.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.loginInfoList_List.customContextMenuRequested.connect(self.InfoMenu_CustomContextMenuRequested)
        bottomHorizontal_Layout.addWidget(self.loginInfoList_List, 1)
        for info in self.loginInfoList:
            info_Item = QListWidgetItem(' ' + info['url'])
            info_Item.setSizeHint(QSize(0, 30))
            self.loginInfoList_List.addItem(info_Item)

        # 登录框控件
        vertical_Layout = QVBoxLayout()
        self.url_Input = Layout.AddIconInput('bat.png', 'url', vertical_Layout)
        self.username_Input = Layout.AddIconInput('hacker.png', 'username', vertical_Layout)
        self.password_Input = Layout.AddIconInput('password.png', 'password', vertical_Layout, True)
        self.cookie_Input = Layout.AddIconInput('cookie.png', 'cookie', vertical_Layout, True)
        login_Button = QPushButton('Login')
        login_Button.clicked.connect(self.Login_Clicked)
        login_Button.setStyleSheet('QPushButton { padding-top: 5px; padding-bottom: 7px; }')
        settings_Button = QPushButton()
        settings_Button.setIconSize(QSize(35, 35))
        settings_Button.setStyleSheet('border: none;')
        settings_Button.setIcon(QIcon('Image\\settings.png'))
        settings_Button.clicked.connect(self.Settings_Clicked)
        horizontal_Layout = QHBoxLayout()
        horizontal_Layout.addWidget(settings_Button)
        horizontal_Layout.addWidget(login_Button, 1)
        vertical_Layout.addLayout(horizontal_Layout)
        bottomHorizontal_Layout.addLayout(vertical_Layout, 2)

    def DisplayInfo_ItemClicked(self, info_Item):
        infoIndex = self.loginInfoList_List.row(info_Item)
        self.url_Input.setText(self.loginInfoList[infoIndex]['url'])
        self.url_Input.setCursorPosition(0)
        self.username_Input.setText(self.loginInfoList[infoIndex]['username'])
        self.password_Input.setText(self.loginInfoList[infoIndex]['password'])
        self.cookie_Input.setText(self.loginInfoList[infoIndex]['cookie'])
        self.cookie_Input.setCursorPosition(0)

    def InfoMenu_CustomContextMenuRequested(self, pos):
        index = self.loginInfoList_List.indexAt(pos)
        if index.isValid():
            menu = QMenu(self)
            delete_Action = menu.addAction('Delete')
            action = menu.exec(self.loginInfoList_List.mapToGlobal(pos))
            if action == delete_Action:
                if SQL.SqlExec('DELETE FROM LoginInfo WHERE id=?', [self.loginInfoList[index.row()]['id']]):
                    del self.loginInfoList[index.row()]
                    self.loginInfoList_List.takeItem(index.row())

    def Login_Clicked(self):
        IM.password = self.password_Input.text()
        IM.loginInfo = {'url': self.url_Input.text(), 'username': self.username_Input.text(), 'password': hashlib.md5(IM.password.encode()).hexdigest(), 'cookie': self.cookie_Input.text()}
        self.im = IM()
        self.im.signal.connect(self.sessionController.ParseInfo)
        self.im.start()

    def Settings_Clicked(self):
        self.settings = Settings()
        self.settings.show()