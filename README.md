# Magic_C2

### Your star inspires me ~🌟

### 你的 Star 是我前进的动力 ~🌟

### 1. 介绍

Version: Magic C2 v2.0 Beta

项目: https://github.com/HackerCalico/Magic_C2

应网友需求以及我的时间精力状况，在开发完 v2.0 的基础功能后，我就将它发布了出来，希望你们喜欢！

![run.png](https://github.com/HackerCalico/Magic_C2/blob/main/run.png)

### 2. 项目亮点

1.完全开源，易于二次开发，源码视频讲解: (待录制)

2.RAT 具备一定免杀能力：

(1) 反射加载遵循尽可能少的调用 WinApi 的 OPSEC 原则。

(2) 默认对反射加载器的汇编进行随机混淆以及对 RAT 本体加密，支持反射加载器以内联汇编函数的形式存在 (打破了常规 ShellCode 加载流程)。

(3) 默认对 RAT 的所有字符串编译时加密。

(4) 默认在 RAT 睡眠时加密堆中的字符串。

(5) 实现了轻量级 C/C++ EXE (ExeLite) 内存加载的新机制，比 BOF 更小更容易开发。

(6) 默认对所有 WinApi 进行栈欺骗调用 (包括 ExeLite)，支持自定义 Gadget。

(7) 用本项目特有的 CLite 库完全代替 C/C++ 库，RAT 体积缩小超过 90%。

3.跨平台兼容，服务端 Go，客户端 Python 3。

### 3. 安装

1.服务端 (Go)：

在服务端目录执行以下命令：

```bash
go mod init Server
go get github.com/gin-gonic/gin
go get github.com/gorilla/websocket
go get github.com/mattn/go-sqlite3
```

go-sqlite3 需要 GCC 的解决方法：

(1) 下载 https://github.com/mstorsjo/llvm-mingw/releases 中的 llvm-mingw-2024xxxx-ucrt-x86_64

(2) 将 bin 添加至环境变量

(3) 设置 CGO_ENABLED=1

(4) 重启编译器

2.客户端 (Python 3)：

```bash
pip install PyQt6
pip install appdirs
pip install websocket-client
pip install capstone
pip install keystone-engine
```

3.RAT / ExeLite 二次开发 (C/C++)：

Visual Studio Installer ---> 单个组件 ---> LLVM (clang-cl) 和 Clang ---> 安装

### 4. 已知问题

(1) 若伪造 RAT 向服务端发送特殊构造的破坏性数据可能导致服务端瘫痪，解决方法：重写 POST 请求数据加密方式。

(2) 客户端与服务端的数据库未加密，SQLite 彻底删除数据需要在执行 DELETE 后通过 VACUUM 清除缓存。首次运行会在服务端目录生成 server.db，在 AppData\Local\Magic C2\v2.0\client 生成 client.db。

(3) 未进行全面的软件测试和压力测试，在简单的运行测试中未发现问题。

(4) 更多问题见 Issues。

### 5. 免责声明

(1) 本项目仅用于网络安全技术的学习研究，旨在提高安全开发能力，研发新的攻防技术。

(2) 若执意要将本项目用于安全业务，需先确保已获得足够的法律授权，在符合网络安全法的条件下进行。

(3) 本项目由个人独立开发，暂未做全面的软件测试，请使用者在虚拟环境中测试本项目的功能，以免发生意外。

(4) 本项目为远程管理软件，未包含任何恶意功能，若使用者在使用本项目的过程中存在任何违法行为或造成任何不良影响，需使用者自行承担责任，与项目作者无关。

(5) 本项目完全开源，请勿将本项目用于任何商业用途。