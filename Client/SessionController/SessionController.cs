using System;
using System.IO;
using System.Text;
using System.Windows;
using Microsoft.Win32;
using Newtonsoft.Json;
using System.Threading;
using System.Data.SQLite;
using System.Windows.Input;
using System.ComponentModel;
using Microsoft.VisualBasic;
using System.Windows.Controls;
using System.Collections.Generic;
using System.Collections.ObjectModel;

// 会话控制器
namespace Client
{
    public partial class SessionController
    {
        public SessionInfo selectedSessionInfo;
        public ObservableCollection<SessionInfo> sessionInfoList = new ObservableCollection<SessionInfo>();

        private string selectedTerminalId;
        public FileInfo rootFileInfo;
        private FileInfo selectedMenuFileInfo;
        private string cutOrCopy;
        private string cutOrCopyFilePath;

        public SessionController()
        {
            InitializeComponent();

            string theme = Function.SetTheme(false, sessionInfoList_DataGrid);

            VersionInfo_WebBrowser.Source = new Uri(Directory.GetCurrentDirectory() + @"\config\README\README_" + theme + ".html");

            // 设置窗口关闭函数
            Closing += WindowClosing;

            // 创建获取正式上线会话信息列表线程
            new Thread(() => Session.GetSessionInfoList(this)).Start();

            // 创建数据更新线程
            new Thread(() => Update.GetNewDataList(this)).Start();
        }

        // 打开工具栏工具
        private void OpenTool_Click(object sender, RoutedEventArgs e)
        {
            // 反射加载对象
            try
            {
                string toolName = (sender as Button)?.Name;
                if (toolName == null)
                {
                    return;
                }
                else if (toolName == "SystemLog")
                {
                    alarm.Visibility = Visibility.Collapsed;
                }
                else if (toolName == "ToggleTheme")
                {
                    string theme = Function.SetTheme(true, sessionInfoList_DataGrid);
                    VersionInfo_WebBrowser.Source = new Uri(Directory.GetCurrentDirectory() + @"\config\README\README_" + theme + ".html");
                    return;
                }
                Type toolClass = Type.GetType("Client." + toolName);
                Window toolWindow = (Window)Activator.CreateInstance(toolClass, null);
                toolWindow.Owner = this;
                toolWindow.WindowStartupLocation = WindowStartupLocation.CenterOwner;
                toolWindow.ShowDialog();
            }
            catch { }
        }

        // 选中正式上线会话信息
        private void SelectSessionInfo_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            selectedSessionInfo = (sender as DataGridRow)?.Item as SessionInfo;

            // 更新控制模块
            TabItem selectedTabItem = controlModule_TabControl.SelectedItem as TabItem;
            if (selectedSessionInfo != null && selectedTabItem != null)
            {
                selectedTerminalId = null;
                dirTree_TreeView.ItemsSource = null;
                fileInfoList_DataGrid.ItemsSource = null;
                rootFileInfo = null;
                selectedMenuFileInfo = null;
                cutOrCopy = null;
                cutOrCopyFilePath = null;
                switch (selectedTabItem.Header.ToString())
                {
                    // 获取终端历史记录
                    case "命令终端":
                        Terminal.GetTerminalHistory(this);
                        break;
                    // 获取文件管理信息
                    case "文件管理":
                        FileManager.GetFileManagerInfo(this);
                        break;
                }
            }
        }

