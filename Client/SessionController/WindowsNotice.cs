using System.Drawing;
using System.Windows.Forms;

// Windows 通知
namespace Client
{
    public class WindowsNotice
    {
        // 系统日志通知
        public static void SystemLogNotice(NewData newData)
        {
            NotifyIcon notifyIcon = new NotifyIcon();
            notifyIcon.Icon = SystemIcons.Information;
            notifyIcon.Visible = true;
            notifyIcon.BalloonTipTitle = newData.username;
            notifyIcon.BalloonTipText = newData.content;
            notifyIcon.ShowBalloonTip(3000);
        }
    }
}