package ToolBar

import (
	"Server/Public"
	"Server/SessionController"
	"github.com/gin-gonic/gin"
	"os"
)

type ListenerConfig struct{}

func (p ListenerConfig) GetListenerInfoList(username string, c *gin.Context) {
	listenerInfoList := Public.SqlSelect("select * from ListenerInfo", nil)
	c.JSON(200, listenerInfoList)
}

func (p ListenerConfig) AddListenerInfo(username string, c *gin.Context) {
	repeatList := Public.SqlSelect("select id from ListenerInfo where port=?", c.PostForm("port"))
	if c.PostForm("port") == os.Args[1] || len(repeatList) > 0 {
		WriteSystemLogInfo(username, "监听器添加失败: 端口重复", "SystemLog")
		c.String(200, "success")
		return
	}
	result := Public.SqlExec("insert into ListenerInfo (name, username, description, protocol, port, connectType) values (?, ?, ?, ?, ?, ?)", c.PostForm("name"), username, c.PostForm("description"), c.PostForm("protocol"), c.PostForm("port"), c.PostForm("connectType"))
	if result == "success" {
		SessionController.ListenerObj.RestartListener()
		WriteSystemLogInfo(username, "添加监听器: "+c.PostForm("name"), "SystemLog")
	}
	c.String(200, result)
}

func (p ListenerConfig) UpdateListenerInfo(username string, c *gin.Context) {
	repeatList := Public.SqlSelect("select id from ListenerInfo where port=?", c.PostForm("port"))
	if c.PostForm("port") == os.Args[1] || len(repeatList) > 0 && repeatList[0]["id"] != c.PostForm("id") {
		WriteSystemLogInfo(username, "监听器修改失败: 端口重复", "SystemLog")
		c.String(200, "success")
		return
	}
	result := Public.SqlExec("update ListenerInfo set name=?, username=?, description=?, protocol=?, port=?, connectType=? where id=?", c.PostForm("name"), username, c.PostForm("description"), c.PostForm("protocol"), c.PostForm("port"), c.PostForm("connectType"), c.PostForm("id"))
	if result == "success" {
		SessionController.ListenerObj.RestartListener()
		WriteSystemLogInfo(username, "修改监听器: "+c.PostForm("name"), "SystemLog")
	}
	c.String(200, result)
}

func (p ListenerConfig) DeleteListenerInfo(username string, c *gin.Context) {
	result := Public.SqlExec("delete from ListenerInfo where id=?", c.PostForm("id"))
	if result == "success" {
		SessionController.ListenerObj.RestartListener()
		WriteSystemLogInfo(username, "删除监听器", "SystemLog")
	}
	c.String(200, result)
}