        // 删除正式上线会话信息
        private void DeleteSessionInfo_PreviewMouseLeftButtonDown(object sender, RoutedEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                return;
            }
            try
            {
                using (SQLiteConnection conn = new SQLiteConnection(@"Data Source = config\client.db"))
                {
                    conn.Open();
                    string sql = "delete from SessionInfo where sid=@sid";
                    using (SQLiteCommand sqlCommand = new SQLiteCommand(sql, conn))
                    {
                        sqlCommand.Parameters.AddWithValue("@sid", selectedSessionInfo.sid);
                        sqlCommand.ExecuteNonQuery();
                    }
                }
                selectedSessionInfo = null;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        // 选中命令终端模块
        private void SelectTerminalTabItem_PreviewMouseLeftButtonDown(object sender, RoutedEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                MessageBox.Show("未选择会话", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            if (e.Source is TabItem sourceTabItem && sourceTabItem == terminal_TabItem)
            {
                Terminal.GetTerminalHistory(this);
            }
        }

        // 选中子命令终端窗口
        private void SelectSubTerminalTabItem_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            TabItem terminal_TabItem = (sender as TabControl)?.SelectedItem as TabItem;
            if (terminal_TabItem != null)
            {
                TextBox commandHistory_TextBox = terminal_TabItem.Content as TextBox;
                if (commandHistory_TextBox != null)
                {
                    selectedTerminalId = commandHistory_TextBox.Tag.ToString();
                }
            }
        }

        // 滑动终端窗口至底
        public void ScrollTerminalWindowToEnd_PreviewMouseLeftButtonDown(object sender, RoutedEventArgs e)
        {
            TabItem terminal_TabItem = sender as TabItem;
            if (terminal_TabItem != null)
            {
                TextBox commandHistory_TextBox = terminal_TabItem.Content as TextBox;
                if (commandHistory_TextBox != null)
                {
                    Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                    {
                        commandHistory_TextBox.ScrollToEnd();
                    }), System.Windows.Threading.DispatcherPriority.Background);
                }
            }
        }

        // 添加命令终端窗口
        public void AddTerminalWindow_PreviewMouseLeftButtonDown(object sender, RoutedEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                MessageBox.Show("未选择会话", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            Terminal.UpdateTerminalHistory(null, selectedSessionInfo.sid, "########################## > help ##########################", "insert into TerminalHistory (sid, command) values (@sid, @command)", this);
        }

        // 执行终端命令
        public void RunTerminalCommand_KeyDown(object sender, KeyEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                MessageBox.Show("未选择会话", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            if (e.Key == Key.Enter)
            {
                TextBox commandInput_TextBox = sender as TextBox;
                if (selectedTerminalId != null && commandInput_TextBox != null)
                {
                    string commandInput = commandInput_TextBox.Text;
                    Terminal.RunTerminalCommand(selectedTerminalId, commandInput, this);
                    commandInput_TextBox.Text = "";
                }
            }
        }

        // 选中文件管理模块
        private void SelectFileManagerTabItem_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                MessageBox.Show("未选择会话", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            if (e.Source is TabItem sourceTabItem && sourceTabItem == fileManager_TabItem)
            {
                FileManager.GetFileManagerInfo(this);
            }
        }

        // 获取输入框路径的文件信息列表
        private void GetInputPathFileInfoList_Click(object sender, RoutedEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                MessageBox.Show("未选择会话", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            if (inputPath_TextBox.Text == "")
            {
                MessageBox.Show("文件路径为空", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            FileManager.GetFileInfoList(selectedSessionInfo.sid, Function.FormatFilePath(inputPath_TextBox.Text), this);
        }

        // 获取文件目录树选中路径的文件信息列表
        private void GetDirTreePathFileInfoList_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                MessageBox.Show("未选择会话", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            FileInfo selectedFileInfo = ((sender as TextBox)?.DataContext as FileInfo);
            if (selectedFileInfo == null || selectedFileInfo.fileType != "📁")
            {
                return;
            }
            FileManager.GetFileInfoList(selectedSessionInfo.sid, selectedFileInfo.absolutePath.Substring(1), this);
        }

        // 获取文件信息列表选中路径的文件信息列表
        private void GetFileInfoListPathFileInfoList_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                MessageBox.Show("未选择会话", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            FileInfo selectedFileInfo = ((sender as DataGridRow)?.Item as FileInfo);
            if (selectedFileInfo == null || selectedFileInfo.fileType != "📁")
            {
                return;
            }
            FileManager.GetFileInfoList(selectedSessionInfo.sid, selectedFileInfo.absolutePath.Substring(1), this);
        }

        // 通过文件菜单选中文件信息
        private void SelectFileInfoFromMenu(object sender, ContextMenuEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                MessageBox.Show("未选择会话", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            selectedMenuFileInfo = ((sender as DataGridRow)?.Item as FileInfo);
        }

        // 分块上传
        private void UploadFile_Click(object sender, RoutedEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                MessageBox.Show("未选择会话", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            if (inputPath_TextBox.Text == "")
            {
                MessageBox.Show("文件路径为空", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.Title = "选择要上传的文件";
            openFileDialog.Filter = "(*.*) | *.*";
            if (openFileDialog.ShowDialog() == true)
            {
                long eachUploadSize;
                long.TryParse(Interaction.InputBox("每次上传大小", "分块上传", "1024"), out eachUploadSize);
                if (eachUploadSize < 1 || eachUploadSize > 1024 * 10)
                {
                    MessageBox.Show("每次上传大小需在 1 - 10240", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                    return;
                }
                string fileName = Path.GetFileName(openFileDialog.FileName);
                UploadFile uploadFileWindow = new UploadFile(selectedSessionInfo.sid, fileName, openFileDialog.FileName, Function.FormatFilePath(inputPath_TextBox.Text) + "\\" + fileName, eachUploadSize);
                uploadFileWindow.WindowStartupLocation = WindowStartupLocation.CenterOwner;
                uploadFileWindow.Show();
            }
        }

        // 分块下载
        private void DownloadFile_Click(object sender, RoutedEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                MessageBox.Show("未选择会话", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            if (selectedMenuFileInfo == null || selectedMenuFileInfo.fileType != "📄")
            {
                MessageBox.Show("未选择文件", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            string fileName = Path.GetFileName(selectedMenuFileInfo.absolutePath.Substring(1));
            SaveFileDialog saveFileDialog = new SaveFileDialog();
            saveFileDialog.Title = "选择下载到的路径";
            saveFileDialog.Filter = "(*.*) | *.*";
            saveFileDialog.FileName = fileName;
            if (saveFileDialog.ShowDialog() == true)
            {
                long eachDownloadSize;
                long.TryParse(Interaction.InputBox("每次下载大小", "分块上传", "1024"), out eachDownloadSize);
                if (eachDownloadSize < 1 || eachDownloadSize > 1024 * 10)
                {
                    MessageBox.Show("每次下载大小需在 1 - 10240", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                    return;
                }
                DownloadFile downloadFileWindow = new DownloadFile(selectedSessionInfo.sid, fileName, selectedMenuFileInfo.fileSize, selectedMenuFileInfo.absolutePath.Substring(1), saveFileDialog.FileName, eachDownloadSize);
                downloadFileWindow.WindowStartupLocation = WindowStartupLocation.CenterOwner;
                downloadFileWindow.Show();
            }
        }

        // 剪切文件
        private void CutFile_Click(object sender, RoutedEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                MessageBox.Show("未选择会话", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            if (selectedMenuFileInfo == null || selectedMenuFileInfo.fileType != "📄")
            {
                MessageBox.Show("未选择文件", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            cutOrCopy = "cut";
            cutOrCopyFilePath = selectedMenuFileInfo.absolutePath.Substring(1);
        }

        // 复制文件
        private void CopyFile_Click(object sender, RoutedEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                MessageBox.Show("未选择会话", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            if (selectedMenuFileInfo == null || selectedMenuFileInfo.fileType != "📄")
            {
                MessageBox.Show("未选择文件", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            cutOrCopy = "copy";
            cutOrCopyFilePath = selectedMenuFileInfo.absolutePath.Substring(1);
        }

        // 粘贴文件
        private void PasteFile_Click(object sender, RoutedEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                MessageBox.Show("未选择会话", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            if (inputPath_TextBox.Text == "")
            {
                MessageBox.Show("文件路径为空", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            if (cutOrCopy == null || cutOrCopyFilePath == null)
            {
                MessageBox.Show("未选择剪切/复制文件", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            // 下发命令
            string fileName = Path.GetFileName(cutOrCopyFilePath);
            Dictionary<string, string> postParameter = new Dictionary<string, string>
            {
                { "sid", selectedSessionInfo.sid },
                { "commandDetail", cutOrCopy == "cut" ? fileName + " 剪切" : fileName + " 复制" },
                { "scriptType", "ZeroOrOne" }
            };
            Dictionary<string, string> scriptPara = new Dictionary<string, string>
            {
                { "cutOrCopy", cutOrCopy },
                { "cutOrCopyFilePath", cutOrCopyFilePath },
                { "targetPath", Function.FormatFilePath(inputPath_TextBox.Text) + "\\" + fileName }
            };
            Function.IssueCommand("PasteFile_", Convert.ToBase64String(Encoding.UTF8.GetBytes(JsonConvert.SerializeObject(scriptPara))), postParameter);
        }

        // 删除文件
        private void DeleteFile_Click(object sender, RoutedEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                MessageBox.Show("未选择会话", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            if (selectedMenuFileInfo == null || selectedMenuFileInfo.fileType != "📄")
            {
                MessageBox.Show("未选择文件", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            // 下发命令
            if (MessageBox.Show("是否删除文件", "警告", MessageBoxButton.YesNo, MessageBoxImage.Warning) == MessageBoxResult.Yes)
            {
                string fileName = Path.GetFileName(selectedMenuFileInfo.absolutePath.Substring(1));
                Dictionary<string, string> postParameter = new Dictionary<string, string>
                {
                    { "sid", selectedSessionInfo.sid },
                    { "commandDetail", fileName + " 删除" },
                    { "scriptType", "ZeroOrOne" }
                };
                Dictionary<string, string> scriptPara = new Dictionary<string, string>
                {
                    { "filePath", selectedMenuFileInfo.absolutePath.Substring(1) }
                };
                Function.IssueCommand("DeleteFile_", Convert.ToBase64String(Encoding.UTF8.GetBytes(JsonConvert.SerializeObject(scriptPara))), postParameter);
            }
        }

        // 新建文件夹
        private void CreateFolder_Click(object sender, RoutedEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                MessageBox.Show("未选择会话", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            if (inputPath_TextBox.Text == "")
            {
                MessageBox.Show("文件路径为空", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            string folderName = Interaction.InputBox("文件夹名称:", "新建文件夹", "");
            if (folderName == "")
            {
                MessageBox.Show("未填写文件夹名称", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            // 下发命令
            Dictionary<string, string> postParameter = new Dictionary<string, string>
            {
                { "sid", selectedSessionInfo.sid },
                { "commandDetail", folderName + " 创建" },
                { "scriptType", "ZeroOrOne" }
            };
            Dictionary<string, string> scriptPara = new Dictionary<string, string>
            {
                { "filePath", Function.FormatFilePath(inputPath_TextBox.Text) + "\\" + folderName }
            };
            Function.IssueCommand("CreateFolder_", Convert.ToBase64String(Encoding.UTF8.GetBytes(JsonConvert.SerializeObject(scriptPara))), postParameter);
        }

        // 重新获取文件信息列表
        private void ReGetFileInfoList_Click(object sender, RoutedEventArgs e)
        {
            if (selectedSessionInfo == null)
            {
                MessageBox.Show("未选择会话", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            if (inputPath_TextBox.Text == "")
            {
                MessageBox.Show("文件路径为空", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            // 下发命令
            string targetPath = Function.FormatFilePath(inputPath_TextBox.Text);
            Dictionary<string, string> postParameter = new Dictionary<string, string>
            {
                { "sid", selectedSessionInfo.sid },
                { "targetPath", targetPath },
                { "scriptType", "GetFileInfoList" }
            };
            Dictionary<string, string> scriptPara = new Dictionary<string, string>
            {
                { "targetPath", targetPath }
            };
            Function.IssueCommand("GetFileInfoList_", Convert.ToBase64String(Encoding.UTF8.GetBytes(JsonConvert.SerializeObject(scriptPara))), postParameter);
        }

        // 终止数据更新线程
        private void WindowClosing(object sender, CancelEventArgs e)
        {
            Update.threadClose = true;
        }
    }
}