package Public

import (
	"github.com/gin-gonic/gin"
)

type Update struct {
	// 新数据字典: [用户名: [新数据, 新数据, ...], ...]
	// 新数据: [数据类型: xxx, 用户名: xxx, 内容: xxx]
	newDataMap map[string][]map[string]string
}

var UpdateObj = Update{make(map[string][]map[string]string)}

// 获取新数据列表
func (p Update) GetNewDataList(username string, c *gin.Context) {
	c.JSON(200, UpdateObj.newDataMap[username])
	UpdateObj.newDataMap[username] = make([]map[string]string, 0)
}

// 添加新数据
func AddNewData(dataType, username, content string) {
	if username != "C2 Server" {
		_, exist := UpdateObj.newDataMap[username]
		if !exist {
			UpdateObj.newDataMap[username] = make([]map[string]string, 0)
		}
	}
	for tempUsername, _ := range UpdateObj.newDataMap {
		newData := make(map[string]string)
		newData["dataType"] = dataType
		newData["username"] = username
		newData["content"] = content
		UpdateObj.newDataMap[tempUsername] = append(UpdateObj.newDataMap[tempUsername], newData)
	}
}
