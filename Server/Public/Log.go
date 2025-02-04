package Public

import (
	"fmt"
	"time"
)

type Log struct{}

var LogObj = Log{}

func (p *Log) GetOlderLogList(username string, paras map[string]interface{}) {
	var logList []map[string]string
	if paras["oldestLogId"] == "max" {
		logList = SqlSelect("SELECT * FROM Log ORDER BY id DESC LIMIT 50")
	} else {
		logList = SqlSelect("SELECT * FROM Log WHERE id < ? ORDER BY id DESC LIMIT 50", paras["oldestLogId"])
	}
	UserInfoObj.SendToUser(username, "olderLogList", logList)
}

func ColorPrint(_type, content string) (string, string) {
	var color string
	if _type == "info" {
		color = "\033[94m"
		content = "[*] " + content
	} else if _type == "error" {
		color = "\033[31m"
		content = "[-] " + content
	} else if _type == "warning" {
		color = "\033[31m"
		content = "[!] " + content
	} else if _type == "success" {
		color = "\033[92m"
		content = "[+] " + content
	} else if _type == "sqlError" {
		color = "\033[35m"
		content = "[-] " + content
	}
	currentTime := time.Now().Format("2006.01.02 15:04:05")
	fmt.Println(color + currentTime + " - " + content + "\033[0m")
	return currentTime, content
}

func PrintLog(_type, content string) {
	id := "???"
	currentTime, content := ColorPrint(_type, content)
	if _type == "sqlError" {
		UserInfoObj.SendToAllUsers("latestLog", map[string]string{"id": id, "time": currentTime, "content": content})
		return
	}
	err := SqlQueryRow("INSERT INTO Log (time, content) VALUES (?, ?) RETURNING id", &id, currentTime, content)
	if err == nil {
		UserInfoObj.SendToAllUsers("latestLog", map[string]string{"id": id, "time": currentTime, "content": content})
	}
}
