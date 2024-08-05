using System;
using System.Text;
using System.Windows;
using Newtonsoft.Json;
using System.Threading;
using System.ComponentModel;
using System.Collections.Generic;

// 分块上传
namespace Client
{
    public partial class UploadFile
    {
        private string uploadId;
        private bool threadActivePause = false;
        public static Dictionary<string, Thread> threadList = new Dictionary<string, Thread>();
        public static Dictionary<string, bool> threadPassivePauseList = new Dictionary<string, bool>();

        public UploadFile(string sid, string fileName, string localFilePath, string targetFilePath, long eachUploadSize)
        {
            InitializeComponent();

            // 设置窗口关闭函数
            Closing += WindowClosing;

            eachUploadSize_TextBox.Text = eachUploadSize.ToString();

            // 创建分块上传线程
            uploadId = new Random().Next(0, 1000000).ToString();
            Thread thread = new Thread(() => UploadFileThread(sid, fileName, localFilePath, targetFilePath, eachUploadSize));
            thread.Start();
            threadList.Add(uploadId, thread);
            threadPassivePauseList.Add(uploadId, true);
        }

        // 分块上传线程
        private void UploadFileThread(string sid, string fileName, string localFilePath, string targetFilePath, long eachUploadSize)
        {
            long fileSize = new System.IO.FileInfo(localFilePath).Length;
            string fileSizeStr = Function.FormatFileSize(fileSize.ToString());
            Application.Current.Dispatcher.BeginInvoke(new Action(() =>
            {
                fileName_Label.Content = fileName + " " + Function.FormatFileSize("0") + "/" + fileSizeStr;
            }));

            long readLength = 0;
            while (true)
            {
                Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                {
                    long checkEachUploadSize;
                    long.TryParse(eachUploadSize_TextBox.Text, out checkEachUploadSize);
                    if (checkEachUploadSize < 1 || checkEachUploadSize > 1024 * 10)
                    {
                        MessageBox.Show("每次上传大小需在 1 - 10240", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                    }
                    else
                    {
                        eachUploadSize = checkEachUploadSize;
                    }
                }));

                // 下发命令
                Dictionary<string, string> postParameter = new Dictionary<string, string>
                {
                    { "sid", sid },
                    { "uploadId", uploadId.ToString() },
                    { "commandDetail", fileName + " 上传" },
                    { "scriptType", "UploadFile" }
                };
                Dictionary<string, string> scriptPara = new Dictionary<string, string>
                {
                    { "localFilePath", localFilePath },
                    { "targetFilePath", targetFilePath },
                    { "readLength", readLength.ToString() },
                    { "eachUploadSize", eachUploadSize.ToString() }
                };
                ScriptOutputInfo scriptOutput = Function.IssueCommand("UploadFile_", Convert.ToBase64String(Encoding.UTF8.GetBytes(JsonConvert.SerializeObject(scriptPara))), postParameter);
                if (scriptOutput == null)
                {
                    return;
                }
                readLength += long.Parse(scriptOutput.scriptInfo);

                threadPassivePauseList[uploadId] = true;
                while (threadPassivePauseList[uploadId])
                {
                    Thread.Sleep(1000);
                }
                Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                {
                    fileName_Label.Content = fileName + " " + Function.FormatFileSize(readLength.ToString()) + "/" + fileSizeStr;
                }));
                if (readLength == fileSize)
                {
                    new Thread(() => WindowsNotice.SystemLogNotice(new NewData() { username = "C2 Server", content = fileName + " 上传成功" })).Start();
                    break;
                }
                while (threadActivePause)
                {
                    Thread.Sleep(1000);
                }
            }           
        }

        // 暂停上传
        private void PauseUpload_Click(object sender, RoutedEventArgs e)
        {
            PauseUpload_Button.Content = PauseUpload_Button.Content.ToString() == "暂停" ? "继续" : "暂停";
            threadActivePause = threadActivePause ? false : true;
        }

        // 终止分块上传线程
        private void WindowClosing(object sender, CancelEventArgs e)
        {
            threadList[uploadId].Abort();
        }
    }
}