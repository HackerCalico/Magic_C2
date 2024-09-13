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
		// 我在开发 v2.0 时发现了这个如果用户不一直接收新数据，新的会话数据就会让服务端内存空间无限堆积的问题
		// 在 v2.0 中用了非常好的解决办法，但是当前版本先用这种不太好的方式处理，因为两个版本会很不一样，不是很愿意为了老版本花时间改
		if dataType == "AddPendingSessionInfo" || dataType == "AddSessionInfo" {
			if len(UpdateObj.newDataMap[tempUsername]) > 10 {
				return
			}
		}
		newData := make(map[string]string)
		newData["dataType"] = dataType
		newData["username"] = username
		newData["content"] = content
		UpdateObj.newDataMap[tempUsername] = append(UpdateObj.newDataMap[tempUsername], newData)
	}
}
