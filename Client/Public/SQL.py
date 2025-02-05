import os
import sqlite3
import appdirs

from Public.Message import Message

class SQL:
    dbPath = None

    @staticmethod
    def SqlExec(sql, args, isSelect=False):
        try:
            with sqlite3.connect(SQL.dbPath) as conn:
                cursor = conn.cursor()
                cursor.execute(sql, args)
                if isSelect:
                    columns = [description[0] for description in cursor.description]
                    return [dict(zip(columns, values)) for values in cursor.fetchall()]
                else:
                    conn.commit()
                    cursor.execute('VACUUM')
                    return True
        except Exception as e:
            Message.MessageBox('error', 'SqlExec', e)
            if isSelect:
                return []
            else:
                return False
        finally:
            if cursor:
                cursor.close()
            if conn:
                conn.close()

    @staticmethod
    def InitDatabase():
        configPath = f'{appdirs.user_data_dir()}\\Magic C2\\v2.0\\client'
        if not os.path.exists(configPath):
            disclaimer = '''1. 本项目仅用于网络安全技术的学习研究，旨在提高安全开发能力，研发新的攻防技术。
2. 若执意要将本项目用于安全业务，需先确保已获得足够的法律授权，在符合网络安全法的条件下进行。
3. 本项目由个人独立开发，暂未做全面的软件测试，请使用者在虚拟环境中测试本项目的功能，以免发生意外。
4. 本项目为远程管理软件，未包含任何恶意功能，若使用者在使用本项目的过程中存在任何违法行为或造成任何不良影响，需使用者自行承担责任，与项目作者无关。
5. 本项目完全开源，请勿将本项目用于任何商业用途。

是否同意遵守以上条款？
Do you agree to abide by the above terms?'''
            if not Message.MessageBox('choice', '免责声明 Disclaimer', disclaimer):
                return False
            os.makedirs(configPath)
        SQL.dbPath = f'{configPath}\\client.db'
        if not os.path.isfile(SQL.dbPath):
            if not SQL.SqlExec("CREATE TABLE Config (theme text DEFAULT '', proxyProtocol text DEFAULT '', proxyHost text DEFAULT '', proxyPort text DEFAULT '', proxyOpen text DEFAULT '')", []):
                os.remove(SQL.dbPath)
                return False
            if not SQL.SqlExec("INSERT INTO Config (theme, proxyProtocol, proxyHost, proxyPort, proxyOpen) VALUES ('Dark', 'http', '127.0.0.1', '8080', 'false')", []):
                os.remove(SQL.dbPath)
                return False
            if not SQL.SqlExec("CREATE TABLE LoginInfo (id integer PRIMARY KEY AUTOINCREMENT, url text DEFAULT '', username text DEFAULT '', password text DEFAULT '', cookie text DEFAULT '')", []):
                os.remove(SQL.dbPath)
                return False
            if not SQL.SqlExec("INSERT INTO LoginInfo (url, username, password) VALUES ('ws://127.0.0.1:7777', 'Magician', 'P@ssw0rd')", []):
                os.remove(SQL.dbPath)
                return False
            if not SQL.SqlExec("INSERT INTO LoginInfo (url, username, password) VALUES ('wss://127.0.0.1:7777', 'Magician', 'P@ssw0rd')", []):
                os.remove(SQL.dbPath)
                return False
        Message.PrintLog('warning', f'Plaintext storage: {SQL.dbPath}')
        return True