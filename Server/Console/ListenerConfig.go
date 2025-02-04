package Console

import (
	"Server/CommandControl"
	"Server/Public"
)

type ListenerConfig struct{}

var ListenerConfigObj = ListenerConfig{}

func (p *ListenerConfig) GetListenerInfoList(username string, paras map[string]interface{}) {
	listenerInfoList := Public.SqlSelect("SELECT * FROM ListenerInfo")
	Public.UserInfoObj.SendToUser(username, "listenerInfoList", listenerInfoList)
}

func (p *ListenerConfig) GetListenerNameList(username string, paras map[string]interface{}) {
	listenerNameList := Public.SqlSelect("SELECT name FROM ListenerInfo")
	Public.UserInfoObj.SendToUser(username, "listenerNameList", listenerNameList)
}

func (p *ListenerConfig) UpdateListenerInfo(username string, paras map[string]interface{}) {
	if paras["action"] == "save" {
		err := Public.SqlExec("UPDATE ListenerInfo SET name=?, username=?, description=?, type=?, host=?, port=? WHERE name=?", paras["name"], username, paras["description"], paras["type"], paras["host"], paras["port"], paras["origName"])
		if err == nil {
			p.GetListenerInfoList(username, nil)
			Public.PrintLog("success", username+" has changed the info of the listener "+paras["origName"].(string)+" -> "+paras["name"].(string)+".")
			if p.CloseListener(username, paras["origName"].(string)) {
				listenerInfoList := Public.SqlSelect("SELECT * FROM ListenerInfo WHERE name=?", paras["name"])
				if len(listenerInfoList) > 0 {
					CommandControl.ListenerObj.StartListener(username, listenerInfoList[0])
				}
			}
		}
	} else if paras["action"] == "add" {
		err := Public.SqlExec("INSERT INTO ListenerInfo (name, username, description, type, host, port) VALUES (?, ?, ?, ?, ?, ?)", paras["name"], username, paras["description"], paras["type"], paras["host"], paras["port"])
		if err == nil {
			p.GetListenerInfoList(username, nil)
			listenerInfoList := Public.SqlSelect("SELECT * FROM ListenerInfo WHERE name=?", paras["name"])
			if len(listenerInfoList) > 0 {
				CommandControl.ListenerObj.StartListener(username, listenerInfoList[0])
			}
		}
	}
}

func (p *ListenerConfig) DeleteListenerInfo(username string, paras map[string]interface{}) {
	err := Public.SqlExec("DELETE FROM ListenerInfo WHERE name=?", paras["name"])
	if err == nil {
		p.GetListenerInfoList(username, nil)
		if p.CloseListener(username, paras["name"].(string)) {
			Public.PrintLog("success", username+" delete the listener "+paras["name"].(string)+".")
		}
	}
}

func (p *ListenerConfig) CloseListener(username, name string) bool {
	err := CommandControl.ListenerObj.CloseListener(name)
	if err == nil {
		Public.PrintLog("success", username+" close the listener "+name+".")
		return true
	} else {
		Public.PrintLog("error", username+" is unable to close the listener "+name+".\n"+err.Error())
		return false
	}
}
