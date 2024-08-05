using System.Windows;
using System.Windows.Input;
using System.Windows.Controls;
using System.Collections.Generic;

// 监听配置
namespace Client
{
    public partial class ListenerConfig
    {
        private ListenerInfo selectedListenerInfo;

        public ListenerConfig()
        {
            InitializeComponent();

            Function.SetTheme(false, listenerInfoList_DataGrid);

            protocol_ComboBox.Items.Add("HTTP");
            GetListenerInfoList();
        }

        // 获取监听器信息列表
        private void GetListenerInfoList()
        {
            Dictionary<string, string> postParameter = new Dictionary<string, string> { };
            listenerInfoList_DataGrid.ItemsSource = new HttpRequest("?packageName=ToolBar&structName=ListenerConfig&funcName=GetListenerInfoList", postParameter).GetListRequest<ListenerInfo>();
        }

        // 显示监听器信息
        private void DisplayListenerInfo_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            selectedListenerInfo = (sender as DataGridRow)?.Item as ListenerInfo;
            if (selectedListenerInfo == null)
            {
                return;
            }
            name_TextBox.Text = selectedListenerInfo.name;
            description_TextBox.Text = selectedListenerInfo.description;
            protocol_ComboBox.SelectedValue = selectedListenerInfo.protocol;
            port_TextBox.Text = selectedListenerInfo.port;
            Function.SetRadioResult(connectType_WrapPanel, selectedListenerInfo.connectType);
        }

        // 增改监听器信息
        private void CuListenerInfo(string method)
        {
            // 检查表单
            string id = null;
            if (method == "UpdateListenerInfo")
            {
                if (selectedListenerInfo != null)
                {
                    id = selectedListenerInfo.id;
                }
                else
                {
                    return;
                }
            }
            string name = name_TextBox.Text;
            string description = description_TextBox.Text;
            string protocol = (string)protocol_ComboBox.SelectedValue;
            if (protocol == null)
            {
                MessageBox.Show("未选择协议", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            string port = port_TextBox.Text;
            if (port == "")
            {
                MessageBox.Show("端口不能为空", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            string connectType = Function.GetRadioResult(connectType_WrapPanel);
            if (connectType == null)
            {
                MessageBox.Show("未选择连接类型", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            if (connectType == "正向")
            {
                MessageBox.Show("暂不支持正向", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            // 名称查重
            foreach (ListenerInfo listenerInfo in listenerInfoList_DataGrid.ItemsSource)
            {
                if (listenerInfo.name == name)
                {
                    if (method == "AddListenerInfo" || method == "UpdateListenerInfo" && listenerInfo.id != id)
                    {
                        MessageBox.Show("名称不建议重复", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                        return;
                    }
                }
            }

            Dictionary<string, string> postParameter = new Dictionary<string, string>
            {
                { "name", name },
                { "description", description },
                { "protocol", protocol },
                { "port", port },
                { "connectType", connectType }
            };
            if (method == "UpdateListenerInfo")
            {
                postParameter.Add("id", id);
            }
            new HttpRequest("?packageName=ToolBar&structName=ListenerConfig&funcName=" + method, postParameter).GeneralRequest();

            GetListenerInfoList();
        }

        // 添加监听器信息
        private void AddListenerInfo_Click(object sender, RoutedEventArgs e)
        {
            CuListenerInfo("AddListenerInfo");
        }

        // 修改监听器信息
        private void UpdateListenerInfo_Click(object sender, RoutedEventArgs e)
        {
            CuListenerInfo("UpdateListenerInfo");
        }

        // 删除监听器信息
        private void DeleteListenerInfo_Click(object sender, RoutedEventArgs e)
        {
            if (selectedListenerInfo == null)
            {
                return;
            }
            if (MessageBox.Show("是否删除监听器？", "警告", MessageBoxButton.YesNo, MessageBoxImage.Warning) == MessageBoxResult.Yes)
            {
                Dictionary<string, string> postParameter = new Dictionary<string, string>
                {
                    { "id", selectedListenerInfo.id }
                };
                new HttpRequest("?packageName=ToolBar&structName=ListenerConfig&funcName=DeleteListenerInfo", postParameter).GeneralRequest();

                GetListenerInfoList();
            }
            selectedListenerInfo = null;
        }
    }
}