# Magic_C2

### English: https://hackercalico.github.io/Magic_C2_EN.html

### 请给我 Star 🌟，非常感谢！这对我很重要！

### Please give me Star 🌟, thank you very much! It is very important to me!

### 1. 介绍

Version: Magic C2 v1.0.0 Beta

项目: https://github.com/HackerCalico/Magic_C2

我相信每一位黑客都有自己的梦想！而我的其中一个就是拥有一款自己的 C2，因为实在是太酷啦！

在首发版本的 Issues 中，我看到了大家反馈的许多问题。所以我在近两周的时间里进行了大范围的整改优化，最终得到此版本。虽然与一个成熟的 C2 框架还有一定距离，但也已经非常棒了！

![屏幕截图 2024-07-17 175750.png](https://github.com/HackerCalico/Magic_C2/blob/main/Client/bin/Debug/config/README/1.png)

### 2. 项目亮点

<mark>(1) 远程抗沙箱</mark>

后门的 ShellCode Loader 本身具备通信功能，用户可远程查看沙箱检测数据，进而远程决定后门是否从 "待定" 阶段进入 "正式上线" 阶段，如图 1。

确定进入 "正式上线" 阶段后，服务端会向后门发送 ShellCode 密钥，进而解密加载。

规避优势：

1. 即使传输与沙箱检测无关的数据，也可以利用时长绕过部分沙箱检测。

2. 因为是人工判定，所以沙箱检测数据可以比常规方式更加灵活，比如图中获取的就是目标主机的截图。

3. ShellCode 密钥由服务端发送，在病毒分析中无法通过简单的方式跳过抗沙箱阶段得到 ShellCode 明文。

<mark>(2) 隐蔽的 ShellCode 调用接口</mark>

本项目结合了 <u>No_X_Memory_ShellCode_Loader</u> 技术: https://github.com/HackerCalico/No_X_Memory_ShellCode_Loader

后门进入 "正式上线" 阶段后的所有非通信功能均通过 "自定义汇编解释器" 运行：

1. 客户端将功能 ShellCode 转为 "自定义汇编指令" ------> 服务端

2. 服务端 ------> 后门 "自定义汇编解释器"

规避优势：

1. 向后门注入任何新功能无需进行任何内存属性 (R/W/X) 修改操作。

2. 内存中任何时候都不会出现新功能的 ShellCode 机器码。

<mark>(3) 告警通知</mark>

服务端在处理无效的凭证或请求数据时，只会响应空白 / 404，并向所有客户端告警，如图 1。

<mark>(4) 文件分块传输</mark>

文件上传下载均采用分块传输，支持随时暂停、取消与修改块的大小。

![7.png](https://github.com/HackerCalico/Magic_C2/blob/main/Client/bin/Debug/config/README/2.png)

<mark>(5) 二次开发</mark>

项目完全开源，代码简洁，未使用高级语法，插件均为 Python，易于二次开发。

### 3. 二次开发

<mark>(1) 插件开发</mark>

在 Client\script 文件夹中可以看到所有插件。

```shell
Client
|— script
    |— cmd
    |— inject
    |— help
    |— GetFileInfoList_
    |— UploadFile_
    ......
```

其中所有非 "_" 结尾的插件均为 "命令终端" 模块的插件，模块如图。

所有插件功能均通过 "自定义汇编解释器" 运行，所以必须先学习 https://github.com/HackerCalico/No_X_Memory_ShellCode_Loader

Python 编写规范可参考 cmd 插件。

![2.png](https://github.com/HackerCalico/Magic_C2/blob/main/Client/bin/Debug/config/README/3.png)

目前其他 "_" 结尾的插件均为 "文件管理" 模块的插件，模块如图。

![3.png](https://github.com/HackerCalico/Magic_C2/blob/main/Client/bin/Debug/config/README/4.png)

<mark>(2) 后门开发</mark>

在 Shell 文件夹中可以看到 "HTTP 反向 Shell" 和 "HTTP 反向 Loader" 的源码项目。

"HTTP 反向 Shell" 为核心功能的 "反射 DLL ShellCode" 项目。

"HTTP 反向 Loader" 为其 "ShellCode Loader" 项目，包含了 "远程抗沙箱" 功能。

```shell
Shell
|— HttpReverseShell
    |— HttpReverseShell.sln
|— HttpReverseLoader
    |— HttpReverseLoader.sln
```

ShellCode Loader 生命周期：

![屏幕截图 2024-07-16 221607.png](https://github.com/HackerCalico/Magic_C2/blob/main/Client/bin/Debug/config/README/5.png)

ShellCode 生命周期：

![屏幕截图 2024-07-16 221608.png](https://github.com/HackerCalico/Magic_C2/blob/main/Client/bin/Debug/config/README/6.png)

<mark>(3) 生成器开发</mark>

在 Shell\Generator 文件夹中可以看到一个 NormalXor 生成器文件夹，NormalXor 文件夹中有一个 Generator.py 和一个 Profile.txt。

```shell
Shell
|— Generator
    |— NormalXor
        |— Generator.py
        |— Profile.txt
```

在使用 "后门生成" 模块时，只要选择 Profile.txt，客户端就会调用与 Profile.txt 同文件夹下的 Generator.py 对后门源码进行 修改代码 - 编译代码 - 还原代码，来生成 ShellCode、EXE。

用户可以在 Generator 文件夹中创建不同的生成器文件夹，也可以在一个生成器文件夹中创建不同 Profile.txt。

<mark>(4) 更多二次开发</mark>

可以结合 "命令控制模型" (客户端 - 服务端 - 后门) 对项目源码进行更深的理解。

![屏幕截图 2024-07-16 221156.png](https://github.com/HackerCalico/Magic_C2/blob/main/Client/bin/Debug/config/README/7.png)

### 4. 免责声明

(1) 本项目仅用于网络安全技术的学习研究。旨在提高安全开发能力，研发新的攻防技术。

(2) 若执意要将本项目用于渗透测试等安全业务，需先确保已获得足够的法律授权，在符合网络安全法的条件下进行。

(3) 本项目由个人独立开发，暂未做全面的软件测试，请使用者在虚拟环境中测试本项目的功能。

(4) 本项目完全开源，请勿将本项目用于任何商业用途。

(5) 若使用者在使用本项目的过程中存在任何违法行为或造成任何不良影响，需使用者自行承担责任，与项目作者无关。
