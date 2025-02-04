import ssl
import json
import websocket
from PyQt6.QtCore import QThread, pyqtSignal

from Public.Message import Message

class IM(QThread):
    ws = None
    password = None
    loginInfo = None
    proxyProtocol = None
    proxyHost = None
    proxyPort = None
    proxyOpen = None
    signal = pyqtSignal(dict)

    # 异步即时通信
    def run(self):
        try:
            sslopt = proxy_type = http_proxy_host = http_proxy_port = None
            if IM.proxyOpen == 'true':
                proxy_type = IM.proxyProtocol
                http_proxy_host = IM.proxyHost
                http_proxy_port = IM.proxyPort
                Message.PrintLog('info', 'If the C2 Server IP is 127.0.0.1 or localhost, use 0.0.0.0 instead of it.')
            if 'wss://' in IM.loginInfo['url']:
                ssl_context = ssl.create_default_context()
                ssl_context.check_hostname = False
                ssl_context.verify_mode = ssl.CERT_NONE
                sslopt = {'context': ssl_context}
            IM.ws = websocket.WebSocketApp(IM.loginInfo['url'], on_open=self.IM_Login, on_message=self.IM_Receive, on_error=self.IM_Error, on_close=self.IM_Close)
            IM.ws.run_forever(sslopt=sslopt, proxy_type=proxy_type, http_proxy_host=http_proxy_host, http_proxy_port=http_proxy_port)
        except Exception as e:
            Message.MessageBox('error', 'IM', e)

    # 将信息发送至服务端, 反射调用服务端函数
    @staticmethod
    def Send(info):
        IM.ws.send(json.dumps(info))

    # 初次连接发送登录信息
    def IM_Login(self, ws):
        IM.Send(IM.loginInfo)

    # 接收服务端信息, 交给 ParseInfo 处理
    def IM_Receive(self, ws, info):
        info = json.loads(info)
        if isinstance(info, dict):
            self.signal.emit(info)

    def IM_Error(self, ws, error):
        Message.MessageBox('error', 'IM_Error', error)
        ws.close()

    def IM_Close(self, ws, close_status_code, close_msg):
        if close_msg:
            Message.MessageBox('error', 'IM_Close', close_msg)
        self.signal.emit({'exit': ''})