#pragma once

#include <iostream>

using namespace std;

struct SessionInfo {
    string sessionType;
    string fid;
    string sid;
    string privateIP;
    string username;
    string processName;
    string pid;
    string bit;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SessionInfo, sessionType, fid, sid, privateIP, username, processName, pid, bit)
};

struct SelfAsmInfo {
    string selfAsm;
    string selfAsmHash;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SelfAsmInfo, selfAsmHash)
};

struct ConnectData {
    string commandId;
    string paraHex;
    string data;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ConnectData, commandId, paraHex, data)
};

struct SessionData {
    SessionInfo sessionInfo;
    vector<SelfAsmInfo> selfAsmHashList;
    vector<ConnectData> commandOutputDataList;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SessionData, sessionInfo, selfAsmHashList, commandOutputDataList)
};

SessionInfo GetSessionInfo(string sessionType, string fid, string sid);

string GetSessionData(SessionInfo sessionInfo, vector<SelfAsmInfo> selfAsmInfoVector, vector<ConnectData>& commandOutputDataVector);

string GetSelfAsm(string selfAsmOrHash, vector<SelfAsmInfo>& selfAsmInfoVector);