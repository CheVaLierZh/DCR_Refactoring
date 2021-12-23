#include "io/sqlite_table.h"
#include <unordered_map>
#include <vector>
#include <stdio.h>
#include <cstdlib>
#include "core/table.h"


namespace dcr {
using std::string;
using std::unordered_map;
using std::vector;


static int FindConflictCallback(void* data, int argc, char** argv, char** azColName) {
	unordered_map<size_t>* pMap = static_cast<unordered_map<size_t>>(data);
	int ret = atoi(argv[0]);
	pMap->insert(ret);
	return 0;
}

static int FindCallback(void* data, int argc, char** argv, char** azColName) {
	vector<size_t>*pVec = static_cast<vector<size_t>>(data);
	int ret = atoi(argv[0]);
	pVec->push_back(ret);
	return 0;
}

vector<size_t> SqliteTable::FindConflict(const Record& r) {
	unordered_map<size_t> res;
	for (FunctionalDependency& fd: fds_) {
		string sql = "select rowid from " + tablename_ + " where ";
		int flag = 0;
		for (string& attr: fd.GetLeftHandAttrs()) {
			if (flag != 0) {
				sql += "and ";
			}
			flag++;
			sql += attr + " = " + r.GetField(attr) + " ";
		}
		sql += "and (";
		flag = 0;
		for (string& attr: fd.GetRightHandAttrs()) {
			of (flag != 0) {
				sql += "or ";
			}
			sql += attr + " != " r.GetField(attr) + " ";
		}
		sql += ")";

		if (sqlite3_exec(db_, sql.c_str(), FindConflictCallback, (void*)&res, NULL) != SQLITE_OK) {
			throw "Fail to find conflict!";
		}
	}

	vector<size_t> tmp;
	for (size_t v: res) {
		tmp.push_back(v);
	}
	return tmp;
}

vector<size_t> Find(SubsetQuery& query) {
	vector<size_t> res;

	string sql = "select rowid from " + tablename_ + " where " + query.ToString();
	if (sqlite3_exec(db_, sql.c_str(), FindCallback, (void*)&res, NULL) != SQLITE_OK) {
		throw "Fail to find!";
	}

	return res;
}


}  // dcr