using System;
using System.IO;
using System.Text;
using System.Windows;
using Newtonsoft.Json;
using System.Threading;
using System.ComponentModel;
using System.Collections.Generic;

// 分块下载
namespace Client
{
    public partial class DownloadFile
    {
        private string downloadId;
        private bool threadActivePause = false;
        public static Dictionary<string, Thread> threadList = new Dictionary<string, Thread>();
        public static Dictionary<string, byte[]> currentDownloadDataList = new Dictionary<string, byte[]>();

        public DownloadFile(string sid, string fileName, string fileSize, string targetFilePath, string localFilePath, long eachDownloadSize)
        {
            InitializeComponent();

            // 设置窗口关闭函数
            Closing += WindowClosing;

            eachDownloadSize_TextBox.Text = eachDownloadSize.ToString();

            // 创建分块下载线程
            downloadId = new Random().Next(0, 1000000).ToString();
            Thread thread = new Thread(() => DownloadFileThread(sid, fileName, fileSize, targetFilePath, localFilePath, eachDownloadSize));
            thread.Start();
            threadList.Add(downloadId, thread);
            currentDownloadDataList.Add(downloadId, null);
        }

        // 分块下载线程
        private void DownloadFileThread(string sid, string fileName, string fileSize, string targetFilePath, string localFilePath, long eachDownloadSize)
        {
            Application.Current.Dispatcher.BeginInvoke(new Action(() =>
            {
                fileName_Label.Content = fileName + " " + Function.FormatFileSize("0") + "/" + fileSize;
            }));

            long downloadSize = 0;
            while (true)
            {
                Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                {
                    long checkEachDownloadSize;
                    long.TryParse(eachDownloadSize_TextBox.Text, out checkEachDownloadSize);
                    if (checkEachDownloadSize < 1 || checkEachDownloadSize > 1024 * 10)
                    {
                        MessageBox.Show("每次下载大小需在 1 - 10240", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                    }
                    else
                    {
                        eachDownloadSize = checkEachDownloadSize;
                    }
                }));

                // 下发命令
                Dictionary<string, string> postParameter = new Dictionary<string, string>
                {
                    { "sid", sid },
                    { "downloadId", downloadId.ToString() },
                    { "commandDetail", fileName + " 下载" },
                    { "scriptType", "DownloadFile" }
                };
                Dictionary<string, string> scriptPara = new Dictionary<string, string>
                {
                    { "eachDownloadSize", eachDownloadSize.ToString() },
                    { "targetFilePath", targetFilePath },
                    { "downloadSize", downloadSize.ToString() }
                };
                Function.IssueCommand("DownloadFile_", Convert.ToBase64String(Encoding.UTF8.GetBytes(JsonConvert.SerializeObject(scriptPara))), postParameter);

                currentDownloadDataList[downloadId] = null;
                while (currentDownloadDataList[downloadId] == null)
                {
                    Thread.Sleep(1000);
                }
                if (downloadSize == 0)
                {
                    using (FileStream fileStream = new FileStream(localFilePath, FileMode.Create))
                    {
                        fileStream.Write(currentDownloadDataList[downloadId], 0, currentDownloadDataList[downloadId].Length);
                    }
                }
                else
                {
                    using (FileStream fileStream = new FileStream(localFilePath, FileMode.Append))
                    {
                        fileStream.Write(currentDownloadDataList[downloadId], 0, currentDownloadDataList[downloadId].Length);
                    }
                }
                downloadSize += currentDownloadDataList[downloadId].Length;
                Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                {
                    fileName_Label.Content = fileName + " " + Function.FormatFileSize(downloadSize.ToString()) + "/" + fileSize;
                }));
                while (threadActivePause)
                {
                    Thread.Sleep(1000);
                }
            }
        }

        // 暂停下载
        private void PauseDownload_Click(object sender, RoutedEventArgs e)
        {
            PauseDownload_Button.Content = PauseDownload_Button.Content.ToString() == "暂停" ? "继续" : "暂停";
            threadActivePause = threadActivePause ? false : true;
        }

        // 终止分块下载线程
        private void WindowClosing(object sender, CancelEventArgs e)
        {
            threadList[downloadId].Abort();
        }
    }
}