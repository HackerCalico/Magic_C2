from PyQt6.QtWidgets import QWidget, QPlainTextEdit, QVBoxLayout
from PyQt6.QtGui import QColor, QTextCursor, QTextOption, QTextCharFormat

from Public.IM import IM

class Log(QWidget):
    def __init__(self):
        super().__init__()
        allHorizontal_Layout = QVBoxLayout(self)
        self.setStyleSheet('background-color: black;')
        self.log_Text = Log_Text()
        allHorizontal_Layout.addWidget(self.log_Text)
        IM.Send({'obj': 'Log', 'func': 'GetOlderLogList', 'paras': {'oldestLogId': self.log_Text.oldestLogId}})

    def GetColorFormat(self, log):
        color_Format = QTextCharFormat()
        if '[+]' in log['content']:
            color_Format.setForeground(QColor('lime'))
        elif '[-]' in log['content']:
            color_Format.setForeground(QColor('red'))
        elif '[*]' in log['content']:
            color_Format.setForeground(QColor('dodgerblue'))
        elif '[!]' in log['content']:
            color_Format.setForeground(QColor('red'))
        return color_Format

    def UpdateOlderLogList(self, logList):
        for log in logList:
            color_Format = self.GetColorFormat(log)
            cursor = self.log_Text.textCursor()
            cursor.movePosition(QTextCursor.MoveOperation.Start)
            cursor.insertText(f'{log["id"]} - {log["time"]} - {log["content"]}\n', color_Format)
            self.log_Text.oldestLogId = log['id']
        self.log_Text.verticalScrollBar().setValue(self.log_Text.verticalScrollBar().value() + len(logList))

    def AddLatestLog(self, log):
        color_Format = self.GetColorFormat(log)
        cursor = self.log_Text.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.End)
        cursor.insertText(f'{log["id"]} - {log["time"]} - {log["content"]}\n', color_Format)
        self.log_Text.verticalScrollBar().setValue(self.log_Text.verticalScrollBar().maximum())

class Log_Text(QPlainTextEdit):
    oldestLogId = 'max'

    def __init__(self):
        super().__init__()
        self.setReadOnly(True)
        self.setWordWrapMode(QTextOption.WrapMode.NoWrap)

    def wheelEvent(self, event):
        # 上滑加载更多日志
        if self.verticalScrollBar().value() == 0 and event.angleDelta().y() > 0:
            IM.Send({'obj': 'Log', 'func': 'GetOlderLogList', 'paras': {'oldestLogId': self.oldestLogId}})
        super().wheelEvent(event)