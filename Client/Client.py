from PyQt6.QtWidgets import QApplication

from Public.SQL import SQL
from Console.Login import Login
from Console.Settings import Settings

if __name__ == '__main__':
    print(r'''--/  |         _/ /  __   ___   ___     /|    _/
~'   `\.__,---'  <__/ _\_/  _\_/  _\__,/ <___/<____  Serpents in the
`\        __,--.___/ /__/  /__/  /______,----,/~~~~~  slithey toves.
  | (0) /'         |/ \___/ \___/ \ \_       `\
  |    |           `               \__^>-=
https://github.com/HackerCalico/Magic_C2''')
    if SQL.InitDatabase():
        app = QApplication(list())
        Settings.SetConfig(app)
        login = Login()
        login.show()
        app.exec()