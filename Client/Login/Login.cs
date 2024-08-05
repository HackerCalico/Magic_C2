using System;
using System.IO;
using System.Net;
using System.Text;
using System.Windows;
using System.Data.SQLite;
using System.Windows.Input;
using System.Windows.Controls;
using System.Collections.Generic;
using System.Security.Cryptography;
using System.Collections.ObjectModel;

// 登录
namespace Client
{
    public partial class Login
    {
        private LoginInfo selectedLoginInfo;
        private ObservableCollection<LoginInfo> loginInfoList = new ObservableCollection<LoginInfo>();

        public Login()
        {
            InitializeComponent();
            WindowStartupLocation = WindowStartupLocation.CenterScreen;

            Function.SetTheme(false, null);

            GetLoginInfo();
        }

        // 获取登录信息
        private void GetLoginInfo()
        {
            try
            {
                loginInfoList.Clear();
                using (SQLiteConnection conn = new SQLiteConnection(@"Data Source = config\client.db"))
                {
                    conn.Open();
                    string sql = "select * from LoginInfo";
                    using (SQLiteCommand sqlCommand = new SQLiteCommand(sql, conn))
                    {
                        using (SQLiteDataReader dataReader = sqlCommand.ExecuteReader())
                        {
                            while (dataReader.Read())
                            {
                                LoginInfo loginInfo = new LoginInfo()
                                {
                                    id = Convert.ToInt32(dataReader["id"]).ToString(),
                                    host = (string)dataReader["host"],
                                    port = (string)dataReader["port"],
                                    accessKey = (string)dataReader["accessKey"],
                                    username = (string)dataReader["username"],
                                    password = (string)dataReader["password"]
                                };
                                loginInfoList.Add(loginInfo);
                            }
                        }
                    }
                }
                loginInfoList_DataGrid.ItemsSource = loginInfoList;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        // 显示登录信息
        private void DisplayLoginInfo_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            selectedLoginInfo = (sender as DataGridRow)?.Item as LoginInfo;
            if (selectedLoginInfo == null)
            {
                return;
            }
            host_TextBox.Text = selectedLoginInfo.host;
            port_TextBox.Text = selectedLoginInfo.port;
            accessKey_TextBox.Text = selectedLoginInfo.accessKey;
            username_TextBox.Text = selectedLoginInfo.username;
            password_PasswordBox.Password = selectedLoginInfo.password;
        }

        // 登录
        private void Login_Click(object sender, RoutedEventArgs e)
        {
            if (!File.Exists(@"C:\Windows\Temp\Magic C2 v1.0.0"))
            {
                if (!Function.CheckEnvironment())
                {
                    return;
                }
                string disclaimer = "免责声明\n1. 本项目仅用于网络安全技术的学习研究。旨在提高安全开发能力，研发新的攻防技术。\n2. 若执意要将本项目用于渗透测试等安全业务，需先确保已获得足够的法律授权，在符合网络安全法的条件下进行。\n3. 本项目由个人独立开发，暂未做全面的软件测试，请使用者在虚拟环境中测试本项目的功能。\n4. 本项目完全开源，请勿将本项目用于任何商业用途。\n5. 若使用者在使用本项目的过程中存在任何违法行为或造成任何不良影响，需使用者自行承担责任，与项目作者无关。\n\n若您同意遵守以上条款，请点击 “是”；否则请不要使用本项目。";
                if (MessageBox.Show(disclaimer, "免责声明", MessageBoxButton.YesNo, MessageBoxImage.Information) == MessageBoxResult.No)
                {
                    return;
                }
                File.Create(@"C:\Windows\Temp\Magic C2 v1.0.0");
            }

            // 发送登录请求
            HttpRequest.serverUrl = "http://" + host_TextBox.Text + ":" + port_TextBox.Text;
            HttpRequest.cookieContainer = new CookieContainer();
            HttpRequest.cookieContainer.Add(new Uri(HttpRequest.serverUrl), new Cookie("accessKey", BitConverter.ToString(MD5.Create().ComputeHash(Encoding.UTF8.GetBytes(accessKey_TextBox.Text))).Replace("-", "").ToLower()));
            HttpRequest.cookieContainer.Add(new Uri(HttpRequest.serverUrl), new Cookie("username", username_TextBox.Text));
            HttpRequest.cookieContainer.Add(new Uri(HttpRequest.serverUrl), new Cookie("password", BitConverter.ToString(MD5.Create().ComputeHash(Encoding.UTF8.GetBytes(password_PasswordBox.Password))).Replace("-", "").ToLower()));
            Dictionary<string, string> postParameter = new Dictionary<string, string> { };
            string response = Encoding.UTF8.GetString(new HttpRequest("?packageName=&structName=&funcName=Login", postParameter).GeneralRequest());
            if (response == "" || response == "HttpRequestError")
            {
                return;
            }

            // 判断登录信息是否已存在
            bool loginInfoExist = false;
            foreach (LoginInfo loginInfo in loginInfoList)
            {
                if (loginInfo.host == host_TextBox.Text && loginInfo.port == port_TextBox.Text && loginInfo.accessKey == accessKey_TextBox.Text && loginInfo.username == username_TextBox.Text && loginInfo.password == password_PasswordBox.Password)
                {
                    loginInfoExist = true;
                    break;
                }
            }

            // 记录新登录信息
            if (!loginInfoExist)
            {
                try
                {
                    using (SQLiteConnection conn = new SQLiteConnection(@"Data Source = config\client.db"))
                    {
                        conn.Open();
                        string sql = "insert into LoginInfo (host, port, accessKey, username, password) values (@host, @port, @accessKey, @username, @password)";
                        using (SQLiteCommand sqlCommand = new SQLiteCommand(sql, conn))
                        {
                            sqlCommand.Parameters.AddWithValue("@host", host_TextBox.Text);
                            sqlCommand.Parameters.AddWithValue("@port", port_TextBox.Text);
                            sqlCommand.Parameters.AddWithValue("@accessKey", accessKey_TextBox.Text);
                            sqlCommand.Parameters.AddWithValue("@username", username_TextBox.Text);
                            sqlCommand.Parameters.AddWithValue("@password", password_PasswordBox.Password);
                            sqlCommand.ExecuteNonQuery();
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }

            // 打开会话控制器
            SessionController sessionController = new SessionController();
            sessionController.Show();
            Close();
        }

        // 删除登录信息
        private void DeleteLoginInfo_Click(object sender, RoutedEventArgs e)
        {
            if (MessageBox.Show("是否删除登录信息？", "警告", MessageBoxButton.YesNo, MessageBoxImage.Warning) == MessageBoxResult.Yes)
            {
                try
                {
                    using (SQLiteConnection conn = new SQLiteConnection(@"Data Source = config\client.db"))
                    {
                        conn.Open();
                        string sql = "delete from LoginInfo where id = @id";
                        using (SQLiteCommand sqlCommand = new SQLiteCommand(sql, conn))
                        {
                            sqlCommand.Parameters.AddWithValue("@id", selectedLoginInfo.id);
                            sqlCommand.ExecuteNonQuery();
                        }
                    }
                    GetLoginInfo();
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
        }
    }
}