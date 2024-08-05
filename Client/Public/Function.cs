using System;
using System.IO;
using System.Windows;
using Newtonsoft.Json;
using System.Diagnostics;
using System.Windows.Media;
using System.Windows.Controls;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Windows.Controls.Primitives;

namespace Client
{
    public class Function
    {
        private static string theme;
        private static ResourceDictionary controlsRes = new ResourceDictionary { Source = new Uri("pack://application:,,,/MahApps.Metro;component/Styles/Controls.xaml") };
        private static ResourceDictionary darkThemeRes = new ResourceDictionary { Source = new Uri("pack://application:,,,/MahApps.Metro;component/Styles/Themes/Dark.Blue.xaml") };

        // 下发命令
        public static ScriptOutputInfo IssueCommand(string scriptName, string scriptParaJsonBase64, Dictionary<string, string> postParameter)
        {
            string toolOutput = InvokeTool("python", "script\\RunScript.py \"" + scriptName + "\" " + scriptParaJsonBase64 + " \"" + Directory.GetCurrentDirectory() + "\"");
            if (toolOutput == null)
            {
                MessageBox.Show("Python 未添加至环境变量", "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                return null;
            }
            ScriptOutputInfo scriptOutput;
            try
            {
                scriptOutput = JsonConvert.DeserializeObject<ScriptOutputInfo>(toolOutput);
                if (scriptOutput == null)
                {
                    throw new Exception();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("命令脚本输出无法转为 Json\n" + ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                return null;
            }
            // 添加命令数据
            try
            {
                if (scriptOutput.selfAsm != null)
                {
                    postParameter.Add("paraHex", scriptOutput.paraHex);
                    postParameter.Add("selfAsm", scriptOutput.selfAsm);
                    postParameter.Add("selfAsmHash", scriptOutput.selfAsmHash);
                    new HttpRequest("?packageName=SessionController&structName=CommandController&funcName=AddCommandData", postParameter).GeneralRequest();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("命令脚本输出键值对不完整\n" + ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                return null;
            }
            return scriptOutput;
        }

        // 调用工具
        public static string InvokeTool(string toolName, string toolPara)
        {
            try
            {
                ProcessStartInfo processInfo = new ProcessStartInfo
                {
                    FileName = toolName,
                    Arguments = toolPara,
                    CreateNoWindow = true,
                    UseShellExecute = false,
                    RedirectStandardOutput = true
                };
                Process scriptProcess = new Process();
                scriptProcess.StartInfo = processInfo;
                scriptProcess.Start();
                string toolOutput = scriptProcess.StandardOutput.ReadToEnd();
                scriptProcess.WaitForExit();
                return toolOutput;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                return null;
            }
        }

        // 检查环境
        public static bool CheckEnvironment()
        {
            bool ok = true;
            // 检查命令脚本
            if (!File.Exists(@"script\RunScript.py"))
            {
                MessageBox.Show("未找到 " + Directory.GetCurrentDirectory() + @"\script\RunScript.py", "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                ok = false;
            }
            // 检查临时文件
            if (!File.Exists(@"tmp\tmp"))
            {
                MessageBox.Show("未找到 " + Directory.GetCurrentDirectory() + @"\tmp\tmp", "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                ok = false;
            }
            // 检查 tools
            if (!File.Exists(@"tools\ResourceHacker.exe"))
            {
                MessageBox.Show("未找到 " + Directory.GetCurrentDirectory() + @"\tools\ResourceHacker.exe", "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                ok = false;
            }
            // 检查 Python & capstone
            string toolOutput = InvokeTool("pip", "list");
            if (toolOutput == null)
            {
                MessageBox.Show("Python 未添加至环境变量", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                ok = false;
            }
            else
            {
                if (!toolOutput.Contains("capstone"))
                {
                    MessageBox.Show("未安装 Python 库 capstone", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                    ok = false;
                }
            }
            // 检查 msbuild.exe
            toolOutput = InvokeTool("msbuild", "");
            if (toolOutput == null)
            {
                MessageBox.Show("msbuild 未添加至环境变量\nC:\\Program Files\\Microsoft Visual Studio\\xxxx\\Professional\\MSBuild\\Current\\Bin", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                ok = false;
            }
            if (!ok)
            {
                MessageBox.Show("添加环境变量后请重启客户端", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
            }
            return ok;
        }

        // 设置主题
        public static string SetTheme(bool toggle, DataGrid dataGrid)
        {
            if (!toggle)
            {
                if (File.Exists(@"tmp\Light"))
                {
                    theme = "Light";
                    Application.Current.Resources.MergedDictionaries.Remove(controlsRes);
                    Application.Current.Resources.MergedDictionaries.Remove(darkThemeRes);
                }
                else
                {
                    theme = "Dark";
                    Application.Current.Resources.MergedDictionaries.Add(controlsRes);
                    Application.Current.Resources.MergedDictionaries.Add(darkThemeRes);
                }
            }
            else
            {
                if (File.Exists(@"tmp\Light"))
                {
                    theme = "Dark";
                    Application.Current.Resources.MergedDictionaries.Add(controlsRes);
                    Application.Current.Resources.MergedDictionaries.Add(darkThemeRes);
                    File.Move(@"tmp\Light", @"tmp\Dark");
                }
                else
                {
                    theme = "Light";
                    Application.Current.Resources.MergedDictionaries.Remove(controlsRes);
                    Application.Current.Resources.MergedDictionaries.Remove(darkThemeRes);
                    File.Move(@"tmp\Dark", @"tmp\Light");
                }
            }
            Style buttonStyle = new Style(typeof(Button));
            buttonStyle.Setters.Add(new Setter(Control.ForegroundProperty, new SolidColorBrush(Colors.Black)));
            Application.Current.Resources[typeof(Button)] = buttonStyle;
            if (dataGrid != null)
            {
                Style dataGridColumnHeaderStyle = new Style(typeof(DataGridColumnHeader));
                if (theme == "Light")
                {
                    dataGridColumnHeaderStyle.Setters.Add(new Setter(DataGridColumnHeader.BorderBrushProperty, Brushes.LightGray));
                    dataGridColumnHeaderStyle.Setters.Add(new Setter(DataGridColumnHeader.BorderThicknessProperty, new Thickness(1)));
                    dataGridColumnHeaderStyle.Setters.Add(new Setter(DataGridColumnHeader.ForegroundProperty, new SolidColorBrush(Colors.Black)));
                    dataGridColumnHeaderStyle.Setters.Add(new Setter(DataGridColumnHeader.BackgroundProperty, new SolidColorBrush(Colors.White)));
                    dataGridColumnHeaderStyle.Setters.Add(new Setter(DataGridColumnHeader.HorizontalContentAlignmentProperty, HorizontalAlignment.Center));
                }
                else
                {
                    dataGridColumnHeaderStyle.Setters.Add(new Setter(DataGridColumnHeader.BorderBrushProperty, Brushes.LightGray));
                    dataGridColumnHeaderStyle.Setters.Add(new Setter(DataGridColumnHeader.BorderThicknessProperty, new Thickness(1)));
                    dataGridColumnHeaderStyle.Setters.Add(new Setter(DataGridColumnHeader.ForegroundProperty, new SolidColorBrush(Colors.White)));
                    dataGridColumnHeaderStyle.Setters.Add(new Setter(DataGridColumnHeader.BackgroundProperty, new SolidColorBrush(Colors.Black)));
                    dataGridColumnHeaderStyle.Setters.Add(new Setter(DataGridColumnHeader.HorizontalContentAlignmentProperty, HorizontalAlignment.Center));
                }
                dataGrid.ColumnHeaderStyle = dataGridColumnHeaderStyle;
            }
            return theme;
        }

        // 格式化文件路径
        public static string FormatFilePath(string filePath)
        {
            filePath = Regex.Replace(filePath, "/+", "\\");
            filePath = Regex.Replace(filePath, @"\\+", "\\");
            return filePath.TrimEnd('\\');
        }

        // 格式化文件大小
        public static string FormatFileSize(string fileSize)
        {
            ulong fileSizeNum = ulong.Parse(fileSize);
            int unitIndex = 0;
            string[] unit = { " B", " KB", " MB", " GB", " TB", " PB" };
            while (fileSizeNum > 1024)
            {
                fileSizeNum = fileSizeNum / 1024;
                unitIndex++;
            }
            return fileSizeNum.ToString() + unit[unitIndex];
        }

        // 设置单选选中
        public static void SetRadioResult(WrapPanel wrapPanel, string result)
        {
            foreach (RadioButton radioButton in wrapPanel.Children)
            {
                if ((string)radioButton.Content == result)
                {
                    radioButton.IsChecked = true;
                    break;
                }
            }
        }

        // 获取单选结果
        public static string GetRadioResult(WrapPanel wrapPanel)
        {
            string result = null;
            foreach (RadioButton radioButton in wrapPanel.Children)
            {
                if (radioButton.IsChecked == true)
                {
                    result = (string)radioButton.Content;
                    break;
                }
            }
            return result;
        }
    }
}