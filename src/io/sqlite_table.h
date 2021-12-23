#ifndef DCR_IO_SQLITE_TABLE_H_
#define DCR_CORE_SQLITE_TABLE_H_

#include <stdio.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "core/table.h"
#include "core/subset_query.h"
#include "sqlite3.h"

namespace dcr {

class Table;
class SubsetQuery;

class SqliteTable: public Table {
public:
    SqliteTable(std::string filename, std::string tablename, int file_type) {
        tablename_ = tablename;

        if (flag == 0) {  // sqlite database 
            if (sqlite3_open(filename.c_str(), &db_) != SQLITE_OK) {
                throw "Can't open database file!";
            }
        } else {
            if (sqlite3_open(":memory:", &db_) != SQLITE_OK) {
                throw "Can't open memory database!";
            }

            sqlite3_enable_load_extension(db_, 1);

            const char* zFile = "csv.so";
            char* msg;
            sqlite3_load_extension(db_, zFile, NULL, &msg);
            
            char create_table[256];
            sprintf(create_table, "CREATE VIRTUAL TABLE %s USING csv(filename=%s)", tablename_.c_str(), filename.c_str());
            if (sqlite3_exec(db_, create_table, NULL, NULL, &msg)) {
                throw "Create virtual table failed!";
            }
        }

        std::string sql = "select * from " + tablename_;

        sqlite3_stmt* stmt;
        int ret = sqlite3_prepare(db_, sql.c_str(), sql.size(), &stmt, NULL);
        if (rc) {
            fprintf(stderr, "Can't parse %s\n", sql.c_str());
            exit(1);
        }

        char *colname;
        for (int i = 0;; i++) {
            colname = sqlite3_column_name(stmt, 0);
            if (colname == NULL) {
                break;
            }
            attrs_.push_back(std::string(colname));
        }
    }

    std::vector<size_t> FindConflict(const Record& r);

    std::vector<size_t> Find(SubsetQuery&);

    SqliteTable() = delete;

    ~SqliteTable() {
        sqlite3_close(db_);
    }

    TableIterator GetIterator() {
        return SqliteTableIterator();
    }
    

private:
    sqlite3* db_;
};


class SqliteTableIterator: public TableIterator {
public:

    SqliteTableIterator(sqlite3* db, std::string& tablename) {
        db_ = db;
        std::string sql = "select rowid, * from " + tablename;

        if (sqlite3_prepare(db_, sql.c_str(), sql.size(), &stmt_, 0) != SQLITE_OK) {
            throw "Fail to iterate over table!";
        }
    }

    bool HasNext() {
        if (sqlite3_step(stmt_) != SQLITE_ROW) {
            return false;
        } 
        return true;
    }

    Record Next() {
        size_t row_idx;
        std::unordered_map<std::string, std::string> content;
        int ncols = sqlite3_column_count(stmt_);
        row_idx = (size_t)sqlite3_column_int(stmt_, 0);
        for (int i = 1; i < ncols; i++) {
            char* attr = sqlite3_column_name(stmt_, i);
            char* val = sqlite3_column_text(stmt_, i);
            content[std::string(attr)] = std::string(val);
        }
        
        Record r(row_idx, content);
        return r;
    }

private:
    sqlite3* db_;
    sqlite3_stmt* stmt_;
};

}  // dcr
#endif  // DCR_IO_SQLITE_TABLE_H_