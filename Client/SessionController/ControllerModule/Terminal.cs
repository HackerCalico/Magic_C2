using System;
using System.Text;
using System.Windows;
using Newtonsoft.Json;
using System.Data.SQLite;
using System.Windows.Media;
using System.Windows.Controls;
using System.Collections.Generic;
using System.Text.RegularExpressions;

// 命令终端
namespace Client
{
    public class Terminal
    {
        // 获取终端历史记录
        public static void GetTerminalHistory(SessionController sessionController, string selectedTerminalId = null)
        {
            int selectedTerminalIndex = -1;
            List<TabItem> terminalList = new List<TabItem>();
            try
            {
                using (SQLiteConnection conn = new SQLiteConnection(@"Data Source = config\client.db"))
                {
                    conn.Open();
                    string sql = "select * from TerminalHistory where sid=@sid";
                    using (SQLiteCommand sqlCommand = new SQLiteCommand(sql, conn))
                    {
                        sqlCommand.Parameters.AddWithValue("@sid", sessionController.selectedSessionInfo.sid);
                        using (SQLiteDataReader dataReader = sqlCommand.ExecuteReader())
                        {
                            for (int i = 0; dataReader.Read(); i++)
                            {
                                if (Convert.ToInt32(dataReader["id"]).ToString() == selectedTerminalId)
                                {
                                    selectedTerminalIndex = i;
                                }
                                TextBox commandHistory_TextBox = new TextBox()
                                {
                                    Tag = Convert.ToInt32(dataReader["id"]).ToString(),
                                    Text = (string)dataReader["command"],
                                    IsReadOnly = true,
                                    VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
                                    Foreground = Brushes.Lime,
                                    Background = Brushes.Black,
                                    Padding = new Thickness(3)
                                };
                                Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                                {
                                    commandHistory_TextBox.ScrollToEnd();
                                }), System.Windows.Threading.DispatcherPriority.Background);
                                TabItem terminal_TabItem = new TabItem()
                                {
                                    Header = sessionController.selectedSessionInfo.sid,
                                    Content = commandHistory_TextBox
                                };
                                terminal_TabItem.PreviewMouseLeftButtonDown += sessionController.ScrollTerminalWindowToEnd_PreviewMouseLeftButtonDown;
                                terminalList.Add(terminal_TabItem);
                            }
                            TabItem addTerminal_TabItem = new TabItem()
                            {
                                Header = "+"
                            };
                            addTerminal_TabItem.PreviewMouseLeftButtonDown += sessionController.AddTerminalWindow_PreviewMouseLeftButtonDown;
                            terminalList.Add(addTerminal_TabItem);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
            }

            // 设置默认选中
            if (terminalList.Count > 1)
            {
                sessionController.terminal_TabControl.ItemsSource = terminalList;
                if (selectedTerminalIndex > -1)
                {
                    sessionController.terminal_TabControl.SelectedIndex = selectedTerminalIndex;
                }
                else
                {
                    sessionController.terminal_TabControl.SelectedIndex = sessionController.terminal_TabControl.Items.Count - 2;
                }
            }
            // 创建该会话首个窗口
            else
            {
                UpdateTerminalHistory(null, sessionController.selectedSessionInfo.sid, "########################## > help ##########################", "insert into TerminalHistory (sid, command) values (@sid, @command)", sessionController);
            }
        }

        // 执行终端命令
        public static async void RunTerminalCommand(string id, string commandInput, SessionController sessionController)
        {
            string scriptName;
            string scriptPara;
            Regex regex = new Regex(@"^\s*(\S+)\s*(.*)$");
            commandInput = commandInput.Replace("\n", "");
            Match match = regex.Match(commandInput);
            if (match.Success)
            {
                scriptName = match.Groups[1].Value;
                scriptPara = match.Groups[2].Value;
            }
            else
            {
                MessageBox.Show("命令格式错误", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            // 关闭当前窗口
            if (scriptName == "close")
            {
                UpdateTerminalHistory(id, sessionController.selectedSessionInfo.sid, null, "delete from TerminalHistory where id=@id", sessionController);
                return;
            }

            // 下发命令
            Dictionary<string, string> postParameter = new Dictionary<string, string>
            {
                { "id", id },
                { "sid", sessionController.selectedSessionInfo.sid },
                { "scriptType", "Terminal" }
            };
            Dictionary<string, string> scriptParaJson = new Dictionary<string, string>
            {
                { "scriptPara", scriptPara }
            };
            ScriptOutputInfo scriptOutput = Function.IssueCommand(scriptName, Convert.ToBase64String(Encoding.UTF8.GetBytes(JsonConvert.SerializeObject(scriptParaJson))), postParameter);

            // 显示命令信息
            if (scriptOutput != null && scriptOutput.display != null)
            {
                UpdateTerminalHistory(id, sessionController.selectedSessionInfo.sid, "\n\n> " + commandInput + "\n" + scriptOutput.display, "update TerminalHistory set command=@command where id=@id", sessionController);
            }
        }

        // 更新终端历史记录
        public static void UpdateTerminalHistory(string id, string sid, string newData, string sql, SessionController sessionController)
        {
            // 终端历史 + 新数据
            string command = newData;
            if (id != null && newData != null)
            {
                try
                {
                    using (SQLiteConnection conn = new SQLiteConnection(@"Data Source = config\client.db"))
                    {
                        conn.Open();
                        using (SQLiteCommand sqlCommand = new SQLiteCommand("select command from TerminalHistory where id=@id", conn))
                        {
                            sqlCommand.Parameters.AddWithValue("@id", id);
                            using (SQLiteDataReader dataReader = sqlCommand.ExecuteReader())
                            {
                                while (dataReader.Read())
                                {
                                    command = (string)dataReader["command"] + newData;
                                }
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }

            // 更新终端历史
            try
            {
                using (SQLiteConnection conn = new SQLiteConnection(@"Data Source = config\client.db"))
                {
                    conn.Open();
                    using (SQLiteCommand sqlCommand = new SQLiteCommand(sql, conn))
                    {
                        sqlCommand.Parameters.AddWithValue("@id", id);
                        sqlCommand.Parameters.AddWithValue("@sid", sid);
                        if (newData != null)
                        {
                            sqlCommand.Parameters.AddWithValue("@command", command);
                        }
                        sqlCommand.ExecuteNonQuery();
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
            }

            if (sessionController.selectedSessionInfo != null && sessionController.selectedSessionInfo.sid == sid)
            {
                GetTerminalHistory(sessionController, id);
            }
        }
    }
}