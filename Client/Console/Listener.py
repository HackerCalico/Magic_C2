from PyQt6.QtCore import Qt, QSize
from PyQt6.QtWidgets import QMenu, QWidget, QComboBox, QListWidget, QPushButton, QHBoxLayout, QVBoxLayout, QListWidgetItem

from Public.IM import IM
from Public.Layout import Layout
from Public.Message import Message

class Listener(QWidget):
    def __init__(self):
        super().__init__()
        allHorizontal_Layout = QHBoxLayout(self)
        leftVertical_Layout = QVBoxLayout()
        allHorizontal_Layout.addLayout(leftVertical_Layout, 1)

        self.name_Input = Layout.AddInput('name', leftVertical_Layout)
        self.username_Input = Layout.AddInput('username', leftVertical_Layout)
        self.description_Input = Layout.AddInput('description', leftVertical_Layout)
        self.typeList_ComboBox = QComboBox()
        self.typeList_ComboBox.addItem('HTTP Reverse')
        leftVertical_Layout.addWidget(self.typeList_ComboBox)
        self.host_Input = Layout.AddInput('host', leftVertical_Layout)
        self.port_Input = Layout.AddInput('port', leftVertical_Layout)

        self.listenerInfoList_List = QListWidget()
        self.listenerInfoList_List.itemClicked.connect(self.DisplayInfo_ItemClicked)
        self.listenerInfoList_List.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.listenerInfoList_List.customContextMenuRequested.connect(self.InfoMenu_CustomContextMenuRequested)
        allHorizontal_Layout.addWidget(self.listenerInfoList_List, 1)

        add_Button = QPushButton('Add')
        add_Button.clicked.connect(self.Add_Clicked)
        save_Button = QPushButton('Save')
        save_Button.clicked.connect(self.Save_Clicked)
        horizontal_Layout = QHBoxLayout()
        horizontal_Layout.addWidget(add_Button)
        horizontal_Layout.addWidget(save_Button)
        leftVertical_Layout.addLayout(horizontal_Layout)

        IM.Send({'obj': 'ListenerConfig', 'func': 'GetListenerInfoList', 'paras': {}})

    def SetListenerInfoList(self, listenerInfoList):
        self.listenerInfoList_List.clear()
        self.listenerInfoList = listenerInfoList
        for info in self.listenerInfoList:
            info_Item = QListWidgetItem(f' {info["name"]} - {info["description"]}')
            info_Item.setSizeHint(QSize(0, 30))
            self.listenerInfoList_List.addItem(info_Item)

    def DisplayInfo_ItemClicked(self, info_Item):
        infoIndex = self.listenerInfoList_List.row(info_Item)
        self.name_Input.setText(self.listenerInfoList[infoIndex]['name'])
        self.name_Input.setCursorPosition(0)
        self.username_Input.setText(self.listenerInfoList[infoIndex]['username'])
        self.description_Input.setText(self.listenerInfoList[infoIndex]['description'])
        self.description_Input.setCursorPosition(0)
        self.typeList_ComboBox.setCurrentText(self.listenerInfoList[infoIndex]['type'])
        self.host_Input.setText(self.listenerInfoList[infoIndex]['host'])
        self.host_Input.setCursorPosition(0)
        self.port_Input.setText(self.listenerInfoList[infoIndex]['port'])

    def Add_Clicked(self):
        IM.Send({'obj': 'ListenerConfig', 'func': 'UpdateListenerInfo', 'paras': {'name': self.name_Input.text(), 'description': self.description_Input.text(), 'type': self.typeList_ComboBox.currentText(), 'host': self.host_Input.text(), 'port': self.port_Input.text(), 'action': 'add'}})

    def Save_Clicked(self):
        selectedIndexes = self.listenerInfoList_List.selectedIndexes()
        if selectedIndexes:
            origName = self.listenerInfoList[selectedIndexes[0].row()]['name']
            IM.Send({'obj': 'ListenerConfig', 'func': 'UpdateListenerInfo', 'paras': {'name': self.name_Input.text(), 'description': self.description_Input.text(), 'type': self.typeList_ComboBox.currentText(), 'host': self.host_Input.text(), 'port': self.port_Input.text(), 'origName': origName, 'action': 'save'}})
        else:
            Message.MessageBox('error', 'Save_Clicked', 'No listenerInfo selected.')

    def InfoMenu_CustomContextMenuRequested(self, pos):
        index = self.listenerInfoList_List.indexAt(pos)
        if index.isValid():
            menu = QMenu(self)
            delete_Action = menu.addAction('Delete')
            action = menu.exec(self.listenerInfoList_List.mapToGlobal(pos))
            if action == delete_Action:
                name = self.listenerInfoList[index.row()]['name']
                IM.Send({'obj': 'ListenerConfig', 'func': 'DeleteListenerInfo', 'paras': {'name': name}})