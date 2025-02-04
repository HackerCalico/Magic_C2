package Public

import (
	"database/sql"
	_ "github.com/mattn/go-sqlite3"
	"math/rand"
	"os"
	"sync"
)

var rwMutex sync.RWMutex

func SqlExec(query string, args ...any) error {
	rwMutex.Lock()
	defer rwMutex.Unlock()

	db, err := sql.Open("sqlite3", "server.db")
	if err != nil {
		PrintLog("sqlError", "SqlExec - "+err.Error())
		return err
	}
	defer db.Close()

	_, err = db.Exec(query, args...)
	if err != nil {
		PrintLog("sqlError", "SqlExec - "+err.Error())
		return err
	}
	if rand.Intn(100) == 1 {
		_, err = db.Exec("VACUUM")
		if err != nil {
			PrintLog("sqlError", "SqlExec - "+err.Error())
			return err
		}
	}
	return nil
}

func SqlQueryRow(query string, pId *string, args ...any) error {
	rwMutex.Lock()
	defer rwMutex.Unlock()

	db, err := sql.Open("sqlite3", "server.db")
	if err != nil {
		PrintLog("sqlError", "SqlQueryRow - "+err.Error())
		return err
	}
	defer db.Close()

	err = db.QueryRow(query, args...).Scan(pId)
	if err != nil {
		PrintLog("sqlError", "SqlQueryRow - "+err.Error())
		return err
	}
	return nil
}

func SqlSelect(query string, args ...any) []map[string]string {
	rwMutex.Lock()
	defer rwMutex.Unlock()

	db, err := sql.Open("sqlite3", "server.db")
	if err != nil {
		PrintLog("sqlError", "SqlSelect - "+err.Error())
		return []map[string]string{}
	}
	defer db.Close()

	result := []map[string]string{}
	rows, err := db.Query(query, args...)
	if err != nil {
		PrintLog("sqlError", "SqlSelect - "+err.Error())
		return []map[string]string{}
	}
	columns, err := rows.Columns()
	if err != nil {
		PrintLog("sqlError", "SqlSelect - "+err.Error())
		return []map[string]string{}
	}
	for rows.Next() {
		values := make([]string, len(columns))
		valuePointers := make([]any, len(columns))
		for i := range columns {
			valuePointers[i] = &values[i]
		}

		err = rows.Scan(valuePointers...)
		if err != nil {
			PrintLog("sqlError", "SqlSelect - "+err.Error())
			return []map[string]string{}
		}

		row := map[string]string{}
		for i := range columns {
			row[columns[i]] = values[i]
		}
		result = append(result, row)
	}
	return result
}

func InitDatabase() bool {
	_, err := os.Stat("server.db")
	if os.IsNotExist(err) {
		err = SqlExec("CREATE TABLE TerminalInfo (username text DEFAULT '', sid text DEFAULT '', content text DEFAULT '')")
		if err != nil {
			RemoveDatabase()
			return false
		}
		err = SqlExec("CREATE TABLE Log (id integer PRIMARY KEY AUTOINCREMENT, time text DEFAULT '', content text DEFAULT '')")
		if err != nil {
			RemoveDatabase()
			return false
		}
		err = SqlExec("CREATE TABLE RATInfo (fid text DEFAULT '', profile text DEFAULT '', time text DEFAULT '', PRIMARY KEY (fid))")
		if err != nil {
			RemoveDatabase()
			return false
		}
		err = SqlExec("CREATE TABLE ListenerInfo (name text DEFAULT '', username text DEFAULT '', description text DEFAULT '', type text DEFAULT '', host text DEFAULT '', port text DEFAULT '', PRIMARY KEY (name))")
		if err != nil {
			RemoveDatabase()
			return false
		}
		err = SqlExec("INSERT INTO ListenerInfo (name, username, description, type, host, port) VALUES ('Phishing', 'Magician', 'Test', 'HTTP Reverse', '127.0.0.1', '1234')")
		if err != nil {
			RemoveDatabase()
			return false
		}
		err = SqlExec("CREATE TABLE CommandLog (id integer PRIMARY KEY AUTOINCREMENT, time text DEFAULT '', username text DEFAULT '', sid text DEFAULT '', type text DEFAULT '', func text DEFAULT '', paras text DEFAULT '', commandId text DEFAULT '', content text DEFAULT '', note text DEFAULT '')")
		if err != nil {
			RemoveDatabase()
			return false
		}
		err = SqlExec("CREATE TABLE SessionInfo (os text DEFAULT '', sid integer PRIMARY KEY AUTOINCREMENT, external text DEFAULT '', internal text DEFAULT '', listener text DEFAULT '', user text DEFAULT '', process text DEFAULT '', note text DEFAULT '', sleep text DEFAULT '', arch text DEFAULT '', pid text DEFAULT '', fid text DEFAULT '', connectTime text DEFAULT '', hashInfo text DEFAULT '')")
		if err != nil {
			RemoveDatabase()
			return false
		}
	}
	ColorPrint("warning", "Plaintext storage: server.db")
	return true
}

func RemoveDatabase() {
	err := os.Remove("server.db")
	if err != nil {
		ColorPrint("sqlError", "InitDatabase - "+err.Error())
	}
}
