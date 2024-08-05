using System;
using System.IO;
using System.Text;
using System.Windows;
using System.Threading;
using System.Data.SQLite;
using System.Windows.Input;
using System.ComponentModel;
using System.Windows.Controls;
using System.Collections.Generic;
using System.Windows.Media.Imaging;
using System.Collections.ObjectModel;

// 待定会话
namespace Client
{
    public partial class PendingSession
    {
        private Thread thread;
        private PendingSessionInfo selectedPendingSessionInfo;
        private ObservableCollection<PendingSessionInfo> pendingSessionInfoList = new ObservableCollection<PendingSessionInfo>();

        public PendingSession()
        {
            InitializeComponent();

            Function.SetTheme(false, pendingSessionInfoList_DataGrid);

            // 设置窗口关闭函数
            Closing += WindowClosing;

            // 创建获取待定会话信息列表线程
            thread = new Thread(() => GetPendingSessionInfoList());
            thread.Start();
        }

        // 获取待定会话信息列表
        private void GetPendingSessionInfoList()
        {
            while (true)
            {
                Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                {
                    try
                    {
                        pendingSessionInfoList.Clear();
                        using (SQLiteConnection conn = new SQLiteConnection(@"Data Source = config\client.db"))
                        {
                            conn.Open();
                            string sql = "select * from PendingSessionInfo order by sid";
                            using (SQLiteCommand sqlCommand = new SQLiteCommand(sql, conn))
                            {
                                using (SQLiteDataReader dataReader = sqlCommand.ExecuteReader())
                                {
                                    while (dataReader.Read())
                                    {
                                        PendingSessionInfo pendingSessionInfo = new PendingSessionInfo()
                                        {
                                            sid = (string)dataReader["sid"],
                                            publicIP = (string)dataReader["publicIP"],
                                            tag = (string)dataReader["tag"],
                                            listenerName = (string)dataReader["listenerName"],
                                            connectTime = (string)dataReader["connectTime"],
                                            heartbeat = Convert.ToInt32(dataReader["heartbeat"]),
                                            currentHeartbeat = Session.CalculateCurrentHeartbeat(Convert.ToInt32(dataReader["heartbeat"])),
                                            determineData = (string)dataReader["determineData"],
                                            pending = (string)dataReader["pending"]
                                        };
                                        pendingSessionInfoList.Add(pendingSessionInfo);
                                    }
                                }
                            }
                        }
                        pendingSessionInfoList_DataGrid.ItemsSource = pendingSessionInfoList;
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                }));
                Thread.Sleep(500);
            }
        }

