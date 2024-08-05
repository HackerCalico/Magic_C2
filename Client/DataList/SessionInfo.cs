namespace Client
{
    public class SessionInfo
    {
        public string fid { get; set; }
        public string sid { get; set; }
        public string publicIP { get; set; }
        public string privateIP { get; set; }
        public string username { get; set; }
        public string processName { get; set; }
        public string pid { get; set; }
        public string bit { get; set; }
        public string listenerName { get; set; }
        public string connectTime { get; set; }
        public int heartbeat { get; set; }
        public string currentHeartbeat { get; set; }
    }
}