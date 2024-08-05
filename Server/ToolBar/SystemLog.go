package ToolBar

import (
	"Server/Public"
	"github.com/gin-gonic/gin"
	"time"
)

type SystemLog struct{}

func (p SystemLog) GetSystemLogInfoList(username string, c *gin.Context) {
	systemLogInfoList := Public.SqlSelect("select * from SystemLogInfo", nil)
	c.JSON(200, systemLogInfoList)
}

// 写入系统日志
func WriteSystemLogInfo(username, content string, dataType string) {
	currentTime := time.Now().Format("2006.01.02 15:04:05")
	Public.AddNewData(dataType, username, content+" "+currentTime)
	Public.SqlExec("insert into SystemLogInfo (username, content, time) values (?, ?, ?)", username, content, currentTime)
}
