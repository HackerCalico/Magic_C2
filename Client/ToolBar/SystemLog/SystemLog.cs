using System.Collections.Generic;
using System.Collections.ObjectModel;

// 系统日志
namespace Client
{
    public partial class SystemLog
    {
        public SystemLog()
        {
            InitializeComponent();

            Function.SetTheme(false, systemLogInfoList_DataGrid);

            // 获取系统日志信息列表
            Dictionary<string, string> postParameter = new Dictionary<string, string> { };
            ObservableCollection<SystemLogInfo> systemLogInfoList = new HttpRequest("?packageName=ToolBar&structName=SystemLog&funcName=GetSystemLogInfoList", postParameter).GetListRequest<SystemLogInfo>();
            List<SystemLogInfo> list = new List<SystemLogInfo>(systemLogInfoList);
            list.Reverse();
            systemLogInfoList_DataGrid.ItemsSource = list;
        }
    }
}