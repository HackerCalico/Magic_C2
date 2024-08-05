namespace Client
{
    public class PendingSessionInfo
    {
        public string sid { get; set; }
        public string publicIP { get; set; }
        public string tag { get; set; }
        public string listenerName { get; set; }
        public string connectTime { get; set; }
        public int heartbeat { get; set; }
        public string currentHeartbeat { get; set; }
        public string determineData { get; set; }
        public string pending {  get; set; }
    }
}