        // 选中待定会话信息
        private void SelectPendingSessionInfo_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            selectedPendingSessionInfo = (sender as DataGridRow)?.Item as PendingSessionInfo;
        }

        // 显示判定数据
        private void DisplayDetData_Click(object sender, RoutedEventArgs e)
        {
            if (selectedPendingSessionInfo == null)
            {
                return;
            }
            try
            {
                using (MemoryStream ms = new MemoryStream(Convert.FromBase64String(selectedPendingSessionInfo.determineData)))
                {
                    using (System.Drawing.Image img = System.Drawing.Image.FromStream(ms))
                    {
                        ms.Position = 0;
                        Window imageWindow = new Window
                        {
                            Title = "判定图片",
                            Width = 1000,
                            Height = 560,
                            WindowStartupLocation = WindowStartupLocation.CenterScreen
                        };
                        BitmapImage bitmapImage = new BitmapImage();
                        bitmapImage.BeginInit();
                        bitmapImage.StreamSource = ms;
                        bitmapImage.EndInit();
                        Image image = new Image() { Source = bitmapImage, Stretch = System.Windows.Media.Stretch.Fill };
                        imageWindow.Content = image;
                        imageWindow.ShowDialog();
                    }
                }
            }
            catch
            {
                MessageBox.Show(Encoding.GetEncoding("GBK").GetString(Convert.FromBase64String(selectedPendingSessionInfo.determineData)), "判定数据", MessageBoxButton.OK, MessageBoxImage.Information);
            }
        }

        // 重新获取判定数据
        private void ReacquireDetData_Click(object sender, RoutedEventArgs e)
        {
            if (selectedPendingSessionInfo == null)
            {
                return;
            }
            if (MessageBox.Show("是否重新获取判定数据？", "警告", MessageBoxButton.YesNo, MessageBoxImage.Warning) == MessageBoxResult.Yes)
            {
                Dictionary<string, string> postParameter = new Dictionary<string, string>
                {
                    { "sid", selectedPendingSessionInfo.sid },
                    { "command", "ReacquireDetData" }
                };
                new HttpRequest("?packageName=ToolBar&structName=PendingSession&funcName=SetPendingSessionCommand", postParameter).GeneralRequest();
            }
        }

        // 进入正式上线阶段
        private void StartNextStage_Click(object sender, RoutedEventArgs e)
        {
            if (selectedPendingSessionInfo == null)
            {
                return;
            }
            if (MessageBox.Show("是否启动？", "警告", MessageBoxButton.YesNo, MessageBoxImage.Warning) == MessageBoxResult.Yes)
            {
                Dictionary<string, string> postParameter = new Dictionary<string, string>
                {
                    { "sid", selectedPendingSessionInfo.sid },
                    { "command", "StartNextStage" }
                };
                new HttpRequest("?packageName=ToolBar&structName=PendingSession&funcName=SetPendingSessionCommand", postParameter).GeneralRequest();
            }
        }

        // 关闭进程
        private void CloseProcess_Click(object sender, RoutedEventArgs e)
        {
            if (selectedPendingSessionInfo == null)
            {
                return;
            }
            if (MessageBox.Show("是否关闭进程？", "警告", MessageBoxButton.YesNo, MessageBoxImage.Warning) == MessageBoxResult.Yes)
            {
                Dictionary<string, string> postParameter = new Dictionary<string, string>
                {
                    { "sid", selectedPendingSessionInfo.sid },
                    { "command", "CloseProcess" }
                };
                new HttpRequest("?packageName=ToolBar&structName=PendingSession&funcName=SetPendingSessionCommand", postParameter).GeneralRequest();
            }
        }

        // 删除待定会话信息
        private void DeletePendingSessionInfo_Click(object sender, RoutedEventArgs e)
        {
            if (selectedPendingSessionInfo == null)
            {
                return;
            }
            try
            {
                using (SQLiteConnection conn = new SQLiteConnection(@"Data Source = config\client.db"))
                {
                    conn.Open();
                    string sql = "delete from PendingSessionInfo where sid=@sid";
                    using (SQLiteCommand sqlCommand = new SQLiteCommand(sql, conn))
                    {
                        sqlCommand.Parameters.AddWithValue("@sid", selectedPendingSessionInfo.sid);
                        sqlCommand.ExecuteNonQuery();
                    }
                }
                selectedPendingSessionInfo = null;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        // 添加待定会话信息
        public static void AddPendingSessionInfo(PendingSessionInfo pendingSessionInfo)
        {
            try
            {
                using (SQLiteConnection conn = new SQLiteConnection(@"Data Source = config\client.db"))
                {
                    conn.Open();
                    string sql = "replace into PendingSessionInfo (sid, publicIP, tag, listenerName, connectTime, heartbeat, determineData, pending) values (@sid, @publicIP, @tag, @listenerName, @connectTime, @heartbeat, @determineData, @pending)";
                    using (SQLiteCommand sqlCommand = new SQLiteCommand(sql, conn))
                    {
                        sqlCommand.Parameters.AddWithValue("@sid", pendingSessionInfo.sid);
                        sqlCommand.Parameters.AddWithValue("@publicIP", pendingSessionInfo.publicIP);
                        sqlCommand.Parameters.AddWithValue("@tag", pendingSessionInfo.tag);
                        sqlCommand.Parameters.AddWithValue("@listenerName", pendingSessionInfo.listenerName);
                        sqlCommand.Parameters.AddWithValue("@connectTime", pendingSessionInfo.connectTime);
                        sqlCommand.Parameters.AddWithValue("@heartbeat", pendingSessionInfo.heartbeat);
                        sqlCommand.Parameters.AddWithValue("@determineData", pendingSessionInfo.determineData);
                        sqlCommand.Parameters.AddWithValue("@pending", pendingSessionInfo.pending);
                        sqlCommand.ExecuteNonQuery();
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
        }

        // 终止获取待定会话信息列表线程
        private void WindowClosing(object sender, CancelEventArgs e)
        {
            thread.Abort();
        }
    }
}