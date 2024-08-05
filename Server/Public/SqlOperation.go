package Public

import (
	"database/sql"
	_ "github.com/mattn/go-sqlite3"
)

func SqlExec(query string, args ...any) string {
	db, err := sql.Open("sqlite3", "config/server.db")
	if err != nil {
		db.Close()
		return ""
	}

	_, err = db.Exec(query, args...)
	if err != nil {
		db.Close()
		return ""
	}

	db.Close()
	return "success"
}

func SqlSelect(query string, args ...any) []map[string]string {
	db, err := sql.Open("sqlite3", "config/server.db")
	if err != nil {
		db.Close()
		return nil
	}

	rows, _ := db.Query(query, args...)
	columns, _ := rows.Columns()
	result := make([]map[string]string, 0)

	for rows.Next() {
		values := make([]string, len(columns))
		valuePointers := make([]any, len(columns))
		for i := range columns {
			valuePointers[i] = &values[i]
		}
		err = rows.Scan(valuePointers...)
		if err != nil {
			db.Close()
			return nil
		}

		row := make(map[string]string)
		for i := range columns {
			row[columns[i]] = values[i]
		}

		result = append(result, row)
	}

	db.Close()
	return result
}
