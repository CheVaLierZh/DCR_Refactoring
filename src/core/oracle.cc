#include <string>
#include <vector>
#include "core/oracle.h"
#include "core/table.h"

namespace dcr {
using std::vector;
using std::string;


Oracle::Oracle(const Table& table, const IncDegreeQuery& q) {
    if (!table.GetEnableSql()) {
        TableIterator iter = table_.GetIterator();

        while (iter.HasNext()) {
            Record r = iter.Next();

            if (q.SatisfyRange(r)) {
                nodes_.insert(r.GetRowIdx());
            }
        }
    } else {
        vector<size_t> ret = table_.Find(q.ToSqlString());
        for (size_t idx: ret) {
            nodes_.insert(idx);
        }
    }
}

}  // dcr