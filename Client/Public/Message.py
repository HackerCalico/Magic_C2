import tkinter as tk
from tkinter import messagebox

class Message:
    @staticmethod
    def PrintLog(_type, content):
        if _type == 'info':
            print('\033[94m[*] ' + content + '\033[0m')
        elif _type == 'error':
            print('\033[31m[-] ' + content + '\033[0m')
        elif _type == 'warning':
            print('\033[31m[!] ' + content + '\033[0m')
        elif _type == 'success':
            print('\033[92m[+] ' + content + '\033[0m')

    @staticmethod
    def MessageBox(_type, title, content):
        choice = None
        try:
            if _type != 'choice':
                Message.PrintLog(_type, f'{title} - {content}')
            root = tk.Tk()
            root.withdraw()
            root.attributes('-topmost', True) # 置顶弹窗
            if _type == 'info':
                messagebox.showinfo(title, content)
            elif _type == 'error':
                messagebox.showerror(title, content)
            elif _type == 'warning':
                messagebox.showwarning(title, content)
            elif _type == 'success':
                messagebox.showinfo(title, content)
            elif _type == 'choice':
                choice = messagebox.askyesno(title, content)
            root.destroy()
        except Exception as e:
            Message.PrintLog('error', f'MessageBox - {e}')
        return choice