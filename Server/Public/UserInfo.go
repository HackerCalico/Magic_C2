package Public

import (
	"github.com/gorilla/websocket"
	"math/rand"
	"strconv"
	"sync"
	"time"
)

type UserInfo struct {
	Password       string
	userCookieInfo map[string]string                 // {username: cookie, ...}
	userConnInfo   map[string]*websocket.Conn        // {username: conn, ...} conn 用于向用户发送信息
	userSendInfo   map[string]map[string]interface{} // {username: {type: info, ...}, ...} 准备向用户发送的不同类型的信息
	rwMutex        sync.RWMutex                      // 读写互斥锁
}

var UserInfoObj = UserInfo{
	userCookieInfo: map[string]string{},
	userConnInfo:   map[string]*websocket.Conn{},
	userSendInfo:   map[string]map[string]interface{}{},
}

// 获取 RLock 不会使其他获取了 RLock 的 Goroutine 阻塞
func (p *UserInfo) GetUserCookie(username string) string {
	p.rwMutex.RLock()
	defer p.rwMutex.RUnlock()
	return p.userCookieInfo[username]
}

// 获取 Lock 使其他获取了 Lock 和 RLock 的 Goroutine 全部阻塞
func (p *UserInfo) SetUserCookie(username, cookie string) {
	p.rwMutex.Lock()
	defer p.rwMutex.Unlock()
	p.userCookieInfo[username] = cookie
}

func (p *UserInfo) SetUserConn(username string, conn *websocket.Conn) {
	p.rwMutex.Lock()
	defer p.rwMutex.Unlock()
	p.userConnInfo[username] = conn
}

// 将信息发送至用户
func (p *UserInfo) SendToUser(username, _type string, info interface{}) {
	p.rwMutex.Lock()
	defer p.rwMutex.Unlock()
	if p.userSendInfo[username] == nil {
		p.userSendInfo[username] = map[string]interface{}{}
	}
	p.userSendInfo[username][_type] = info
	err := p.userConnInfo[username].WriteMessage(websocket.PingMessage, nil) // 检查连接是否断开
	if err != nil {
		return
	}
	err = p.userConnInfo[username].WriteJSON(p.userSendInfo[username])
	if err != nil {
		PrintLog("error", "Unable to send json to user "+username+".\n"+err.Error())
		return
	}
	p.userSendInfo[username] = map[string]interface{}{}
}

// 将信息发送至全部用户
func (p *UserInfo) SendToAllUsers(_type string, info interface{}) {
	p.rwMutex.Lock()
	defer p.rwMutex.Unlock()
	for username, _ := range p.userSendInfo {
		if p.userSendInfo[username] == nil {
			p.userSendInfo[username] = map[string]interface{}{}
		}
		p.userSendInfo[username][_type] = info
		err := p.userConnInfo[username].WriteMessage(websocket.PingMessage, nil)
		if err != nil {
			continue
		}
		err = p.userConnInfo[username].WriteJSON(p.userSendInfo[username])
		if err != nil {
			PrintLog("error", "Unable to send json to user "+username+".\n"+err.Error())
			continue
		}
		p.userSendInfo[username] = map[string]interface{}{}
	}
}

func (p *UserInfo) Login(loginInfo map[string]string, conn *websocket.Conn, ip string) bool {
	if loginInfo["username"] == "C2 Server" {
		PrintLog("warning", "Login - C2 Server cannot be used as a username.")
		return false
	}
	if loginInfo["password"] != p.Password {
		PrintLog("warning", "Login - Wrong password from "+ip)
		return false
	}
	cookie := p.GetUserCookie(loginInfo["username"])
	if cookie == "" {
		random := rand.New(rand.NewSource(time.Now().UnixNano()))
		cookie = strconv.Itoa(random.Int())
		p.SetUserCookie(loginInfo["username"], cookie)
	} else if loginInfo["cookie"] != cookie {
		PrintLog("warning", "Login - Wrong cookie from "+ip)
		return false
	}
	p.SetUserConn(loginInfo["username"], conn)
	p.SendToUser(loginInfo["username"], "cookie", cookie)
	sessionInfoList := SqlSelect("SELECT * FROM SessionInfo")
	p.SendToUser(loginInfo["username"], "sessionInfoList", sessionInfoList)
	PrintLog("success", "Login - "+loginInfo["username"])
	return true
}
