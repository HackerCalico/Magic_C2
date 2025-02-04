from PyQt6.QtGui import QIcon, QColor, QAction, QPalette
from PyQt6.QtWidgets import QLabel, QLineEdit, QHBoxLayout

class Layout:
    theme = None

    @staticmethod
    def SetTheme(app):
        app.setStyle('Fusion')
        app.setWindowIcon(QIcon('Image\\logo.png'))
        app.setStyleSheet('QWidget { font-size: 11pt; }')
        if Layout.theme == 'Dark':
            palette = QPalette()
            palette.setColor(QPalette.ColorRole.Window, QColor(53, 53, 53))
            palette.setColor(QPalette.ColorRole.WindowText, QColor(255, 255, 255))
            palette.setColor(QPalette.ColorRole.Base, QColor(25, 25, 25))
            palette.setColor(QPalette.ColorRole.PlaceholderText, QColor(128, 128, 128))
            palette.setColor(QPalette.ColorRole.AlternateBase, QColor(53, 53, 53))
            palette.setColor(QPalette.ColorRole.ToolTipBase, QColor(255, 255, 255))
            palette.setColor(QPalette.ColorRole.ToolTipText, QColor(255, 255, 255))
            palette.setColor(QPalette.ColorRole.Text, QColor(255, 255, 255))
            palette.setColor(QPalette.ColorRole.Button, QColor(53, 53, 53))
            palette.setColor(QPalette.ColorRole.ButtonText, QColor(255, 255, 255))
            palette.setColor(QPalette.ColorRole.BrightText, QColor(255, 0, 0))
            palette.setColor(QPalette.ColorRole.Link, QColor(42, 87, 141))
            app.setPalette(palette)

    @staticmethod
    def AddInput(hint, layout, invisible=False):
        input_input = QLineEdit()
        input_input.setPlaceholderText(hint)
        input_input.setStyleSheet('QLineEdit { height: 35px; padding-left: 5px; }')
        if invisible:
            eye_Action = QAction(QIcon('Image\\closeEye.png'), '', layout)
            eye_Action.setCheckable(True)
            eye_Action.triggered.connect(lambda: Layout.ChangeEchoMode_Triggered(eye_Action, input_input))
            input_input.addAction(eye_Action, QLineEdit.ActionPosition.TrailingPosition)
            input_input.setEchoMode(QLineEdit.EchoMode.Password)
        layout.addWidget(input_input)
        return input_input

    @staticmethod
    def AddIconInput(image, hint, layout, invisible=False):
        # 图标
        icon_Label = QLabel()
        icon_Label.setPixmap(QIcon(f'Image\\{image}').pixmap(35, 35))
        horizontal_Layout = QHBoxLayout()
        horizontal_Layout.addWidget(icon_Label)
        # 输入框
        input_input = Layout.AddInput(hint, horizontal_Layout, invisible)
        layout.addLayout(horizontal_Layout)
        return input_input

    @staticmethod
    def ChangeEchoMode_Triggered(eye_Action, input_input):
        if eye_Action.isChecked():
            eye_Action.setIcon(QIcon('Image\\eye.png'))
            input_input.setEchoMode(QLineEdit.EchoMode.Normal)
        else:
            eye_Action.setIcon(QIcon('Image\\closeEye.png'))
            input_input.setEchoMode(QLineEdit.EchoMode.Password)